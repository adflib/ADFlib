
include_directories (
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_BINARY_DIR}/src
)

#check_symbol_exists ( chmod "unistd.h" HAVE_GETOPT )

#if ( "${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC" )
if ( ${HAVE_GETOPT} )
  set ( ADFLIB_CUSTOM_GETOPT_SRC "")
  message ( STATUS "Using system's getopt implementation." )
else()
  set ( ADFLIB_CUSTOM_GETOPT_SRC
    getopt.c
    getopt.h )
  message ( STATUS "Using custom getopt implementation." )
  #message ( STATUS "ADFLIB_CUSTOM_GETOPT_SRC: ${ADFLIB_CUSTOM_GETOPT_SRC}" )
endif()

add_executable ( adfbitmap adfbitmap.c )
target_link_libraries ( adfbitmap adf )
set_target_properties ( adfbitmap PROPERTIES
  INSTALL_RPATH "/usr/local/lib"
)

add_executable ( adfformat
  adfformat.c
  ${ADFLIB_CUSTOM_GETOPT_SRC}
)
target_link_libraries ( adfformat adf )
set_target_properties ( adfformat PROPERTIES
  INSTALL_RPATH "/usr/local/lib"
)

add_executable( adfimgcreate
  adfimgcreate.c
  pathutils.c
  pathutils.h
  ${ADFLIB_CUSTOM_GETOPT_SRC}
)
target_link_libraries ( adfimgcreate adf )
set_target_properties ( adfimgcreate PROPERTIES
  INSTALL_RPATH "/usr/local/lib"
)

add_executable ( adfinfo
  adfinfo.c
  adfinfo_common.c
  adfinfo_common.h
  adfinfo_dir.c
  adfinfo_dir.h
  adfinfo_dircache.c
  adfinfo_dircache.h
  adfinfo_file.c
  adfinfo_file.h
  adfinfo_hd.c
  adfinfo_hd.h
  adfinfo_link.c
  adfinfo_link.h
  adfinfo_volume.c
  adfinfo_volume.h
  common.c
  common.h
  pathutils.c
  pathutils.h
)
target_link_libraries ( adfinfo adf )
set_target_properties ( adfinfo PROPERTIES
  INSTALL_RPATH "/usr/local/lib"
)

add_executable ( adfls
  adfls.c
  common.c
  common.h
  pathutils.c
  pathutils.h
  ${ADFLIB_CUSTOM_GETOPT_SRC}
)
target_link_libraries ( adfls adf )
set_target_properties ( adfls PROPERTIES
  INSTALL_RPATH "/usr/local/lib"
)

add_executable ( adfsalvage
  adfsalvage.c
  ${ADFLIB_CUSTOM_GETOPT_SRC}
)
target_link_libraries ( adfsalvage adf )
set_target_properties ( adfsalvage PROPERTIES
  INSTALL_RPATH "/usr/local/lib"
)

add_executable ( unadf
  unadf.c
  unadf_arg.c
  unadf_arg.h
  unadf_extract.c
  unadf_extract.h
  unadf_fs.c
  unadf_fs.h
  unadf_info.c
  unadf_info.h
  unadf_pathutil.c
  unadf_pathutil.h
  unadf_util.c
  unadf_util.h
  unadf_win32.c
  unadf_win32.h
)
target_link_libraries ( unadf adf )
set_target_properties ( unadf PROPERTIES
  #  INSTALL_RPATH "${CMAKE_INSTALL_LIBDIR}"
  INSTALL_RPATH "/usr/local/lib"
)

install ( TARGETS
  adfbitmap
  adfformat
  adfimgcreate
  adfinfo
  adfls
  adfsalvage
  unadf
)
