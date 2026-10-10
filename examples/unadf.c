/*
 *  unadf - an unzip-like utility for Amiga disk images (ADF)
 *
 *  Copyright (C) 1999-2018 Laurent Clevy
 *                2019      Stuart Caie
 *                2023      Stuart Caie, Tomasz Wolak
 *                2024-2026 Tomasz Wolak
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, see <https://www.gnu.org/licenses/>.
 *
 */


#include "unadf_arg.h"
#include "unadf_extract.h"
#include "unadf_info.h"

#include "adf_dev_driver_nativ.h"

int main(int argc, char *argv[]) {
    struct AdfDevice *dev = NULL;
    struct AdfVolume *vol = NULL;
    struct AdfList *list, *node;

    adfLibInit();
    parse_args(argc, argv);

    adfAddDeviceDriver( &adfDeviceDriverNative );

    /* open device */
    if (!(dev = adfDevOpen(adf_file, ADF_ACCESS_MODE_READONLY))) {
        fprintf(stderr, "%s: can't open device\n", adf_file);
        goto error_handler;
    }

    /* mount device */
    if (adfDevMount(dev) != ADF_RC_OK) {
        fprintf(stderr, "%s: can't mount device\n", adf_file);
        goto error_handler;
    }
    if (!pipe_mode) {
        print_device(dev);
    }

    /* mount volume */
    if (vol_number < 0 || vol_number >= dev->nVol) {
        fprintf(stderr, "%s: volume %d is invalid (device has %d volume(s))\n",
            adf_file, vol_number, dev->nVol);
        goto error_handler;
    }
    if (!(vol = adfVolMount(dev, vol_number, ADF_ACCESS_MODE_READONLY))) {
        fprintf(stderr, "%s: can't mount volume %d\n",
            adf_file, vol_number);
        goto error_handler;
    }
    if (!pipe_mode) {
        print_volume(vol);
    }

    if (pipe_mode && !file_list) {
        fprintf(stderr, "can't use pipe mode without giving a file name to extract\n");
        goto error_handler;
    }

    if (list_mode || list_all) {
        /* list files */
        if (use_dircache && adfVolHasDIRCACHE(vol)) {
            adfEnvSetProperty(ADF_PR_USEDIRC, true);
            puts("Using dir cache blocks.");
        }
        if (list_all) {
            /* list all files recursively */
            list = adfGetRDirEnt(vol, vol->curDirPtr, true);
            print_tree(list, "");
        }
        else {
            /* list contents of root directory */
            list = adfGetDirEnt(vol, vol->curDirPtr);
            for (node = list; node; node = node->next) {
                print_entry(node->content, "");
            }
        }
        adfFreeDirList(list);
    }
    else {
        /* extract files */
        if (file_list) {
            /* extract list of files given on command line */
            for (node = file_list; node; node = node->next) {
                extract_filepath(vol, node->content);

                if (pipe_mode) {
                    /* only extract one file in pipe mode */
                    break;
                }
            }
        }
        else {
            /* extract all files */
            list = adfGetRDirEnt(vol, vol->curDirPtr, true);
            extract_tree(vol, list, "");
            adfFreeDirList(list);
        }
    }

error_handler:
    if (vol) adfVolUnMount(vol);
    if (dev && dev->mounted) adfDevUnMount(dev);
    if (dev) adfDevClose(dev);
    if (file_list) adfListFree(file_list);
    adfLibCleanUp();
    return 0;
}
