#!/bin/sh

TEST_PROGS=`find Source/test/ -name test_* -executable -type f`

total_passed=0
total_failed=0
total_crashed=0

test_failed=""
for x in $TEST_PROGS; do
    ./$x
    if test "$?" -eq 0; then
        total_passed=$((total_passed + 1))
    else
        total_failed=$((total_failed + 1))
        test_failed="$x $test_failed"
    fi
done

total=$((total_passed + total_failed + total_crashed))

echo "#######"
echo "# Tests run:        $total"
echo "# Passed:           $total_passed"
echo "# Failed:           $total_failed"
echo "# Crashed:          $total_crashed"
echo "#######"

if test $total_failed -ne 0; then
    echo "Crashed tests:"
    for x in $test_failed; do
        echo $x
    done
fi

exit 0
