#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Runs all tests in all unit test modules in this directory."""

import os
import sys
import unittest
import logging

SRC = os.path.join(os.path.dirname(__file__), os.path.pardir, os.path.pardir)


def main():  # pragma: no cover
  if 'full-log' in sys.argv:
    # Configure logging to show line numbers and logging level
    fmt = '%(module)s:%(lineno)d - %(levelname)s: %(message)s'
    logging.basicConfig(level=logging.DEBUG, stream=sys.stdout, format=fmt)
  elif 'no-log' in sys.argv:
    # Only WARN and above are shown, to standard error. (This is the logging
    # module default config, hence we do nothing here)
    pass
  else:
    # Behave as before. Make logging.info mimic print behavior
    fmt = '%(message)s'
    logging.basicConfig(level=logging.INFO, stream=sys.stdout, format=fmt)

  # Running the tests depends on having the below modules in PYTHONPATH.
  sys.path.append(os.path.join(SRC, 'tools', 'telemetry'))
  sys.path.append(os.path.join(SRC, 'third_party', 'pymock'))

  suite = unittest.TestSuite()
  loader = unittest.TestLoader()
  script_dir = os.path.dirname(__file__)
  suite.addTests(loader.discover(start_dir=script_dir, pattern='*_test.py'))

  print 'Running unit tests in %s...' % os.path.abspath(script_dir)
  result = unittest.TextTestRunner(verbosity=1).run(suite)
  return 0 if result.wasSuccessful() else 1


if __name__ == '__main__':
  sys.exit(main())  # pragma: no cover
