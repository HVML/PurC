#!/bin/sh

export PURC_EXECUTOR_PATH=`pwd`/lib
export PURC_DVOBJS_PATH=`pwd`/lib

SHOW_STDERR=${SHOW_STDERR:-0}
USE_VALGRIND=${USE_VALGRIND:-0}
TEST_PROGS=`find ${1:-Source/test} -name test_* -perm -0111 -type f`
VALGRIND="valgrind --leak-check=full --num-callers=100 --error-exitcode=1"
VALGRIND="${VALGRIND} --exit-on-first-error=yes"
#VALGRIND="${VALGRIND} --suppressions=/usr/lib/x86_64-linux-gnu/valgrind/default.supp"
VALGRIND="${VALGRIND} --suppressions=/usr/share/glib-2.0/valgrind/glib.supp"
VALGRIND="${VALGRIND} --suppressions=Source/valgrind/valgrind.supp"


total_passed=0
total_failed=0
total_crashed=0

test_failed=""
test_crashed=""

for x in $TEST_PROGS; do
    echo ">> Start of $x"
    if test $USE_VALGRIND -eq 0; then
        if test $SHOW_STDERR -eq 0; then
            ./$x 2> /dev/null
        else
            ./$x
        fi
    else
        ${VALGRIND} ./$x || exit
    fi
    if test "$?" -eq 0; then
        total_passed=$((total_passed + 1))
    elif test "$?" -gt 128; then
        total_crashed=$((total_crashed + 1))
        test_crashed="$x $test_crashed"
    else
        total_failed=$((total_failed + 1))
        test_failed="$x $test_failed"
    fi
    echo "<< End of $x"
    echo ""
done

total=$((total_passed + total_failed + total_crashed))

echo "#######"
echo "# Tests run:        $total"
echo "# Passed:           $total_passed"
echo "# Failed:           $total_failed"
echo "# Crashed:          $total_crashed"
echo "#######"

if test $total_failed -ne 0; then
    echo "Failed tests:"
    for x in $test_failed; do
        echo $x
    done
fi

if test $total_crashed -ne 0; then
    echo "Crashed tests:"
    for x in $test_crashed; do
        echo $x
    done
fi

exit 0
