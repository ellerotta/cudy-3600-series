/*
<:copyright-BRCM:2019:DUAL/GPL:standard

   Copyright (c) 2019 Broadcom
   All Rights Reserved

Unless you and Broadcom execute a separate written software license
agreement governing use of this software, this software is licensed
to you under the terms of the GNU General Public License version 2
(the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
with the following added to such license:

   As a special exception, the copyright holders of this software give
   you permission to link this software with independent modules, and
   to copy and distribute the resulting executable under terms of your
   choice, provided that you also meet, for each linked independent
   module, the terms and conditions of the license of that module.
   An independent module is a module which is not derived from this
   software.  The special exception does not apply to any modifications
   of the software.

Not withstanding the above, under no circumstances may you combine
this software in any way with any other Broadcom software provided
under a license other than the GPL, without Broadcom's express prior
written consent.

:>
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <getopt.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <vfbio_lvm.h>

static int usage(FILE *fp, int argc, char **argv)
{
    fprintf(fp,
        "Usage: %s command [options]\n\n"
        "Command:\n"
        "create [-l id] [-n name] [-m mode] [-e] -s size  Create a new LUN\n"
        "delete [-l id] [-n name]          Delete LUN\n"
        "resize [-l id] [-n name] -s size  Resize LUN\n"
        "rename lun1:name1 [lun2:name2...] Rename LUN(s)\n"
        "chmod [-l id] [-n name] -m mode   Change access mode\n"
        "info [-l id] [-n name]            Get LUN info\n"
        "device_info                       Get free space on device\n"
        "get_id -n name                    Get LUN id by name\n"
        "list [--all]                      List LUNs (all or dynamic only)\n"
        "\n"
        "Options:\n"
        "-h, --help                  Print this message\n"
        "-l, --lun=lun_id            LUN id\n"
        "-n, --name=lun_name         LUN name\n"
        "-m, --mode=ro|rw            Access mode (read-only or read-write)\n"
        "-e, --encrypted             Encrypted LUN\n"
        "--all                       List all LUNs including static\n"
        "\nsizes can have a suffix multiplier of g, m, k:\n"
        "\tg - 1024 * 1024 * 1024\n"
        "\tm - 1024 * 1024\n"
        "\tk - 1024\n",
        argv[0]);
    return -1;
}

static const char short_options[] = "l:s:n:m:aex";

static const struct option
long_options[] = {
    { "lun",  required_argument, NULL, 'l' },
    { "size", required_argument, NULL, 's' },
    { "name", required_argument, NULL, 'n' },
    { "mode", required_argument, NULL, 'm' },
    { "all",  no_argument,       NULL, 'a' },
    { "encrypted", no_argument,  NULL, 'e' },
    { 0, 0, 0, 0 }
};

/*
 * Read a number with a possible multiplier.
 * Returns -1 if the number format is illegal.
 */
static int getnum(char *cp, uint64_t *value)
{
    char *endp;
    *value = strtoull(cp, &endp, 0);
    if (endp && *endp) {
        switch (tolower(*endp)) {
        case 'g':
            *value *= (1024 * 1024 * 1024);
            break;
        case 'm':
            *value *= (1024 * 1024);
            break;
        case 'k':
            *value *= 1024;
            break;
        default:
            goto bad;
        }
    }
    return 0;

bad:
    printf("Bad size: %s\n", cp);
    return -1;
}

static int validate_name_id(const char *name, int *id_ptr)
{
    int id = *id_ptr;
    int rc = 0;

    if (id < 0 && name == NULL) {
        printf("id or name is required\n");
        return -EINVAL;
    }
    if (id >= 0 && name != NULL) {
        printf("Either LUN id or LUN name should be used, but not both\n");
        return -EINVAL;
    }
    if (name != NULL) {
        rc = vfbio_lun_get_id(name, id_ptr);
        if (rc)
            printf("LUN %s doesn't exist\n", name);
    }
    return rc;
}

/*
 * Command handlers
 */

static int create_handler(int argc, char **argv)
{
    int index;
    int id = -1;
    const char *name = NULL;
    uint64_t size = 0;
    uint32_t flags = VFBIO_LUN_CREATE_FLAG_NONE;
    int rc;

    for (;;) {
        int c;

        c = getopt_long(argc, argv,
                short_options, long_options,
                &index);

        if (-1 == c)
            break;

        switch (c) {
        case 0: /* getopt_long() flag */
            break;
        case 'h':
            return usage(stdout, argc, argv);
        case 's':
            if (getnum(optarg, &size))
                return usage(stderr, argc, argv);
            break;
        case 'n':
            name = optarg;
            break;
        case 'l':
            id = atoi(optarg);
            break;
        case 'm':
            if (!strcmp(optarg, "ro") || !strcmp(optarg, "RO"))
                flags |= VFBIO_LUN_CREATE_FLAG_READ_ONLY;
            else if (strcmp(optarg, "rw") && strcmp(optarg, "RW"))
            {
                printf("Supported access modes are 'ro' and 'rw (default)\n");
                return usage(stderr, argc, argv);
            }
            break;
        case 'e':
            flags |= VFBIO_LUN_CREATE_FLAG_ENCRYPTED;
            break;

        default:
            usage(stderr, argc, argv);
            exit(EXIT_FAILURE);
        }
    }
    if (!size)    {
        printf("size is required\n");
        return -EINVAL;
    }
    if (name == NULL && id < 0) {
        printf("Either name and/or id is required\n");
        return -EINVAL;
    }
    rc = vfbio_lun_create(name, size, flags, &id);
    if (!rc)
        printf("LUN %d created\n", id);

    return rc;
}

static int delete_handler(int argc, char **argv)
{
    const char *name = NULL;
    int index;
    int id = -1;
    int rc;

    for (;;) {
        int c;

        c = getopt_long(argc, argv,
                short_options, long_options,
                &index);

        if (-1 == c)
            break;

        switch (c) {
        case 0: /* getopt_long() flag */
            break;

        case 'h':
            usage(stdout, argc, argv);
            exit(EXIT_SUCCESS);
        case 'l':
            id = atoi(optarg);
            break;
        case 'n':
            name = optarg;
            break;
        default:
            usage(stderr, argc, argv);
            exit(EXIT_FAILURE);
        }
    }
    rc = validate_name_id(name, &id);
    if (rc)
        return rc;
    rc = vfbio_lun_delete(id);
    if (!rc)
        printf("LUN %d deleted\n", id);

    return rc;
}

static int resize_handler(int argc, char **argv)
{
    int index;
    int id = -1;
    const char *name = NULL;
    uint64_t size = 0;
    int rc;

    for (;;) {
        int c;

        c = getopt_long(argc, argv,
                short_options, long_options,
                &index);

        if (-1 == c)
            break;

        switch (c) {
        case 0: /* getopt_long() flag */
            break;

        case 'h':
            return usage(stdout, argc, argv);
        case 's':
            if (getnum(optarg, &size))
                return usage(stderr, argc, argv);
            break;
        case 'l':
            id = atoi(optarg);
            break;
        case 'n':
            name = optarg;
            break;
        default:
            usage(stderr, argc, argv);
            exit(EXIT_FAILURE);
        }
    }
    if (!size) {
        printf("size is required\n");
        return -EINVAL;
    }
    rc = validate_name_id(name, &id);
    if (rc)
        return rc;
    rc = vfbio_lun_resize(id, size);
    if (!rc)
        printf("LUN %d resized\n", id);

    return rc;
}

static int rename_handler(int argc, char **argv)
{
    struct vfbio_lun_id_name id_name[VFBIO_LVM_MAX_ID_NAME_PAIRS];
    uint8_t num_luns = 0;
    char *end_p;
    int rc = 0;

    if (argc <= 2)
        return usage(stderr, argc, argv);

    for (int i = 2; i < argc; i++)
    {
        id_name[num_luns].lun_id = strtol(argv[i], &end_p, 10);
        if (!end_p || ! *end_p || *end_p != ':') {
            fprintf(stderr, "%s is invalid, must be in format id:name\n", argv[i]);
            rc = -EINVAL;
            break;
        }
        if (strlen(end_p+1) >= VFBIO_LVM_MAX_NAME_SIZE) {
            fprintf(stderr, "Name %s is too long, must be shorter than %d characters\n", end_p+1, VFBIO_LVM_MAX_NAME_SIZE);
            rc = -EINVAL;
            break;
        }
        strncpy(id_name[num_luns].lun_name, end_p + 1, VFBIO_LVM_MAX_NAME_SIZE);
        ++num_luns;
    }
    rc = rc ? rc : vfbio_lun_rename(num_luns, id_name);
    if (!rc) {
        printf("%u LUNs  renamed. The new names will become active after reboot\n", num_luns);
    }
    return rc;
}

static int chmod_handler(int argc, char **argv)
{
    int index;
    int id = -1;
    const char *name = NULL;
    int mode = -1;
    int rc;

    for (;;) {
        int c;

        c = getopt_long(argc, argv,
                short_options, long_options,
                &index);

        if (-1 == c)
            break;

        switch (c) {
        case 0: /* getopt_long() flag */
            break;

        case 'h':
            return usage(stdout, argc, argv);
        case 'm':
            if (!strcmp(optarg, "ro") || !strcmp(optarg, "RO"))
                mode = 1;
            else if (!strcmp(optarg, "rw") || !strcmp(optarg, "RW"))
                mode = 0;
            else {
                printf("Supported access modes are 'ro' and 'rw\n");
                return usage(stderr, argc, argv);
            }
            break;
        case 'l':
            id = atoi(optarg);
            break;
        case 'n':
            name = optarg;
            break;
        default:
            return usage(stderr, argc, argv);
        }
    }
    if (mode < 0) {
        printf("mode is required\n");
        return -EINVAL;
    }
    rc = validate_name_id(name, &id);
    if (rc)
        return rc;
    rc = vfbio_lun_chmod(id, mode);
    if (!rc)
        printf("LUN %d is now %s\n", id, mode ? "read-only" : "read-write");

    return rc;
}

static int info_handler(int argc, char **argv)
{
    int index;
    int id = -1;
    const char *name = NULL;
    struct vfbio_lun_descr lun_info;
    int rc;

    for (;;) {
        int c;

        c = getopt_long(argc, argv,
                short_options, long_options,
                &index);

        if (-1 == c)
            break;

        switch (c) {
        case 0: /* getopt_long() flag */
            break;

        case 'h':
            return usage(stdout, argc, argv);
        case 'l':
            id = atoi(optarg);
            break;
        case 'n':
            name = optarg;
            break;
        default:
            return usage(stderr, argc, argv);
        }
    }
    rc = validate_name_id(name, &id);
    if (rc)
        return rc;
    rc = vfbio_lun_get_info(id, &lun_info);
    if (!rc) {
        printf("LUN %d:\n", id);
        printf("  Name       : %s\n", lun_info.lun_name);
        printf("  Block size : %u (0x%x)\n", lun_info.block_size, lun_info.block_size);
        printf("  Size       : %u blocks  %llu (0x%llx) bytes\n",
            lun_info.size_in_blocks,
            (long long unsigned int)((uint64_t)lun_info.block_size * (uint64_t)lun_info.size_in_blocks),
            (long long unsigned int)((uint64_t)lun_info.block_size * (uint64_t)lun_info.size_in_blocks));
        printf("  Type       : %s\n", lun_info.dynamic ? "dynamic" : "static");
        printf("  Mode       : %s\n", lun_info.read_only ? "read-only" : "read-write");
        printf("  Encrypted  : %s\n", lun_info.encrypted ? "yes" : "no");
    }

    return rc;
}

static int device_info_handler(int argc, char **argv)
{
    int index;
    uint64_t total_size, free_size;
    int rc;

    for (;;) {
        int c;

        c = getopt_long(argc, argv,
                short_options, long_options,
                &index);

        if (-1 == c)
            break;

        switch (c) {
        case 0: /* getopt_long() flag */
            break;

        default:
            return usage(stderr, argc, argv);
        }
    }
    rc = vfbio_device_get_info(&total_size, &free_size);
    if (!rc) {
        printf("Dynamic lun info: total size = %llu (0x%llx)  free size = %llu (0x%llx)\n",
            (long long unsigned int)total_size, 
            (long long unsigned int)total_size, 
            (long long unsigned int)free_size, 
            (long long unsigned int)free_size);
    }

    return rc;
}

static int list_luns_handler(int argc, char **argv)
{
    int index;
    int rc;
    int list_all = 0;
    int lun_id = -1;
    struct vfbio_lun_descr lun_info;

    for (;;) {
        int c;

        c = getopt_long(argc, argv,
                short_options, long_options,
                &index);

        if (-1 == c)
            break;

        switch (c) {
        case 0: /* getopt_long() flag */
            break;

        case 'a':
            list_all = 1;
            break;

        default:
            return usage(stderr, argc, argv);
        }
    }

    printf("LUN list:\n");
    do {
        rc = vfbio_lun_get_next(lun_id, &lun_id);
        if (rc)
            break;
        rc = vfbio_lun_get_info(lun_id, &lun_info);
        if (rc)
            break;
        if (!lun_info.dynamic && !list_all)
            continue;
        printf("  %.2u:%.16s size=%llu bytes%s%s%s\n",
            lun_id, lun_info.lun_name,
            (long long unsigned int)((uint64_t)lun_info.block_size * (uint64_t)lun_info.size_in_blocks),
            lun_info.dynamic ? " dynamic" : "",
            lun_info.read_only ? " read-only" : "",
            lun_info.encrypted ? " encrypted" : "");
    } while (!rc);

    return (rc == VFBIO_ERROR_NO_MORE_LUNS) ? 0 : rc;
}

static int get_id_handler(int argc, char **argv)
{
    int index;
    int id = -1;
    const char *name = NULL;
    int rc;

    for (;;) {
        int c;

        c = getopt_long(argc, argv,
                short_options, long_options,
                &index);

        if (-1 == c)
            break;

        switch (c) {
        case 0: /* getopt_long() flag */
            break;
        case 'h':
            return usage(stdout, argc, argv);
        case 'n':
            name = optarg;
            break;
        default:
            usage(stderr, argc, argv);
            exit(EXIT_FAILURE);
        }
    }
    if (name == NULL) {
        printf("name is required\n");
        return -EINVAL;
    }
    rc = vfbio_lun_get_id(name, &id);
    if (!rc)
        printf("LUN %s: lun id is %d\n", name, id);

    return rc;
}

typedef int (*f_command_handler)(int argc, char **argv);

struct command_name_handler
{
    const char *command;
    f_command_handler func;
};
static struct command_name_handler command_handlers[] = {
    { "create", create_handler },
    { "delete", delete_handler },
    { "resize", resize_handler },
    { "rename", rename_handler },
    { "chmod",  chmod_handler },
    { "info",   info_handler },
    { "device_info", device_info_handler },
    { "list",   list_luns_handler },
    { "get_id", get_id_handler },
    { NULL, NULL }
};

int main(int argc, char **argv)
{
    struct command_name_handler *handler;
    int rc;

    if (argc <= 1)
        return usage(stderr, argc, argv);

    for (handler = command_handlers; handler->command != NULL; ++handler) {
        if (!strcmp(handler->command, argv[1]))
            break;
    }
    if (!handler->command) {
        return usage(stderr, argc, argv);
    }
    rc = handler->func(argc, argv);
    if (rc) {
        printf("'%s' failed with status '%s'\n",
            handler->command, vfbio_error_str(rc));
    }
    return rc;
}
