# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import itertools
import re

from recipe_engine import recipe_api

class GitApi(recipe_api.RecipeApi):
  _GIT_HASH_RE = re.compile('[0-9a-f]{40}', re.IGNORECASE)

  def __call__(self, *args, **kwargs):
    """Return a git command step."""
    name = kwargs.pop('name', 'git '+args[0])
    infra_step = kwargs.pop('infra_step', True)
    if 'cwd' not in kwargs:
      kwargs.setdefault('cwd', self.m.path['checkout'])
    git_cmd = ['git']
    if self.m.platform.is_win:
      git_cmd = [self.m.path['depot_tools'].join('git.bat')]
    options = kwargs.pop('git_config_options', {})
    for k, v in sorted(options.iteritems()):
      git_cmd.extend(['-c', '%s=%s' % (k, v)])
    can_fail_build = kwargs.pop('can_fail_build', True)
    try:
      return self.m.step(name, git_cmd + list(args), infra_step=infra_step,
                         **kwargs)
    except self.m.step.StepFailure as f:
      if can_fail_build:
        raise
      else:
        return f.result

  def fetch_tags(self, remote_name=None, **kwargs):
    """Fetches all tags from the remote."""
    kwargs.setdefault('name', 'git fetch tags')
    remote_name = remote_name or 'origin'
    return self('fetch', remote_name, '--tags', **kwargs)

  def cat_file_at_commit(self, file_path, commit_hash, remote_name=None,
                         **kwargs):
    """Outputs the contents of a file at a given revision."""
    self.fetch_tags(remote_name=remote_name, **kwargs)
    kwargs.setdefault('name', 'git cat-file %s:%s' % (commit_hash, file_path))
    return self('cat-file', 'blob', '%s:%s' % (commit_hash, file_path),
                **kwargs)

  def count_objects(self, previous_result=None, can_fail_build=False, **kwargs):
    """Returns `git count-objects` result as a dict.

    Args:
      previous_result (dict): the result of previous count_objects call.
        If passed, delta is reported in the log and step text.
      can_fail_build (bool): if True, may fail the build and/or raise an
        exception. Defaults to False.

    Returns:
      A dict of count-object values, or None if count-object run failed.
    """
    if previous_result:
      assert isinstance(previous_result, dict)
      assert all(isinstance(v, long) for v in previous_result.itervalues())
      assert 'size' in previous_result
      assert 'size-pack' in previous_result

    step_result = None
    try:
      step_result = self(
          'count-objects', '-v', stdout=self.m.raw_io.output(),
          can_fail_build=can_fail_build, **kwargs)

      if not step_result.stdout:
        return None

      result = {}
      for line in step_result.stdout.splitlines():
        name, value = line.split(':', 1)
        result[name] = long(value.strip())

      def results_to_text(results):
        return ['  %s: %s' % (k, v) for k, v in results.iteritems()]

      step_result.presentation.logs['result'] = results_to_text(result)

      if previous_result:
        delta = {
            key: value - previous_result[key]
            for key, value in result.iteritems()
            if key in previous_result}
        step_result.presentation.logs['delta'] = (
            ['before:'] + results_to_text(previous_result) +
            ['', 'after:'] + results_to_text(result) +
            ['', 'delta:'] + results_to_text(delta)
        )

        size_delta = (
            result['size'] + result['size-pack']
            - previous_result['size'] - previous_result['size-pack'])
        # size_delta is in KiB.
        step_result.presentation.step_text = (
            'size delta: %+.2f MiB' % (size_delta / 1024.0))

      return result
    except Exception as ex:
      if step_result:
        step_result.presentation.logs['exception'] = ['%r' % ex]
        step_result.presentation.status = self.m.step.WARNING
      if can_fail_build:
        raise recipe_api.InfraFailure('count-objects failed: %s' % ex)
      return None

  def checkout(self, url, ref=None, dir_path=None, recursive=False,
               submodules=True, submodule_update_force=False,
               keep_paths=None, step_suffix=None,
               curl_trace_file=None, can_fail_build=True,
               set_got_revision=False, remote_name=None,
               display_fetch_size=None, file_name=None,
               submodule_update_recursive=True):
    """Returns an iterable of steps to perform a full git checkout.
    Args:
      url (str): url of remote repo to use as upstream
      ref (str): ref to fetch and check out
      dir_path (Path): optional directory to clone into
      recursive (bool): whether to recursively fetch submodules or not
      submodules (bool): whether to sync and update submodules or not
      submodule_update_force (bool): whether to update submodules with --force
      keep_paths (iterable of strings): paths to ignore during git-clean;
          paths are gitignore-style patterns relative to checkout_path.
      step_suffix (str): suffix to add to a each step name
      curl_trace_file (Path): if not None, dump GIT_CURL_VERBOSE=1 trace to that
          file. Useful for debugging git issue reproducible only on bots. It has
          a side effect of all stderr output of 'git fetch' going to that file.
      can_fail_build (bool): if False, ignore errors during fetch or checkout.
      set_got_revision (bool): if True, resolves HEAD and sets got_revision
          property.
      remote_name (str): name of the git remote to use
      display_fetch_size (bool): if True, run `git count-objects` before and
        after fetch and display delta. Adds two more steps. Defaults to False.
      file_name (str): optional path to a single file to checkout.
      submodule_update_recursive (bool): if True, updates submodules
          recursively.
    """
    # TODO(robertocn): Break this function and refactor calls to it.
    #     The problem is that there are way too many unrealated use cases for
    #     it, and the function's signature is getting unwieldy and its body
    #     unreadable.
    display_fetch_size = display_fetch_size or False
    if not dir_path:
      dir_path = url.rsplit('/', 1)[-1]
      if dir_path.endswith('.git'):  # ex: https://host/foobar.git
        dir_path = dir_path[:-len('.git')]

      # ex: ssh://host:repo/foobar/.git
      dir_path = dir_path or dir_path.rsplit('/', 1)[-1]

      dir_path = self.m.path['slave_build'].join(dir_path)

    if 'checkout' not in self.m.path:
      self.m.path['checkout'] = dir_path

    git_setup_args = ['--path', dir_path, '--url', url]

    if remote_name:
      git_setup_args += ['--remote', remote_name]
    else:
      remote_name = 'origin'

    if self.m.platform.is_win:
      git_setup_args += ['--git_cmd_path',
                         self.m.path['depot_tools'].join('git.bat')]

    step_suffix = '' if step_suffix is  None else ' (%s)' % step_suffix
    steps = [
        self.m.python(
            'git setup%s' % step_suffix,
            self.m.path['build'].join('scripts', 'slave', 'git_setup.py'),
            git_setup_args),
    ]

    # There are five kinds of refs we can be handed:
    # 0) None. In this case, we default to properties['branch'].
    # 1) A 40-character SHA1 hash.
    # 2) A fully-qualifed arbitrary ref, e.g. 'refs/foo/bar/baz'.
    # 3) A fully qualified branch name, e.g. 'refs/heads/master'.
    #    Chop off 'refs/heads' and now it matches case (4).
    # 4) A branch name, e.g. 'master'.
    # Note that 'FETCH_HEAD' can be many things (and therefore not a valid
    # checkout target) if many refs are fetched, but we only explicitly fetch
    # one ref here, so this is safe.
    fetch_args = []
    if not ref:                                  # Case 0
      fetch_remote = remote_name
      fetch_ref = self.m.properties.get('branch') or 'master'
      checkout_ref = 'FETCH_HEAD'
    elif self._GIT_HASH_RE.match(ref):        # Case 1.
      fetch_remote = remote_name
      fetch_ref = ''
      checkout_ref = ref
    elif ref.startswith('refs/heads/'):       # Case 3.
      fetch_remote = remote_name
      fetch_ref = ref[len('refs/heads/'):]
      checkout_ref = 'FETCH_HEAD'
    else:                                     # Cases 2 and 4.
      fetch_remote = remote_name
      fetch_ref = ref
      checkout_ref = 'FETCH_HEAD'

    fetch_args = [x for x in (fetch_remote, fetch_ref) if x]
    if recursive:
      fetch_args.append('--recurse-submodules')

    fetch_env = {}
    fetch_stderr = None
    if curl_trace_file:
      fetch_env['GIT_CURL_VERBOSE'] = '1'
      fetch_stderr = self.m.raw_io.output(leak_to=curl_trace_file)

    fetch_step_name = 'git fetch%s' % step_suffix
    if display_fetch_size:
      count_objects_before_fetch = self.count_objects(
          name='count-objects before %s' % fetch_step_name,
          cwd=dir_path,
          step_test_data=lambda: self.m.raw_io.test_api.stream_output(
              self.test_api.count_objects_output(1000)))
    self('retry', 'fetch', *fetch_args,
      cwd=dir_path,
      name=fetch_step_name,
      env=fetch_env,
      stderr=fetch_stderr,
      can_fail_build=can_fail_build)
    if display_fetch_size:
      self.count_objects(
          name='count-objects after %s' % fetch_step_name,
          cwd=dir_path,
          previous_result=count_objects_before_fetch,
          step_test_data=lambda: self.m.raw_io.test_api.stream_output(
              self.test_api.count_objects_output(2000)))

    if file_name:
      self('checkout', '-f', checkout_ref, '--', file_name,
        cwd=dir_path,
        name='git checkout%s' % step_suffix,
        can_fail_build=can_fail_build)

    else:
      self('checkout', '-f', checkout_ref,
        cwd=dir_path,
        name='git checkout%s' % step_suffix,
        can_fail_build=can_fail_build)

    if set_got_revision:
      rev_parse_step = self('rev-parse', 'HEAD',
                           cwd=dir_path,
                           name='set got_revision',
                           stdout=self.m.raw_io.output(),
                           can_fail_build=False)

      if rev_parse_step.presentation.status == 'SUCCESS':
        sha = rev_parse_step.stdout.strip()
        rev_parse_step.presentation.properties['got_revision'] = sha

    clean_args = list(itertools.chain(
        *[('-e', path) for path in keep_paths or []]))

    self('clean', '-f', '-d', '-x', *clean_args,
      name='git clean%s' % step_suffix,
      cwd=dir_path,
      can_fail_build=can_fail_build)

    if submodules:
      self('submodule', 'sync',
        name='submodule sync%s' % step_suffix,
        cwd=dir_path,
        can_fail_build=can_fail_build)
      submodule_update = ['submodule', 'update', '--init']
      if submodule_update_recursive:
        submodule_update.append('--recursive')
      if submodule_update_force:
        submodule_update.append('--force')
      self(*submodule_update,
        name='submodule update%s' % step_suffix,
        cwd=dir_path,
        can_fail_build=can_fail_build)

  def get_timestamp(self, commit='HEAD', test_data=None, **kwargs):
    """Find and return the timestamp of the given commit."""
    step_test_data = None
    if test_data is not None:
      step_test_data = lambda: self.m.raw_io.test_api.stream_output(test_data)
    return self('show', commit, '--format=%at', '-s',
                stdout=self.m.raw_io.output(),
                step_test_data=step_test_data).stdout.rstrip()

  def rebase(self, name_prefix, branch, dir_path, remote_name=None,
             **kwargs):
    """Run rebase HEAD onto branch
    Args:
    name_prefix (str): a prefix used for the step names
    branch (str): a branch name or a hash to rebase onto
    dir_path (Path): directory to clone into
    remote_name (str): the remote name to rebase from if not origin
    """
    remote_name = remote_name or 'origin'
    try:
      self('rebase', '%s/master' % remote_name,
          name="%s rebase" % name_prefix, cwd=dir_path, **kwargs)
    except self.m.step.StepFailure:
      self('rebase', '--abort', name='%s rebase abort' % name_prefix,
          cwd=dir_path, **kwargs)
      raise

  def config_get(self, prop_name, **kwargs):
    """Returns: (str) The Git config output, or None if no output was generated.

    Args:
      prop_name: (str) The name of the config property to query.
      kwargs: Forwarded to '__call__'.
    """
    kwargs['name'] = kwargs.get('name', 'git config %s' % (prop_name,))
    result = self('config', '--get', prop_name, stdout=self.m.raw_io.output(),
                  **kwargs)

    value = result.stdout
    if value:
      value = value.strip()
      result.presentation.step_text = value
    return value

  def get_remote_url(self, remote_name=None, **kwargs):
    """Returns: (str) The URL of the remote Git repository, or None.

    Args:
      remote_name: (str) The name of the remote to query, defaults to 'origin'.
      kwargs: Forwarded to '__call__'.
    """
    remote_name = remote_name or 'origin'
    return self.config_get('remote.%s.url' % (remote_name,), **kwargs)

  def bundle_create(self, bundle_path, rev_list_args=None, **kwargs):
    """Run 'git bundle create' on a Git repository.

    Args:
      bundle_path (Path): The path of the output bundle.
      refs (list): The list of refs to include in the bundle. If None, all
          refs in the Git checkout will be bundled.
      kwargs: Forwarded to '__call__'.
    """
    if not rev_list_args:
      rev_list_args = ['--all']
    self('bundle', 'create', bundle_path, *rev_list_args, **kwargs)
