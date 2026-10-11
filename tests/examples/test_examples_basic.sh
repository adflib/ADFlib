#!/bin/sh
#
# A simple test of command-line utilities.
#
# It tries to execute them with some parameters
# and shows the outputs (it does no further checks),
# fails if the execution is not possible.
#

basedir=$(dirname $0)

set -e

if [ $# -gt 1 ]
then
    echo 2>&1 "Usage: $0 [bin install path]"
    exit 1
fi

if [ $# -eq 1 ]
then
    examples_bin="$1"
fi

. ${basedir}/common.sh

test_adf=${dumps_dir}/arccsh.adf

if [ ! -f "${test_adf}" ]
then
    echo "Test data not available ( ${test_adf} )."
    exit 1
fi

#[ -n "$srcdir" ] && test_adf="$srcdir/arccsh.adf"

adfbitmap=$(get_test_cmd adfbitmap)
adfimgcreate=$(get_test_cmd adfimgcreate)
adfformat=$(get_test_cmd adfformat)
adfinfo=$(get_test_cmd adfinfo)
unadf=$(get_test_cmd unadf)


run_cmd()
{
    echo "============================================================="
    #stdbuf -oL
    if [ "${OSTYPE}x" = "msysx" ]
    then
	local EXEC=${1}.exe
    else
	local EXEC=${1}
    fi
    shift
    echo "executing:  ${EXEC} $@"
    #${EXAMPLES_PATH}/${EXEC} $@
    ${EXEC} $@
}

#cmds[0]="$unadf -r $test_adf"
#cmds[1]="$adfinfo $test_adf"
#cmds[2]="$adfinfo $test_adf 0"
#mds[2]="$adfinfo $test_adf 0 CSH"
#mds[3]="$adfinfo $test_adf 0 c/"
#cmds[4]="$adfinfo $test_adf 0 l"
#cmds[5]="$adfimgcreate -t dd testflopdd1.adf"
#cmds[6]="$adfformat -t 1 -l TestFlopDD1 testflopdd1.adf"
#cmds[7]="$adfinfo testflopdd1.adf"
#cmds[8]="rm -v testflopdd1.adf"
#cmds[9]="$adfbitmap show $test_adf"

#for cmd in "${cmds[@]}"
#do
#run_cmd ${cmd}
#done

for cmd in \
    "$unadf -r $test_adf" \
    "$adfinfo $test_adf" \
    "$adfinfo $test_adf 0" \
    "$adfinfo $test_adf 0 CSH" \
    "$adfinfo $test_adf 0 c/" \
    "$adfinfo $test_adf 0 l" \
    "$adfimgcreate -t dd testflopdd1.adf" \
    "$adfformat -t 1 -l TestFlopDD1 testflopdd1.adf" \
    "$adfinfo testflopdd1.adf" \
    "rm -v testflopdd1.adf" \
    "$adfbitmap show $test_adf"
do
#   echo $cmd
    run_cmd ${cmd}
done
