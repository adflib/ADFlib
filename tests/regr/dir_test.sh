#!/bin/sh

# common settings
. "`dirname $0`/common.sh"

set -e

#echo "----- dir_test"
test_file=dir_test_testffs_adf
cat $FFSDUMP > ${test_file}
./dir_test ${test_file}
rm -v ${test_file}
