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
on Linux (Debian, Ubuntu), Windows (MSVC, CygWin and MSYS2) and MacOs.

It should be possible to build on (or cross-compile for) other systems.


## Features
The library supports:
- floppy and hard disk images ("dumps")
- mount, unmount, create a device image (an adf file) or a volume (a partition
  inside a device)
- create, open, close, delete, rename/move a file or a directory
- file operations: read, write, truncate
- directory operations: get contents, change current, get parent
- volume operations: rebuild block allocation bitmap
- use dir cache to get directory contents
- hard- and softlinks for accessing files and directories

Untested and/or experimental support exists for:
- WinNT and Linux physical devices (with the 'native driver')
- multiple partitions harddisk dumps
- UAE hardfiles
- undel files and directories


## Command-line utilities
The `examples/` directory contains few useful command-line utilities
(as well as examples of the library usage).

Usage info is shown when they are executed without any parameters
(see also man pages).


### unADF

`unADF` is an unzip like utility for `.ADF` files:

```
unadf [-lrcsp -v n] dumpname.adf [files-with-path] [-d extractdir]
    -l : lists root directory contents
    -r : lists directory tree contents
    -c : use dircache data (must be used with -l)
    -s : display entries logical block pointer (must be used with -l)
    -m : display file comments, if exists (must be used with -l)

    -v n : mount volume #n instead of default #0 volume

    -p : send extracted files to pipe (unadf -p dump.adf Pics/pic1.gif | xv -)
    -d dir : extract to 'dir' directory
```

### adf_floppy_create

Creates an image of a floppy disk (empty, not formatted). It can create
standard DD (double density) 880K floppy image, HD (high density) 1760K, or
special formats extended number of tracks (like DD with 81-83 tracks).


### adf_format

Create a classic AmigaDOS filesystem (OFS or FFS) on the specified volume
of an ADF/HDF device.

Current limitation: it supports only unpartitioned devices/disk images,
so ones without an RDSK block and the typical hard disk structure.
Only devices with a single volume on the whole device, so only floppy disks
(ADF) or unpartitioned hard disk file (HDF) devices can be formatted.


### adf_show_metadata

A low-level utility / diagnostic tool, showing metadata about a device / device
image or a file/directory inside the Amiga filesystem. In particular, it shows
contents of Amiga filesystem metadata blocks, it can help understand structure
of Amiga filesystems (for anyone curious...).

### adf_bitmap

A low-level utility / diagnostic tool for block allocation bitmap of ADF volumes.
It can display the bitmap or rebuild it (in fact, enforce rebuilding it, even if
the volume's flag says that the bitmap is valid).

### adf_salvage

An utility allowing to list deleted entries (files, directories) on a volume
and undelete them (in the future possibly also extract them to local filesystem).

## Credits:

- main design and code : Laurent Clévy
- current maintainer, recent core developments (Dec 2022-): Tomasz Wolak
- unadf rewrite (in 2019) : Stuart Caie
- Bug fixes and C++ wrapper : Bjarke Viksoe (adfwrapper.h)
- WinNT native driver : Dan Sutherland and Gary Harris
- See also: AUTHORS

## Bugs
See BUGS.md

## Compilation and installation

See INSTALL file.


## Files
- `AUTHORS` : Contributors
- `README.md` : The file you are reading
- `ChangeLog` : updates in subsequent versions
- `INSTALL` : compilation, testing and installation information
- `TODO` : possible future changes/improvements
- `src/` :	main library files
- `src/win32/` : WinNT native driver (untested!)
- `src/linux/` : Linux native driver (experimental!)
- `src/generic/` : native files templates ("dummy" device)
- `regtests/Boot/` :	Bootblocks that might by used to put on floppy disks
- `doc/` :	The library developer's documentation, man pages for utilities
- `doc/FAQ/` : The Amiga Filesystem explained
- `examples/` : utilities: `unadf`, `adf_floppy_create/format`, `adf_show_metadata`,
`adf_bitmap`, `adf_salvage`
- `packaging` : misc. packaging configurations (so far - deb only)
- `regtests/` : regression tests
- `tests/` : unit tests


## Current status
Most of the code has certain age (the lib was designed and in large part
implemented before the year 2000). Some subsystems have improved, but still
some remain untouched for a long time and may not have dedicated tests
(meaning: are not tested).

### Native devices
The library is mostly tested with ADF disk (ie. floppy) images, not with
any native/real disk devices. Since version 0.9.0, the native devices
are not enabled by default, so it is now more safe to keep them compiled.
However, if they are not needed at all, the library still can be build
without them (with the `generic` (dummy) native device instead, see
INSTALL for details).

Enabling and using the devices remember that **a native device can be
any physical storage device on your system(!)**. Please _know what you
are doing_ (eg. do not open your `C:\` on windows or `/dev/sda` on Linux
and call a function for creating an Amiga filesystem on it... unless
you really want to reinstall your system and restore your data
from a backup...).

Since native devices are not much tested - consider them as
testing/experimental and treat as such (ie. use in a safe environment,
like a VM).

### Write support
The file read and write support are rather well tested, but still, writing
is a new and potentially harmul feature so do not experiment on a unique
copy of an ADF image with your precious data. Do it on a copy (and report
if any issues are encountered).

Update: The notice above is especially actual that it was discovered that
the version `0.8.0` and earlier **do not rebuild the block allocation bitmap
for volumes where it is marked invalid**. In short, this means that if the bitmap
is really incorrect, writing to such volume may lead to data loss/corruption
(blocks used by stored data might not be marked as such!).
Because of this, it is strongly advised to **UPDATE TO THE LATEST VERSION**.

(See also TODO and BUGS).


## The API documentation in doc/
... in major part is outdated. The lib underwent many changes and, possibly,
many are to come, not enough time to fully deal with this, too...

Please use it as a reference for the concepts and general ways of use (as those
have not changed) while for the details regarding functions check the header
files, sources in `tests/` and `examples/` to see in details how the current
API can be used.

<!--Please report any bugs or mistakes in the documentation !-->

Have fun anyway !


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
- [`fuseadf`](https://gitlab.com/t-m/fuseadf) - FUSE-based Linux filesystem allowing
to mount and access ADF images in read/write mode.
- [`patool`](https://pypi.org/project/patool/) - an archive file manager written
in Python.
#### Older projects (possibly not maintained)
- [AdfView](https://www.viksoe.dk/code/adfview.htm) - Windows shell extension
- [AdfOpus](https://adfopus.sourceforge.net/) - Windows file management tool for ADF
  ADF files.

### Bindings and ports to other languages
- [pyadf](https://sourceforge.net/projects/pyadf/) - Python bindings (for older versions)
- [adflib in Rust](https://github.com/vschwaberow/adflib)

### Other
- [adfExplorer in R](https://github.com/pepijn-devries/adfExplorer)
- [affs Linux kernel module](https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/tree/Documentation/filesystems/affs.rst)
