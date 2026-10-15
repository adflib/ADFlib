
AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = -std=c99 -Wall -Wextra -pedantic \
    -Werror-implicit-function-declaration \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/examples

check_PROGRAMS = \
    test_unadf_path

TESTS = $(check_PROGRAMS)

ADFLIBS = $(top_builddir)/src/libadf.la

test_unadf_path_SOURCES = test_unadf_path.c \
    $(top_srcdir)/examples/unadf_arg.c \
    $(top_srcdir)/examples/unadf_arg.h \
    $(top_srcdir)/examples/unadf_path.c \
    $(top_srcdir)/examples/unadf_path.h \
    $(top_srcdir)/examples/unadf_util.c \
    $(top_srcdir)/examples/unadf_util.h
test_unadf_path_CFLAGS = $(CHECK_CFLAGS)
test_unadf_path_LDADD = $(ADFLIBS) $(CHECK_LIBS)
test_unadf_path_DEPENDENCIES = $(top_builddir)/src/libadf.la
