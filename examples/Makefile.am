
AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/src/$(NATIVE_DIR)

bin_PROGRAMS = \
	adfbitmap \
	adfformat \
	adfimgcreate \
	adfinfo \
	adfsalvage \
	unadf

ADFLIBS = $(top_builddir)/src/libadf.la


adfbitmap_SOURCES = adfbitmap.c
adfbitmap_LDADD = $(ADFLIBS)
adfbitmap_DEPENDENCIES = $(top_builddir)/src/libadf.la

adfformat_SOURCES = adfformat.c
adfformat_LDADD = $(ADFLIBS)
adfformat_DEPENDENCIES = $(top_builddir)/src/libadf.la

adfimgcreate_SOURCES = adfimgcreate.c \
  pathutils.c \
  pathutils.h
adfimgcreate_LDADD = $(ADFLIBS)
adfimgcreate_DEPENDENCIES = $(top_builddir)/src/libadf.la

adfinfo_SOURCES = adfinfo.c \
  adfinfo_common.c \
  adfinfo_common.h \
  adfinfo_dir.c \
  adfinfo_dir.h \
  adfinfo_dircache.c \
  adfinfo_dircache.h \
  adfinfo_file.c \
  adfinfo_file.h \
  adfinfo_hd.c \
  adfinfo_hd.h \
  adfinfo_volume.c \
  adfinfo_volume.h \
  pathutils.c \
  pathutils.h
adfinfo_LDADD = $(ADFLIBS)
adfinfo_DEPENDENCIES = $(top_builddir)/src/libadf.la

adfsalvage_SOURCES = adfsalvage.c
adfsalvage_LDADD = $(ADFLIBS)
adfsalvage_DEPENDENCIES = $(top_builddir)/src/libadf.la

unadf_SOURCES = unadf.c
unadf_LDADD = $(ADFLIBS)
unadf_DEPENDENCIES = $(top_builddir)/src/libadf.la
