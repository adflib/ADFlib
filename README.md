```
      _/_/    _/_/_/    _/_/_/_/  _/  _/  _/
   _/    _/  _/    _/  _/        _/      _/_/_/
  _/_/_/_/  _/    _/  _/_/_/    _/  _/  _/    _/
 _/    _/  _/    _/  _/        _/  _/  _/    _/
_/    _/  _/_/_/    _/        _/  _/  _/_/_/

Amiga Disk File library
```

The `ADFlib` is a free, portable and open implementation of the Amiga filesystem.

The library is written in portable C (currently C99).

The origins of the library reaches late 1990s, when the initial version
was designed and developed by Laurent Clévy on his Amiga 1200 (with
a big-endian CPU: MC68EC020 14MHz).

Recent versions were built and tested in a CI system (GitHub Actions)
on Linux (Debian, Ubuntu), Windows (MSVC, CygWin and MSYS2) and MacOS.

It should be possible to build on (or cross-compile for) other systems.


## Features
The main purpose of the library is to allow read and write Amiga-formatted
devices and files being byte-level copies of such devices, called disk images
or dumps. In case of classic Amiga systems, such files are most often
[ADFs](https://en.wikipedia.org/wiki/Amiga_Disk_File) or HDFs ("Hard Disk
Files") containing
[OFS](https://en.wikipedia.org/wiki/Amiga_Old_File_System) or
[FFS](https://en.wikipedia.org/wiki/Amiga_Fast_File_System) (there are also
other Amiga filesystems, for instance
[PFS](https://en.wikipedia.org/wiki/Professional_File_System)).

ADFlib allows accessing the aforementioned devices on 3 levels:
1. device level - access to blocks (storage units) of the whole device
2. volume level - access to blocks of the logical parts of the device,
   which, in case of the Amiga ecosystem, are called volumes
3. filesystem level - access to files and directories stored inside volumes.

Considering these 3 levels, the library supports:
1. devices
   - formats:
      - ADFs - floppy disks (standard DD/880 KiB and HD/1780 KiB and non-standard)
      - HDFs 
	     - hard disk files (unpartitioned devices larger than floppies)
	     - hard disks (partitioned, with Rigid Disk Block)
		    - limited testing (-> experimental)
            - only partial support for partitioning, writing Rigid Disk Block etc.
               - ie. due to lack of physical hardware info (parking zones etc.)
               - hard disks should be partitioned on Amiga or with dedicated software
         - depending on OS: up to 2GB or 4GB (see limits)
   - device types (implemented with "drivers"):
      - dump files
      - physical ("native") devices (so far: on Windows and Linux)
	     - limited testing (-> experimental)
      - ramdisk
      - user-implemented devices
   - operations: create and open, read/write blocks
2. volumes
   - up to 4GiB (OFS/FFS limit)
   - operations: create (format) and open, read/write blocks
3. filesystem:
   - "classic" Amiga filesystems: OFS and FFS
   - file operations: read, write, truncate
   - directory operations: get contents, create, remove, rename, move files
     and directories
   - use dircache to get directory contents
   - use hard- and softlinks for accessing files and directories
   - rebuild block allocation bitmap
   - recover deleted files and directories (experimental)

## Library usage (in C or C++)

Library initialization and setup:
```C
#include <adflib.h>

int main( void )
{
    // library initialization
    adfLibInit();

    // (optional) library setup
    if ( adfEnvSetProperty( ADF_PR_QUIET, false ) != ADF_RC_OK )
        goto library_cleanup;

    // (optional) add / remove a device driver
    // by default 2 are available:
    //   adfDeviceDriverDump 
    //   adfDeviceDriverRamdisk
    // no need to do anything for using those

    access_devices();

library_cleanup:
    adfLibCleanUp();
}
```

Accessing a dump device:
```C
void access_dump_devices(void)
{
    // open a device (dump file)
    struct AdfDevice * const dev = adfDevOpen( "mydisk.adf", 
                                               ADF_ACCESS_MODE_READWRITE );
    if ( dev == NULL )
        return;

    work_on_device( dev );

    // close the device
    adfDevClose( dev );
}
```

Accessing a physical device:
```C
void access_native_devices(void)
{
    // need to add the driver to access physical devices
    // (available on Linux and Windows)
    //
    // (USE VERY CAREFULLY: all physical disk devices, including your
    // system disk(!!!) can be accessed using this method)
    adfAddDeviceDriver( &adfDeviceDriverNative );

    // opening a native device on Linux
    struct AdfDevice * const dev = adfDevOpen( "/dev/sde",
                                               ADF_ACCESS_MODE_READONLY );

    // opening a native device on Windows
    //
    // "5" in this example is the numerical id of the physical(!)
    // disk device on your system, meaning:   \\.\PhysicalDrive5
    // (if you do not know what it is, better do not use the native device driver...)
    struct AdfDevice * const dev = adfDevOpen( "|H5", ADF_ACCESS_MODE_READONLY );

    if ( dev == NULL )
        return;

    work_on_device( dev );

    // close the device
    adfDevClose( dev );
}
```

Before accessing device's volumes, information about them must be retrieved.
The "mount" operation on an opened device serves that purpose:
```C
void work_on_device( struct AdfDevice * const dev )
{
    // read/write device blocks or mount device (ie. get volume information)

    if ( adfDevMount( dev ) != ADF_RC_OK )
        return;

    access_volumes( dev );
	
    adfDevUnMount( dev );
}
```

Accessing a specific volume:
```C
void access_volumes( struct AdfDevice * const dev )
{
    // mount the first volume of the device
    struct AdfVolume * const vol = adfVolMount( dev, 0, 
                                                ADF_ACCESS_MODE_READONLY );
    if ( vol == NULL )
        return;

    work_on_volume( vol );

    // unmount the volume
    adfVolUnMount( vol );
}
```

Working on a volume (read/write blocks, open and read files, directories etc.):
```C
void work_on_volume( struct AdfVolume * const vol )
{
    // use "s/" directory
    adfChangeDir( vol, "s" );

    // open a file
    struct AdfFile * const file = adfFileOpen( vol, "startup-sequence",
                                               ADF_FILE_MODE_READ );
    if ( file == NULL )
        return;
	
    // read the file
    const unsigned bufsize = 256;
    char buf[ bufsize ];
    while ( ! adfFileAtEOF( file ) ) {
        adfFileRead( file, bufsize, buf );
    }

    // close the file
    adfFileClose( file );

    // go back to the main (root) directory
    adfToRootDir( vol );
}
```


## Command-line utilities
The `examples/` directory contains couple of useful command-line utilities
(as well as examples of the library usage).

Usage info is shown when they are executed without any parameters
(see also man pages in `doc/`).


### unadf
`unadf` is an unzip-like utility for `.ADF` files.

```
Usage:  unadf [-lrcsmpwV] [-v n] [-d extractdir] dumpname.adf [files-with-path]

    -l : lists root directory contents
    -r : lists directory tree contents
    -c : use dircache data (must be used with -l)
    -s : display entries logical block pointer (must be used with -l)
    -m : display file comments, if exists (must be used with -l)
    -p : send extracted files to pipe (unadf -p dump.adf Pics/pic1.gif | xv -)
    -w : mangle filenames to be compatible with Windows filesystems
    -h : show help
    -V : show version

    -v n : mount volume #n instead of default #0 volume
    -d dir : extract to 'dir' directory
```

### adfimgcreate
Create empty (not formatted!) ADF/HDF image files of specified type or size.

### adfformat
Create a classic AmigaDOS filesystem (OFS or FFS) on the specified volume
of an ADF/HDF device.

Current limitation: it supports only unpartitioned devices/disk images,
so ones without an RDSK block and the typical hard disk structure.
Only devices with a single volume on the whole device, so only floppy disks
(ADF) or unpartitioned hard disk file (HDF) devices can be formatted.

### adfinfo
A low-level utility / diagnostic tool, showing metadata about an ADF device,
volume or a file/directory inside the Amiga filesystem. In particular, it shows
contents of Amiga filesystem metadata blocks, so it can help understand structure
of Amiga filesystems (for anyone curious...).

### adfbitmap
A low-level utility / diagnostic tool for block allocation bitmap of ADF volumes.
It can display the bitmap or rebuild it (in fact, enforce rebuilding it, even if
the volume's flag says that the bitmap is valid).

### adfsalvage
An utility allowing to list deleted entries (files, directories) on a volume
and, if possible, undelete them (in the future possibly also extract them
to local filesystem).

## Credits:
- main design and code : Laurent Clévy
- main developer (Dec 2022-): Tomasz Wolak
- unadf rewrite (in 2019) : Stuart Caie
- WinNT native driver : Dan Sutherland and Gary Harris
- See also: AUTHORS

## Bugs
See BUGS.md

## Compilation and installation
See INSTALL.


## Files
- `AUTHORS` : Contributors
- `README.md` : The file you are reading
- `ChangeLog` : Updates in subsequent versions
- `INSTALL` : Compilation, testing and installation information
- `TODO` : Possible future changes/improvements
- `src/` :	Main library files
- `src/win32/` : WinNT native device driver
- `src/linux/` : Linux native device driver
- `src/generic/` : Native device driver template ("dummy" device)
- `doc/` :	The library developer's documentation, man pages for utilities
- `doc/FAQ/` : The Amiga Filesystem explained
- `examples/` : Utilities: `unadf`, `adfimgcreate`, `adfformat`,
`adfinfo`, `adfbitmap`, `adfsalvage`
- `packaging/` : Packaging configurations (so far - deb only)
- `tests/data/Boot/` : Bootblocks that might by used to put on floppy disks
- `tests/regs` : Regression tests
- `tests/unit` : Unit and functional tests
- `tests/examples` : Tests of command-line utilities


## Current status
As for version 0.10.0, _most_ of the library functionality is tested (even if not
100%...).

Main points:
- The library can be used safely for read-only access.
- Read-write access should be used more carefully (see below).
- For volumes with dircache (esp. in write mode!) - consider the library **not tested**.
- The use of native devices (real hard disks or floppies) on Linux and Windows
  is tested only briefly (in any case - use them very carefully! read below).

### Write support
The file read and write support are relatively well tested (except dir. cache,
see below!), but still, writing is a relatively new and potentially harmul
feature so **do not experiment on a unique copy of an ADF image with your
precious data**. Do it on a copy (and report any issues if encountered).

If you use the ADFlib for writing data, see the BUGS file for information about
the issues found. It is very strongly advised to **UPDATE TO THE LATEST VERSION**,
where any issues found are fixed.

(See also TODO and BUGS).

### Native devices
The library is mostly tested with ADF disk (ie. floppy) images, only briefly
with native/real disk devices. Since version 0.9.0, the native devices
are not enabled by default, so it is now more safe to keep them compiled.
However, if they are not needed at all, the library still can be build
without them (with the `generic` (dummy) native device instead, see
INSTALL for details).

Enabling and using native devices remember that **a native device can be
any physical storage device on your system(!)** - so **know (and be certain)
what you are doing** (!!!). For instance, do not open your `C:\` (`|H0`)
on Windows or `/dev/sda` on Linux and call a function for creating an Amiga
filesystem on it... unless you _really_ want to reinstall your system and
restore your data from a backup...).

Since native devices are not much tested - consider them as
testing/experimental and treat as such (ie. use in a safe environment,
like a VM).

### DirCache
Dircache is an existing but somewhat exotic feature of the FFS.
Until the time of writing this, I haven't encountered any existing disk image
(adf or hdf) with dircache enabled (the only one known dump file with dircache
enabled is one of the test floppies for the ADFlib: `testffs.adf`).

While dircache support is implemented in the ADFlib (at least, to certain
extent), so far, there are very few tests of dircache, only on simple dump
image created for testing (no real cases). Assume that, as such, this feature
is practically **not tested**.
While volumes with dircache can be used rather safely in read-only mode - be
very careful writing volumes with dircache (always do it on a copy).


### Limits
There are certain limits that comes from the design and the way devices are
used. The main goal (so far) is portability, so the functions used are standard
ones (ie. `stdio` for dump files). This, however, can limit device sizes in
some cases.

This can be improved in the future (esp. if signals that it is needed appears),
but it may have to be implemented specifically for each target OS.

#### Dump file size limit
The library uses `stdio` for accessing dump files. This implies use of
`long` type as offset in files. On 32-bit systems this might be a 32-bit
(signed) value. This limits the max. size of dump files to 2GiB. 64-bit systems
_should_ support bigger dumps (but this was not tested! If anyone uses bigger
dumps - feedback welcomed).

#### Native device size limit
This (also) depends on the target operating system (precisely on `off_t` type,
used with `lseek`). On a 64-bit OS, devices larger than 4GiB _should_ be
available (on a 32-bit OS, not so sure...).

#### Volume size limit
Volumes must, obviously, fit the ADF device. Data in a volume is addressed with
blocks of 512-bytes, so, theoretically, can be quite large: 2GiB (signed int,
32bit) * 512 = 1TiB. In practice, volumes should not be larger than what is
supported by AmigaOS (4GiB).

## Documentation

### The API documentation in doc/
... in major part is outdated. The lib underwent many changes and, possibly,
some still to come. Not enough time to fully deal with this, too...

Please use it as a reference for the concepts and general ways of use (as those
have not changed) while for the details regarding functions check the header
files, sources in `tests/` and `examples/` to see in details how the current
API can be used.

### Command-line programs (examples)
See `doc/` (man pages).

### ADF format (devices and filesystems):
- [The .ADF (Amiga Disk File) format FAQ](http://lclevy.free.fr/adflib/adf_info.html)
  by [Laurent Clévy](http://lclevy.free.fr/) (also
  [here](https://adflib.github.io/FAQ/adf_info.html)) - a comprehensive study
  of the Amiga storage technology - from disk data encoding through disk
  geometry to filesystem structure (OFS/FFS)
- [Wikipedia article about ADF](https://en.wikipedia.org/wiki/Amiga_Disk_File)

### Misc.

#### Using native devices with command-line programs
In the version 0.10.0, the only programs that has enabled support for native
devices are `adfinfo` and `unadf` (both are using the ADF devices in read-only
mode).

So far, native devices are supported on 2 operating systems: Windows and Linux.
Native devices are distinguished from regular dump files by special naming
convensions, which, depending on the operating system, are as follows:
- on Linux - any file specified as `/dev/....` (so any Linux device file) is
opened as a native device
- on Windows - device name specified as `|Hx`, where '`x`' is the numerical id
of the physical disk (equivalent of Windows pathname: `\\.\PhysicalDiskX`).
is opened as a native device. Note that "`|`" is a special character for system
pipe (sending data to another process), so the device name must be given within
"" (double quotes).


## Contributing
If you encountered a problem, please review
[the existing issues](https://github.com/adflib/ADFlib/issues) and,
if the problem you have is not already there, open a new one.

For bugfixes and/or new things - please open a _Pull Request_ to the `devel`
branch (not the `master`).

### Repository structure
Using the code from the repository will be easier understanding that the project
has adapted
[a new branching model](https://nvie.com/posts/a-successful-git-branching-model/)
and so far use the following:
- the `master` branch contains the latest tagged and released code
- the `devel` branch contains development code that was tested and accepted
  for future releases
- other existing branches are new "feature" branches (that may or may not be merged)

Other things of the scheme may be used/adapted as needed (hotfixes for released code
and such, very likely release branch(es) will also appear).


## Related projects

### Projects using ADFlib
- [adfExplorer](https://pepijn-devries.github.io/adfExplorer/) ([GitHub](https://github.com/pepijn-devries/adfExplorer)) - package providing access to ADFs in R
- [`ADFinder` and `send2ADF`](https://ginnov.github.io/littlethings/) ([GitHub](https://github.com/GINNOV/littlethings/tree/master/Amiga/Tools)) - ADF tools for MacOS
- [DiskFlashBack](https://robsmithdev.co.uk/diskflashback) ([GitHub](https://github.com/RobSmithDev/DiskFlashback)) - Windows app for viewing and editing misc. disk formats (ADFs are among them).
- [`fuseadf`](https://gitlab.com/t-m/fuseadf) - FUSE-based filesystem allowing
to mount and access ADF images in read/write mode.
- [`patool`](https://pypi.org/project/patool/) - an archive file manager written
in Python.

#### Older projects (not sure if maintained)
- [AmigaDX](https://github.com/pbakota/amigadx) - plugin for Total Commander for reading/writing Amiga disk dump files
- [AdfView](https://www.viksoe.dk/code/adfview.htm) - Windows shell extension
- [AdfOpus](https://adfopus.sourceforge.net/) - Windows file management tool for ADF files.
- [CopyToAdf](https://www.pouet.net/prod_nfo.php?which=65625&font=4) - part of WinUAE Demo Toolchain 5
- [pyadf](https://sourceforge.net/projects/pyadf/) - Python bindings (for older versions)
  - note that this is neither
    [scripting framework for multiscale quantum chemistry](https://onlinelibrary.wiley.com/doi/full/10.1002/jcc.21810)
	([on GitHub](https://github.com/chjacob-tubs/pyadf-releases))
    nor [a Python library to help with Atlassian Document Format](https://github.com/tamus-cyber/pyadf)
	(what a popular name this is...)

### Other
- [adflib in Rust](https://github.com/vschwaberow/adflib)
- [affs Linux kernel module](https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/tree/Documentation/filesystems/affs.rst)([doc](https://docs.kernel.org/filesystems/affs.html))
- [amitools](https://amitools.readthedocs.io/) - a set of useful tools in Python
- [DiskSalv](https://www.nightvzn.net/portfolio/web/amiga_monitor/archives/1_5html/disksalv.htm) ([AmiNet](https://aminet.net/package/disk/salv/DiskSalv))- DiskSalv for AmigaOS

### ADF in AmiNet
- [All](https://aminet.net/search?type=advanced&name=adf)
- in [disk/](https://aminet.net/search?type=advanced&name=adf&q_path=AND&path%5B%5D=disk)
- in [doc/](https://aminet.net/search?type=advanced&name=adf&q_path=AND&path%5B%5D=doc)
- in [util/](https://aminet.net/search?type=advanced&name=adf&q_path=AND&path%5B%5D=util)
- [Empty ADF files](https://aminet.net/package/disk/misc/Empty_ADF_files)
