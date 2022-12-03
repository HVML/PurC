#!/bin/sh

USE_VALGRIND=${USE_VALGRIND:-0}
SAMPLE_PROGS=`find ${1:-Source/Samples} -perm -0111 -type f`
VALGRIND="valgrind --leak-check=full --num-callers=30 --exit-on-first-error=yes --error-exitcode=1 --suppressions=/usr/share/glib-2.0/valgrind/glib.supp --suppressions=Source/valgrind/valgrind.supp"

total_passed=0
total_failed=0
total_crashed=0

sample_failed=""
sample_crashed=""

for x in $SAMPLE_PROGS; do
    if test $USE_VALGRIND -eq 0; then
        if [[ "$x" =~ .*"layout_html".* ]]; then
             ./$x -f Source/Samples/DOMRuler/layout_html/window.html  \
                  -c Source/Samples/DOMRuler/layout_html/window.css 2> /dev/null
        else
             ./$x 2> /dev/null
        fi
    else
        if [[ "$x" =~ .*"layout_html".* ]]; then
             ${VALGRIND} ./$x -f Source/Samples/DOMRuler/layout_html/window.html  \
                          -c Source/Samples/DOMRuler/layout_html/window.css || exit
        else
             ${VALGRIND} ./$x || exit
        fi
    fi
    if test "$?" -eq 0; then
        total_passed=$((total_passed + 1))
    elif test "$?" -gt 128; then
        total_crashed=$((total_crashed + 1))
        sample_crashed="$x $sample_crashed"
    else
        total_failed=$((total_failed + 1))
        sample_failed="$x $sample_failed"
    fi
    echo "<< End of $x"
    echo ""
done

total=$((total_passed + total_failed + total_crashed))

echo "#######"
echo "# Samples run:      $total"
echo "# Passed:           $total_passed"
echo "# Failed:           $total_failed"
echo "# Crashed:          $total_crashed"
echo "#######"

if test $total_failed -ne 0; then
    echo "Failed samples:"
    for x in $sample_failed; do
        echo $x
    done
fi

if test $total_crashed -ne 0; then
    echo "Crashed samples:"
    for x in $sample_crashed; do
        echo $x
    done
fi

exit 0

