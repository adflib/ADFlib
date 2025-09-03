
## General information

There are 2 buildsystems that can be used - CMake and autotools (links below).
Obviously, a requirement for building is installation of the chosen one.

Testing require also the Check testing framework for C (link below).

Building deb packages require typical tools for that (ie. helper script uses
debuild from devscripts).


## Building with CMake

There are helper scripts in the `util/` directory, which should help doing quick
builds. For instance, to build a release version (both static and binary lib.):
1. `$ util/cmake_release_configure`
2. `$ util/cmake_release_build`

Debug version (for using with `gdb` and an address sanitizer, where available...)
can be built in the same way, just replace "release" with "debug" in the commands
above.

To clean the build subdirectory use:
```
$ util/cmake_clean
```

(all have to be configured again for building).

Native device driver type (generic/dummy device vs. real physical linux/win32
devices) can be selected by setting `ADFLIB_ENABLE_NATIVE_DEV` option (to `ON`
or `OFF`). For instance:
```
$ util/cmake_shared_configure -DADFLIB_ENABLE_NATIVE_DEV:BOOL=ON
```
would configure build with shared library and support for the real, physical
devices. Setting `OFF` would enforce using generic driver.

Other options allow to build with or without tests:
```
ADFLIB_ENABLE_TESTS, ADFLIB_ENABLE_UNIT_TESTS
```
and build with or without the address sanitizer (normally used only for
debugging/sanitizing memory problems): `ADFLIB_ENABLE_ADDRESS_SANITIZER`


## Building with Autotools

Standard way:
```
1. $ ./autogen.sh

2. $ ./configure

   or better (see the warning below):

   $ ./configure --enable-native-generic

3. $ make
```

Native devices can be selected with configure script switches:
- `--enable-native-generic`  Enable generic native devices
- `--enable-native-linux  `  Enable Linux native devices
- `--enable-native-win32  `  Enable Win32 native devices

Note that since version `0.9.0`, native devices (their "driver"),
even if compiled, is not enabled by default, so if you want
to experiment with it (with caution!), the driver must be enabled
in the code.


## Building with Visual Studio

Existing CMake configuration should allow to build with VS. Just open
the directory with the project (or possibly clone the repo with VS).
It should configure automatically, and it can be build as usual ("Build all").

There are 2 debug and 2 release configurations for VS, allowing to build
a static or a shared (DLL) library. All use MSVC compiler (but this can be
changed easily in CMake settings, eg. to `clang` ).


## Building Debian packages

To build a `.deb` package for Debian (or any derivative), use the helper
script:
```
$ util/deb_build.sh
```

In the parent(!) directory (to that of the project), it will create:
- a source package (`.tar.gz`)
- binary packages (`.deb`)

The source package alone can be created with:
```
$ util/deb_create_quilt3_archive.sh
```

## Testing

There are 3 sets of tests:
- `tests/regr/` - contain regression and some functional tests (with test data,
  like adf images)
- `tests/unit/` - mainly unit tests; require Check testing framework (link below)
- `tests/examples/` - contains some basic tests of the command-line tools

Note that:
- on Windows (Visual Studio, CygWin) tests with shared library will fail,
  unless the compiled library (`src/adf.dll`, `cygadf.dll` for CygWin) is copied
  to the directories with test binaries (it probably can be fixed and done
  automatically but no time for it now; if sb has good solution for this,
  help welcomed).
  For the `x64-Debug-Shared` configuration, also the library with the address
  sanitizer:
  `Microsoft\VisualStudio\2019\Community\VC\Tools\MSVC\14.29.30133\bin\Hostx64\x64\clang_rt.asan_dbg_dynamic-x86_64.dll`
  has to be copied.

  (see also [a related Stack overflow post](https://stackoverflow.com/questions/66531482/application-crashes-when-using-address-sanitizer-with-msvc) )

## Testing with CMake

After successful building (see above), automatic tests can be started with:
- `$ util/cmake_debug_test `  (to test debug version)
- `$ util/cmake_shared_test`  (for the shared library version)
- `$ util/cmake_static_test`  (for the static library version)


## Testing with Autotools

After successful building (see above), automatic tests can be build
and started with:
```
  $ make check
```

## Testing with Visual Studio

"Run CTests".

Note that:
- tests using shell scripts does not work - they require `sh` / `bash`
  (can be installed and configured but not a standard thing on Windows...)


## Installation

... was not much used / tested. Except for `deb` packages, which build and
install fine on current Debian stable and on Ubuntu in the CI system (GitHub
Actions).

Basic checks are done by the CI system for CMake and autotools on Linux
(Debian and Ubuntu) and MacOS, so those _should_ work fine.


## Installation with CMake

(Note that this was not much used / tested!)

To default location:
```
$ util/cmake_release_install
```

To a custom location:
```
$ util/cmake_release_install [custom_prefix]
```

To install one version of the library (static or shared):
```
$ util/cmake_shared_install
$ util/cmake_static_install
```

## Installation with autotools

To a configured location:
```
$ make install
```

To change default location, do:
```
$ ./configure --help
```
and look for prefix options.


## Installation with Visual Studio

Find produced DLL and EXE and copy wherever you want...
(btw. not sure what CMake install does on Windows...)


## Cross-compilation examples

1. With CMake:

Building for Windows can be done cross-compiling eg. on Linux with MinGW:
```
  $ CC=x86_64-w64-mingw32-gcc util/cmake_shared_configure
```

(or
```
  $ CC=i686-w64-mingw32-gcc util/cmake_shared_configure
```
to compile 32-bit version)

or just use:
```
  $ util/cmake_shared_configure_cross_mingw
```
and then, as usual:
```
  $ util/cmake_shared_build
```

Note that the tests will not work for rather obvious reasons - `.exe` and `.dll`
files are not for Linux...
Most likely, it is possible to run the tests eg. using some Windows emulation
like Wine. The executables should work of course, but the testing suites will
require eg. configuring Windows CMake or tinkering test configuration to use
Wine for execution etc...
It should rather be tested and used on the native target system.

2. With autotools:
```
  $ ./autogen.sh
  $ mkdir build
  $ cd build
  $ ../configure --host=x86_64-w64-mingw32
  $ make
```

Links
------
- [CMake](https://cmake.org/)
- [GNU Autotools](https://en.wikipedia.org/wiki/GNU_Autotools) (and links there)
- [Check testing framework](https://libcheck.github.io/check/)
