/*
 *  unadf - an unzip-like utility for Amiga disk images (ADF)
 *
 *  Copyright (C) 1999-2018 Laurent Clevy
 *                2019      Stuart Caie
 *                2023      Stuart Caie, Tomasz Wolak
 *                2024-2025 Tomasz Wolak
 *
 *  unadf is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  unadf is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Foobar; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include "adflib.h"
#include "adf_dev_driver_nativ.h"

//#define DEBUG_UNADF

#ifdef WIN32

 //#if !defined(__GNUC__)
#if !defined(__MINGW32__) && !defined(_CYGWIN)
#include <io.h>         // for open(), write(), ...
typedef uint32_t mode_t;
#endif

#if defined _MSC_VER
#include <direct.h>     // for _mkdir()
#define strcasecmp _stricmp
#endif

# define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
# include <sys/utime.h>

#if !defined(_CYGWIN)
// utime() does not work on directories on Windows,
// need a custom implementation
int utimeWin32( const char * const            pathname,
                const struct utimbuf * const  times );
#endif

# define DIRSEP '\\'
#else
# include <sys/time.h>
# include <unistd.h>
# define DIRSEP '/'

# ifndef O_BINARY
# define O_BINARY 0
# endif

#endif  // WIN32

#define UNADF_VERSION       ADFLIB_VERSION
#define EXTRACT_BUFFER_SIZE 8192

/* command-line arguments */
bool list_mode     = false,
     list_all      = false,
     use_dircache  = false,
     show_sectors  = false,
     show_comments = false,
     pipe_mode     = false,
     win32_mangle  = false;
char *adf_file    = NULL,
     *extract_dir = NULL;
int vol_number = 0;
struct AdfList *file_list = NULL;

/* prototypes */
void parse_args(int argc, char *argv[]);
void help(FILE * const stream);
void print_device(struct AdfDevice *dev);
void print_volume(struct AdfVolume * vol);
void print_tree(struct AdfList *node, char *path);
void print_entry(struct AdfEntry *e, char *path);
void extract_tree(struct AdfVolume *vol, struct AdfList *node, char *path);
void extract_filepath(struct AdfVolume *vol, char *filepath);
void extract_file(struct AdfVolume *vol, char *filename, char *out, mode_t perms);
char *output_name(char *path, char *name);
char *join_path(char *path, char *name);
void set_file_date(char *out, struct AdfEntry *e);
void mkdir_if_needed(char *path, mode_t perms);
mode_t permissions(struct AdfEntry *e);
void fix_win32_filename(char *name);
void set_permissions(const char * const path, const mode_t perms);
void print_error(const char *s);

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

/* parses the command line arguments into global variables */
void parse_args(int argc, char *argv[]) {
    struct AdfList *list = NULL;
    int i;
    size_t j;

    /* parse flags */
    for (i = 1; i < argc && argv[i][0] == '-'; i++) {

        /* check options with arguments first */
        char option = argv[i][1];
        if ( option == 'v' || option == 'd' ) {
            if ( i + 1 >= argc || argv[i][2] != '\0' ) {
                help(stderr);
                exit(1);
            }

            switch (option) {
            case 'v':
                vol_number = atoi ( argv[i + 1] );
                break;
            case 'd':
                extract_dir = argv[i + 1];
                break;
            //default: /* unknown option - show help and exit */
            //    help();
            //    exit(1);
            }

            i++, j = strlen(argv[i]) - 1; /* skip to next arg */
            continue;
        }

        /* check options without arguments (can be given together) */
        for (j = 1; argv[i][j]; j++) {
            switch (argv[i][j]) {
            case 'l': list_mode     = true; break;
            case 'r': list_all      = true; break;
            case 'c': use_dircache  = true; break;
            case 's': show_sectors  = true; break;
            case 'm': show_comments = true; break;
            case 'w': win32_mangle  = true; break;
            case 'p':
                pipe_mode = true;
                fprintf(stderr, list_mode
                    ? "-p option must be used with extraction, ignored\n"
                    : "sending files to pipe\n");
                break;
            case 'h':
                help(stdout);
                exit(0);
            case 'V':
                /* knowing also the build version of ADFlib is useful, in case
                   of any issues and version discrepancy (build vs. runtime) */
                printf("%s, powered by ADFlib: build   v%s (%s)\n"
                       "                           runtime v%s (%s)\n",
                       UNADF_VERSION, ADFLIB_VERSION, ADFLIB_DATE,
                       adfGetVersionNumber(), adfGetVersionDate());
                exit(0);
            default: /* unknown option - show help and exit */
                help(stderr);
                exit(1);
            }
        }
    }

    /* ADF filename must come after options */
    if (i < argc) {
        adf_file = argv[i++];
    }
    else {
        help(stderr);
        exit(1);
    }

    /* list of files to extract can follow ADF filename */
    for (; i < argc; i++) {
        list = adfListNewCell(list, argv[i]);
        if (!file_list) {
            file_list = list;
        }
    }
}

void help(FILE * const stream) {
    fprintf(stream,
        "\nunADF - an unzip-like utility for .ADF files.\n\n"
        "Usage:  unadf [-lrcsmpwV] [-v n] [-d extractdir] dumpname.adf [files-with-path]\n\n"
        "    -l : lists root directory contents\n"
        "    -r : lists directory tree contents\n"
        "    -c : use dircache data (must be used with -l)\n"
        "    -s : display entries logical block pointer (must be used with -l)\n"
        "    -m : display file comments, if exists (must be used with -l)\n"
        "    -p : send extracted files to pipe (unadf -p dump.adf Pics/pic1.gif | xv -)\n"
        "    -w : mangle filenames to be compatible with Windows filesystems\n"
        "    -h : show help\n"
        "    -V : show version\n\n"
        "    -v n : mount volume #n instead of default #0 volume\n"
        "    -d dir : extract to 'dir' directory\n\n");
}

/* prints one line of information about a device */
void print_device(struct AdfDevice *dev)
{
    printf("Device : %s. Cylinders = %d, Heads = %d, Sectors = %d. Volumes = %d\n",
        dev->type != ADF_DEVTYPE_UNKNOWN    ? adfDevTypeGetDescription( dev->type ) :
        dev->class == ADF_DEVCLASS_HARDDISK ? "Harddisk"  :
        dev->class == ADF_DEVCLASS_HARDFILE ? "Hardfile"  : "???",
        dev->geometry.cylinders, dev->geometry.heads, dev->geometry.sectors, dev->nVol);
}

/* prints one line of information about a volume */
void print_volume(struct AdfVolume *vol)
{
    ADF_SECTNUM num_blocks = vol->lastBlock - vol->firstBlock + 1;

    switch (vol->dev->class) {
    case ADF_DEVCLASS_FLOP:
        printf("Volume : %s,", adfDevTypeGetDescription( vol->dev->type ) );
        break;
    case ADF_DEVCLASS_HARDDISK:
        printf("Volume : HD partition #%d %3.1f KBytes,", vol_number, num_blocks / 2.0);
        break;
    case ADF_DEVCLASS_HARDFILE:
        printf("Volume : HardFile %3.1f KBytes,", num_blocks / 2.0);
        break;
    default:
        printf("???,");
    }

    if (vol->volName) {
        printf(" \"%s\"", vol->volName);
    }

    printf(" between sectors [%d-%d]. %s%s%s. Filled at %2.1f%%.\n",
        vol->firstBlock,
        vol->lastBlock,
        adfVolIsFFS(vol) ? "FFS" : "OFS",
        adfVolHasINTL(vol) ? " INTL" : "",
        adfVolHasDIRCACHE(vol) ? " DIRCACHE" : "",
        100.0 - ((adfCountFreeBlocks(vol) * 100.0) / num_blocks));
}

/* recurses through all directories/files and calls print_entry() on them */
void print_tree(struct AdfList *node, char *path)
{
    for (; node; node = node->next) {
        print_entry(node->content, path);
        if (node->subdir) {
            struct AdfEntry *e = (struct AdfEntry *) node->content;
            char *newpath = join_path(path, e->name);
            print_tree(node->subdir, newpath);
            free(newpath);
        }
    }
}

/* prints one line of information about a directory/file entry */
void print_entry(struct AdfEntry *e, char *path) {
    bool is_dir = e->type == ADF_ST_DIR;
    bool print_comment = show_comments && e->comment && *e->comment;

    /* do not print the links entries, ADFlib do not support them yet properly */
    if (e->type == ADF_ST_LFILE || e->type == ADF_ST_LDIR || e->type == ADF_ST_LSOFT) {
        return;
    }

    /* print "size date time [ sector ] path/filename [, comment]" */
    printf(is_dir ? "       " : "%7d", e->size);
    printf("  %4d/%02d/%02d  %2d:%02d:%02d ",
        e->year, e->month, e->days, e->hour, e->mins, e->secs);
    if (show_sectors) {
        printf(" %06d ", e->sector);
    }
    printf(" %s%s%s%s%s%s\n",
        path,
        *path ? "/" : "",
        e->name,
        is_dir ? "/" : "",
        print_comment ? ", " : "",
        print_comment ? e->comment : "");
}

/* extracts all files, recursing into directories */
void extract_tree(struct AdfVolume *vol, struct AdfList *node, char *path)
{
#ifdef DEBUG_UNADF
    fprintf( stderr, "%s: path '%s'\n", __func__, path );
#endif

    for (; node; node = node->next) {
        struct AdfEntry *e = node->content;

        /* extract file or create directory */
        char *out = output_name(path, e->name);
#ifdef DEBUG_UNADF
        fprintf( stderr, "%s: out '%s'\n", __func__, out );
#endif
        if (e->type == ADF_ST_DIR) {
            if (!pipe_mode) {
                printf("x - %s/\n", out);
                mkdir_if_needed(out, 0777 ); // temp. permissionss
                                             // ("w" is needed for extraction)
            }
        }
        else if (e->type == ADF_ST_FILE) {
            extract_file(vol, e->name, out, permissions(e));
        }

        /* recurse into subdirectories */
        if (node->subdir) {
            char *newpath = join_path(path, e->name);
            if (adfChangeDir(vol, e->name) == ADF_RC_OK) {
                extract_tree(vol, node->subdir, newpath);
                adfParentDir(vol);
            }
            else {
                fprintf(stderr, "%s: can't find directory %s in volume\n",
                    adf_file, newpath);
            }
            free(newpath);
        }

        // set timestamp
        if (!pipe_mode) {
            set_file_date(out, e);
            set_permissions( out, permissions(e) ); // real permissions
        }

        free(out);
    }
}

/* follows a path to a file on disk and extracts just that file */
void extract_filepath(struct AdfVolume *vol, char *filepath)
{
    char *p, *element, *out;

    /* skip any leading slashes */
    for (p = filepath; *p == '/';) p++;

    /* switch to root directory */
    adfToRootDir(vol);

    /* find each path element and adfChangeDir() to each element */
    for (element = p; *p; p++) {
        if (*p == '/') {
            *p = 0;
            if (*element) {
                if (adfChangeDir(vol, element) != ADF_RC_OK) {
                    fprintf(stderr, "%s: can't find directory %s in volume\n",
                        adf_file, filepath);
                    return;
                }
            }
            *p = '/';
            element = p + 1;
        }
    }

    /* extract the file using the final path element */
    if (*element) {
        struct AdfEntry * const entry = malloc(sizeof(struct AdfEntry));
        if (adfGetEntry(vol, vol->curDirPtr, element, entry) != ADF_RC_OK) {
            fprintf(stderr, "%s: can't get file permissions for %s\n",
                adf_file, filepath);
            return;
        }

        if (element == filepath) {
            /* no directory parts, just a filename */
            out = output_name("", element);
        }
        else {
            element[-1] = 0; /* split path and file */
            out = output_name(filepath, element);
        }

        extract_file(vol, element, out, permissions(entry));
        adfFreeEntry(entry);
        free(out);
    }
}

/* copies a file from the volume to a given output filename */
void extract_file(struct AdfVolume *vol, char *filename, char *out, mode_t perms)
{
    struct AdfFile *f = NULL;
    uint8_t buf[EXTRACT_BUFFER_SIZE];
    int fd = 0;

    if ((f = adfFileOpen(vol, filename, ADF_FILE_MODE_READ)) == NULL)  {
        fprintf(stderr, "%s: can't find file %s in volume\n", adf_file, filename);
        goto error_handler;
    }

    /* open local file for writing */
    if (pipe_mode) {
#ifdef _WIN32
        fd = _fileno( stdout );
        const int result = _setmode( fd, _O_BINARY );
        if ( result == -1 ) {
            perror( "Cannot set binary mode" );
            goto error_handler;
        }
#else
        fd = fileno( stdout );
#endif
    }
    else {
        printf("x - %s\n", out);
        fd = open(out, O_CREAT | O_WRONLY | O_TRUNC | O_BINARY, perms);
        if (fd < 0) {
            print_error(out);
            goto error_handler;
        }
    }

    /* copy from volume to local file until EOF */
    while (!adfFileAtEOF(f)) {
        unsigned n = adfFileRead(f, sizeof(buf), buf);
        if (n != sizeof(buf) && !adfFileAtEOF(f)) {
            fprintf(stderr, "%s: error reading %s at %u\n",
                adf_file, filename, adfFileGetPos(f));
            goto error_handler;
        }

        if (write(fd, buf, n) != n) {
            print_error(out);
            goto error_handler;
        }
    }

error_handler:
    if (fd && !pipe_mode) close(fd);
    if (f) adfFileClose(f);
}

/* combines path and file to "path/file", or just "file" if path is blank */
char *join_path(char *path, char *file) {
    char *newpath = malloc(strlen(path) + strlen(file) + 2);
    if (!newpath) {
        print_error(adf_file);
        exit(1);
    }
    sprintf(newpath, "%s%s%s", path, *path ? "/" : "", file);
    return newpath;
}

/* creates a suitable output filename from the amiga filename */
char *output_name(char *path, char *name) {
    /* maybe (extract_dir + "/") + path + "/" + name + maybe "_" + "\0" */
    size_t dirlen = ( extract_dir ? strlen(extract_dir) + 1 : 0 );
    char *out = malloc(dirlen + strlen(path) + strlen(name) + 3), *s, *o;
    if (!out) {
        print_error(adf_file);
        exit(1);
    }

    /* copy user's extract directory if needed */
    if (dirlen) {
        strcpy(out, extract_dir);
        out[dirlen - 1] = DIRSEP;
    }

    /* copy path and name, translating '/' to platform's separator */
    for (s = path, o = &out[dirlen]; *s; s++) {
        *o++ = (*s == '/') ? DIRSEP : *s;
    }
    if (*path) {
        *o++ = DIRSEP;
    }
    strcpy(o, name);

    /* alter filenames that are a problem for Windows */
    if (win32_mangle) {
        fix_win32_filename(o);
    }

    /* search for "../" and change to "xx" to stop directory traversal */
    for (o = &out[dirlen]; *o; o++) {
        if (o[0] == '.' && o[1] == '.' && (o[2] == '/' || o[2] == '\\')) {
            o[0] = o[1] = 'x';
            o += 2;
        }
    }

    if (!pipe_mode) {
        /* create directories leading up to name, if needed */
        for (o = out; *o; o++) {
            if (o > out && *o == DIRSEP) {
                *o = 0;
                mkdir_if_needed(out, 0777);
                *o = DIRSEP;
            }
        }
    }

    return out;
}

/* set amiga file date on output file */
void set_file_date(char *out, struct AdfEntry *e) {

    struct tm tm;
    tm.tm_sec = e->secs;
    tm.tm_min = e->mins;
    tm.tm_hour = e->hour;
    tm.tm_mday = e->days;
    tm.tm_mon = e->month - 1;
    tm.tm_year = e->year - 1900;
    tm.tm_isdst = -1;

    time_t time = mktime(&tm);

#ifdef WIN32
    /* on Windows, don't allow dates earlier than 1 Jan 1980 */
    if (time < 315532800) {
        time = 315532800;
    }

    struct utimbuf times;
    times.actime = times.modtime = time;

#if !defined(_CYGWIN)
    // utime() does not work on directories on Windows,
    // need a custom implementation
    if (utimeWin32(out, &times) != 0) {
        print_error(out);
    }
#else
    if (utime(out, &times) != 0) {
        print_error(out);
    }
#endif

#else
    struct timeval times[2];
    times[0].tv_sec = times[1].tv_sec = time;
    times[0].tv_usec = times[1].tv_usec = 0;
    if (utimes(out, times) != 0) {
        print_error(out);
    }
#endif
}

/* OS-agnostic make directory function */
static inline int makedir(char* path, mode_t perms)
{
#ifdef DEBUG_UNADF
    fprintf (stderr, "%s: path '%s', perms 0x%08x\n",
             __func__, path, (uint32_t) perms );
#endif

#if defined _MSC_VER
    (void) perms;
    return _mkdir(path);
#else
#if defined(__MINGW32__)
    (void) perms;
    return mkdir(path);
#else
    return mkdir(path, perms);
#endif
#endif
}


/* creates directory, if it does not already exist */
void mkdir_if_needed(char *path, mode_t perms) {
    struct stat st;
    if (stat(path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        if (makedir(path, perms) != 0) {
            print_error(path);
        }
    }
}

/* convert amiga permissions to unix permissions */
mode_t permissions(struct AdfEntry *e) {
    return (mode_t)
        (!(e->access & 4) ? 0600 : 0400) | /* rw for user */
        (e->type == ADF_ST_DIR || !(e->access & 2) ? 0100 : 0) | /* x for user */
        ((e->access >> 13) & 0007) | /* rwx for others */
        ((e->access >> 5) & 0070); /* rwx for group */
}

/* alter filenames that would cause problems on Windows:
 * https://learn.microsoft.com/en-us/windows/win32/fileio/naming-a-file
 * 1. replace characters : * " ? < > | / \ and chars 1-31 with underscore
 * 2. replace any trailng dot or space with underscore (breaks Windows File Explorer)
 * 3. if filename is CON PRN AUX NUL COM[1-9] LPT[1-9], regardless of case or
 *    extension, insert an underscore (vestiges of CP/M retained by Windows)
 *
 * In this last case, the name will be _extended_ by 1 character!
 */
void fix_win32_filename(char *name) {
    /* work forwards through name, replacing forbidden characters */
    char *o;
    for (o = name; *o; o++) {
        if (*o < 32   || *o == ':' || *o == '*' || *o == '"' || *o == '?' ||
            *o == '<' || *o == '>' || *o == '|' || *o == '/' || *o == '\\')
        {
            *o = '_';
        }
    }
    /* work backwards through name, replacing forbidden trailing chars */
    for (o--; o >= name && (*o == '.' || *o == ' '); o--) {
       *o = '_';
    }

    /* temporarily remove file extension */
    char *ext = strchr(name, '.');
    if (ext) {
        *ext = 0;
    }

    /* is the name (excluding extension) a reserved name in Windows? */
    const char *reserved_names[] = {
        "CON", "PRN", "AUX", "NUL",
        "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
        "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
    };
    for (unsigned int i = 0; i < sizeof(reserved_names)/sizeof(char **); i++) {
       if (strcasecmp(reserved_names[i], name) == 0) {
           if (ext) {
               /* restore extension and insert underscore, PRN.txt -> PRN_.txt */
               *ext = '.';
               memmove(&ext[1], ext, strlen(ext) + 1);
               *ext = '_';
           }
           else {
               /* no extension: append underscore, e.g. CON -> CON_ */
               o = &name[strlen(name)];
               *o++ = '_';
               *o++ = 0;
           }
           return;
       }
    }

    /* not a reserved name, restore file extension */
    if (ext) {
        *ext = '.';
    }
}


#if defined(WIN32) && !defined(_CYGWIN)

// utime() does not work on directories on Windows,
// need a custom implementation

#include <time.h>
#include <windows.h>

FILETIME getFileTime( time_t time );

int utimeWin32( const char * const            pathname,
                const struct utimbuf * const  times )
{
    if ( times == NULL )
        return -1;

    const HANDLE handle = CreateFileA(
        pathname, GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
        OPEN_EXISTING,
        FILE_WRITE_ATTRIBUTES |
        FILE_FLAG_BACKUP_SEMANTICS, NULL );

    if ( handle == INVALID_HANDLE_VALUE )
        return -1;

    int status = 0;

    FILETIME timeAccess = getFileTime( times->actime );
    FILETIME timeWrite  = getFileTime( times->modtime );

    if ( ( timeAccess.dwHighDateTime == 0 &&
           timeAccess.dwLowDateTime == 0 ) ||
         ( timeWrite.dwHighDateTime == 0 &&
           timeWrite.dwLowDateTime == 0 ) )
    {
        status = -1;
        goto win32_utime_clean_up;
    }

    //if ( ! SetFileTime( handle, &timeWrite, &timeAccess, &timeWrite ) ) {
    if ( ! SetFileTime( handle, NULL, &timeAccess, &timeWrite ) ) {
        //fprintf( stderr,"%s: SetFileTime failed.\n", __func__ );
        status = -1;
        goto win32_utime_clean_up;
    }

    //fprintf( stderr,"%s: SetFileTime OK.\n", __func__ );

win32_utime_clean_up:
    CloseHandle( handle );
    return status;
}


FILETIME getFileTime( time_t time )
{
    FILETIME fileTime = {
        .dwHighDateTime = 0,
        .dwLowDateTime  = 0
    };

    struct tm tm;
    if ( gmtime_s( &tm, &time ) != 0 )
        return fileTime;

    SYSTEMTIME sysTime = {
        .wYear         = (short unsigned) tm.tm_year + 1900,
        .wMonth        = (short unsigned) tm.tm_mon + 1,
        //WORD .wDayOfWeek;
        .wDay          = (short unsigned) tm.tm_mday,
        .wHour         = (short unsigned) tm.tm_hour,
        .wMinute       = (short unsigned) tm.tm_min,
        .wSecond       = (short unsigned) tm.tm_sec,
        .wMilliseconds = 0
    };

    if ( ! SystemTimeToFileTime( &sysTime, &fileTime ) )
        fileTime.dwHighDateTime =
        fileTime.dwLowDateTime  = 0;

    return fileTime;
}

#endif


#ifdef HAVE_CHMOD
#include <sys/stat.h>

void set_permissions( const char * const path,
                      const mode_t       perms )
{
    if ( chmod( path, perms ) != 0 )
        perror( path );
//    fprintf (stderr, "%s: real\n", __func__ );
}
#else
void set_permissions( const char * const path,
                      const mode_t       perms )
{
    (void) path, (void) perms;
//    fprintf (stderr, "%s: fake\n", __func__ );
}
#endif


#ifdef DEBUG_UNADF
void adfPrintBacktrace(void);
#endif

void print_error(const char *s)
{
#ifdef DEBUG_UNADF
    adfPrintBacktrace();
#endif

    perror(s);
}

#ifdef DEBUG_UNADF
#include <execinfo.h>    // for backtrace()

void adfPrintBacktrace(void)
{
    const unsigned BUFSIZE = 100;
    void *buffer[ BUFSIZE ];

    int size = backtrace( buffer, (int) BUFSIZE );
    const char * const * const strings =
        (const char * const * const) backtrace_symbols( buffer, size );

    if ( strings == NULL ) {
        perror( "error getting symbols" );
        return;
    }

    fprintf( stderr, "Obtained %d stack frames.\n", size );
    for ( int i = 0; i < size; i++ )
        fprintf( stderr, "%s\n", strings[i] );

    free( (void *) strings );
}
#endif
