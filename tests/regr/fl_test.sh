#!/bin/sh

# common settings
. "`dirname $0`/common.sh"

set -e

#echo "----- fl_test"
test_file=fl_test_testffs_adf
cat $FFSDUMP > ${test_file}
./fl_test ${test_file} fl_test_newdev $BOOTBLK
rm -v ${test_file} fl_test_newdev
