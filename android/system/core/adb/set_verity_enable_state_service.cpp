/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define TRACE_TAG TRACE_ADB

#include "sysdeps.h"

#include <fcntl.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>

#include "cutils/properties.h"

#include "adb.h"
#include "adb_io.h"
#include "ext4_sb.h"
#include "fs_mgr.h"
#include "remount_service.h"

#define FSTAB_PREFIX "/fstab."
struct fstab *fstab;

#ifdef ALLOW_ADBD_DISABLE_VERITY
static const bool kAllowDisableVerity = true;
#else
static const bool kAllowDisableVerity = false;
#endif

#define VERITY_BLOCK         "/dev/block/by-name/verity_block"

static int get_target_device_size(int fd, const char *blk_device,
                                  uint64_t *device_size)
{
    int data_device;
    struct ext4_super_block sb;
    struct fs_info info;

    info.len = 0;  /* Only len is set to 0 to ask the device for real size. */

    data_device = adb_open(blk_device, O_RDONLY | O_CLOEXEC);
    if (data_device < 0) {
        WriteFdFmt(fd, "Error opening block device (%s)\n", strerror(errno));
        return -1;
    }

    if (lseek64(data_device, 1024, SEEK_SET) < 0) {
        WriteFdFmt(fd, "Error seeking to superblock\n");
        adb_close(data_device);
        return -1;
    }

    if (adb_read(data_device, &sb, sizeof(sb)) != sizeof(sb)) {
        WriteFdFmt(fd, "Error reading superblock\n");
        adb_close(data_device);
        return -1;
    }

    ext4_parse_sb(&sb, &info);
    *device_size = info.len;

    adb_close(data_device);
    return 0;
}

#if 0  /*bpi*/
/* Turn verity on/off */
static int set_verity_enabled_state(int fd, const char *block_device,
                                    const char* mount_point, bool enable)
{
    uint32_t magic_number;
    const uint32_t new_magic = enable ? VERITY_METADATA_MAGIC_NUMBER
                                      : VERITY_METADATA_MAGIC_DISABLE;
    uint64_t device_length = 0;
    int device = -1;
    int retval = -1;

    if (!make_block_device_writable(block_device)) {
        WriteFdFmt(fd, "Could not make block device %s writable (%s).\n",
                   block_device, strerror(errno));
        goto errout;
    }

    device = adb_open(block_device, O_RDWR | O_CLOEXEC);
    if (device == -1) {
        WriteFdFmt(fd, "Could not open block device %s (%s).\n", block_device, strerror(errno));
        WriteFdFmt(fd, "Maybe run adb remount?\n");
        goto errout;
    }

    // find the start of the verity metadata
    if (get_target_device_size(fd, (char*)block_device, &device_length) < 0) {
        WriteFdFmt(fd, "Could not get target device size.\n");
        goto errout;
    }

    if (lseek64(device, device_length, SEEK_SET) < 0) {
        WriteFdFmt(fd, "Could not seek to start of verity metadata block.\n");
        goto errout;
    }

    // check the magic number
    if (adb_read(device, &magic_number, sizeof(magic_number)) != sizeof(magic_number)) {
        WriteFdFmt(fd, "Couldn't read magic number!\n");
        goto errout;
    }

    if (!enable && magic_number == VERITY_METADATA_MAGIC_DISABLE) {
        WriteFdFmt(fd, "Verity already disabled on %s\n", mount_point);
        goto errout;
    }

    if (enable && magic_number == VERITY_METADATA_MAGIC_NUMBER) {
        WriteFdFmt(fd, "Verity already enabled on %s\n", mount_point);
        goto errout;
    }

    if (magic_number != VERITY_METADATA_MAGIC_NUMBER
            && magic_number != VERITY_METADATA_MAGIC_DISABLE) {
        WriteFdFmt(fd, "Couldn't find verity metadata at offset %" PRIu64 "!\n", device_length);
        goto errout;
    }

    if (lseek64(device, device_length, SEEK_SET) < 0) {
        WriteFdFmt(fd, "Could not seek to start of verity metadata block.\n");
        goto errout;
    }

    if (adb_write(device, &new_magic, sizeof(new_magic)) != sizeof(new_magic)) {
        WriteFdFmt(fd, "Could not set verity %s flag on device %s with error %s\n",
                   enable ? "enabled" : "disabled",
                   block_device, strerror(errno));
        goto errout;
    }

    WriteFdFmt(fd, "Verity %s on %s\n", enable ? "enabled" : "disabled", mount_point);
    retval = 0;
errout:
    if (device != -1)
        adb_close(device);
    return retval;
}

#else

/* Turn verity on/off */
static int set_verity_enabled_state(int fd, const char *block_device,
                                    bool enable)
{
    uint32_t magic_number;
    const uint32_t new_magic = enable ? VERITY_METADATA_MAGIC_NUMBER
                                      : VERITY_METADATA_MAGIC_DISABLE;
    uint64_t device_length = 0;
    int device = -1;
    int retval = -1;

    if (!make_block_device_writable(block_device)) {
        WriteFdFmt(fd, "Could not make block device %s writable (%s).\n",
                   block_device, strerror(errno));
        goto errout;
    }

    device = adb_open(block_device, O_RDWR | O_CLOEXEC);
    if (device == -1) {
        WriteFdFmt(fd, "Could not open block device %s (%s).\n", block_device, strerror(errno));
        WriteFdFmt(fd, "Maybe run adb remount?\n");
        goto errout;
    }

    // find the start of the verity metadata
    if (get_target_device_size(fd, (char*)block_device, &device_length) < 0) {
        WriteFdFmt(fd, "Could not get target device size.\n");
        goto errout;
    }

    if (lseek64(device, device_length, SEEK_SET) < 0) {
        WriteFdFmt(fd, "Could not seek to start of verity metadata block.\n");
        goto errout;
    }

    // check the magic number
    if (adb_read(device, &magic_number, sizeof(magic_number)) != sizeof(magic_number)) {
        WriteFdFmt(fd, "Couldn't read magic number!\n");
        goto errout;
    }

    if (!enable && magic_number == VERITY_METADATA_MAGIC_DISABLE) {
        WriteFdFmt(fd, "Verity already disabled\n");
        goto errout;
    }

    if (enable && magic_number == VERITY_METADATA_MAGIC_NUMBER) {
        WriteFdFmt(fd, "Verity already enabled\n");
        goto errout;
    }

    if (magic_number != VERITY_METADATA_MAGIC_NUMBER
            && magic_number != VERITY_METADATA_MAGIC_DISABLE) {
        WriteFdFmt(fd, "Couldn't find verity metadata at offset %" PRIu64 "!\n", device_length);
        goto errout;
    }

    if (lseek64(device, device_length, SEEK_SET) < 0) {
        WriteFdFmt(fd, "Could not seek to start of verity metadata block.\n");
        goto errout;
    }

    if (adb_write(device, &new_magic, sizeof(new_magic)) != sizeof(new_magic)) {
        WriteFdFmt(fd, "Could not set verity %s flag on device %s with error %s\n",
                   enable ? "enabled" : "disabled",
                   block_device, strerror(errno));
        goto errout;
    }

    WriteFdFmt(fd, "Verity %s\n", enable ? "enabled" : "disabled");
    retval = 0;
errout:
    if (device != -1)
        adb_close(device);
    return retval;
}

#endif /*end bpi*/

void set_verity_enabled_state_service(int fd, void* cookie)
{
    bool enable = (cookie != NULL);
    if (kAllowDisableVerity) {
        char fstab_filename[PROPERTY_VALUE_MAX + sizeof(FSTAB_PREFIX)];
        char propbuf[PROPERTY_VALUE_MAX];
        int i;
        bool any_changed = false;

        property_get("ro.secure", propbuf, "0");
        if (strcmp(propbuf, "1")) {
            WriteFdFmt(fd, "verity not enabled - ENG build\n");
            goto errout;
        }

        property_get("ro.debuggable", propbuf, "0");
        if (strcmp(propbuf, "1")) {
            WriteFdFmt(fd, "verity cannot be disabled/enabled - USER build\n");
            goto errout;
        }

        property_get("ro.hardware", propbuf, "");
        snprintf(fstab_filename, sizeof(fstab_filename), FSTAB_PREFIX"%s",
                 propbuf);

        fstab = fs_mgr_read_fstab(fstab_filename);
        if (!fstab) {
            WriteFdFmt(fd, "Failed to open %s\nMaybe run adb root?\n", fstab_filename);
            goto errout;
        }

        /* Loop through entries looking for ones that vold manages */
        for (i = 0; i < fstab->num_entries; i++) {
            if(fs_mgr_is_verified(&fstab->recs[i])) {
#if 0 /*bpi*/
                if (!set_verity_enabled_state(fd, fstab->recs[i].blk_device,
                                              fstab->recs[i].mount_point,
                                              enable)) {
#else
		if (!set_verity_enabled_state(fd, VERITY_BLOCK,
                                              enable)) {
#endif
                    any_changed = true;
                }
           }
        }

        if (any_changed) {
            WriteFdFmt(fd, "Now reboot your device for settings to take effect\n");
        }
    } else {
        WriteFdFmt(fd, "%s-verity only works for userdebug builds\n",
                   enable ? "enable" : "disable");
    }

errout:
    adb_close(fd);
}
