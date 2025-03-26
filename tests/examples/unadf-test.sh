#!/bin/sh
basedir=`dirname "$0"`
. $basedir/common.sh

UNADF=`get_test_cmd unadf`

# check no args
check_no_args() {
    $( $UNADF >$actual 2>&1 || exit 0; )
    compare_with "check no args" unadf_no_args
}

check_help() {
    $UNADF -h >$actual
    compare_with "check help" unadf_help
}

# check invalid args
check_invalid_args() {
    $( $UNADF -v >$actual 2>&1 || exit 0; )
    compare_with "check invalid arg for -v" unadf_invalid_args_1

    $( $UNADF -d >$actual 2>&1 || exit 0; )
    compare_with "check invalid arg for -d" unadf_invalid_args_2
}

# -l (list root directory) option
check_list_root_directory_option() {
    $UNADF -l "$DUMPS_DIR/arccsh.adf" >$actual 2>/dev/null
    compare_with "-l (list root directory) option" \
        unadf_list_root_directory_option
}

# -r (list entire disk) option
check_list_entire_disk_option() {
    $UNADF -r "$DUMPS_DIR/arccsh.adf" >$actual 2>/dev/null
    compare_with "-r (list entire disk) option" unadf_list_entire_disk_option
}

# -s (show logical block pointer) option
check_show_logical_block_pointer_option() {
    $UNADF -ls "$DUMPS_DIR/arccsh.adf" >$actual 2>/dev/null
    compare_with "-s (show logical block pointer) option" \
        unadf_show_logical_block_pointer_option
}
# TODO -c (use dircache data) option
# TODO -m (display file comments) option
# TODO -v (mount different volume) option

# -d (extract to dir)
check_extract_to_dir() {
    $UNADF -d $tmpdir/x "$DUMPS_DIR/arccsh.adf" >$actual 2>/dev/null
    compare_with "-d (extract to dir)" unadf_extract_to_dir

# check permissions were set on extracted files
# (these tests are failing on MSYS2:
#  -> either the way of checking or permissions are not valid)
    if [ "x${host_type}" != 'xMINGW32' -a \
         "x${host_type}" != 'xMINGW64' ]
    then
	[ -x $tmpdir/x/CSH ] || { echo "Invalid x/CSH permissions" ;
				  echo failed >$status ; }
	[ -x $tmpdir/x/c/LZX ] || { echo "Invalid x/c/LZX permissions" ;
				    echo failed >$status ; }
    fi
}

# -d (extract to dir) with specific files, not all their original case
check_extract_to_dir_with_specific_files() {
    $UNADF -d $tmpdir/x "$DUMPS_DIR/arccsh.adf" csh s/startup-sequence \
        devs/system-configuration >$actual 2>/dev/null
    compare_with "-d (extract to dir) with specific files, not all their original case" \
        unadf_extract_to_dir_with_specific_files
}
# TODO check permissions were set (bug: currently they aren't)

# -p (extract to pipe) option
check_extract_to_pipe_option() {
    $UNADF -p "$DUMPS_DIR/arccsh.adf" s/startup-sequence >$actual 2>/dev/null
    compare_with " -p (extract to pipe) option" unadf_extract_to_pipe_option
}

# -w (mangle win32 filenames) option
check_mangle_win32_filenames_option() {
    $UNADF -d $tmpdir/x -w "$DUMPS_DIR/win32-names.adf" >$actual 2>/dev/null
    compare_with "-w (mangle win32 filenames) option" \
        unadf_mangle_win32_filenames_option
}

# confirm the mangling (-w) only occurs on extraction
check_confirm_the_mangling() {
    $UNADF -r -w "$DUMPS_DIR/win32-names.adf" >$actual 2>/dev/null
    compare_with "confirm the mangling (-w) only occurs on extraction" \
        unadf_confirm_the_mangling
}


check_no_args
check_help
check_invalid_args
check_list_root_directory_option
check_list_entire_disk_option
check_show_logical_block_pointer_option
check_extract_to_dir
check_extract_to_dir_with_specific_files
check_extract_to_pipe_option
check_mangle_win32_filenames_option
check_confirm_the_mangling

read status < $status && test "x$status" = xsuccess
echo "status: $status"
