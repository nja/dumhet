TESTS_LOG=tests/tests.log

echo "Running unit tests:"

echo -n > ${TESTS_LOG}

for i in tests/*_tests
do
    if test -f $i
    then
	if $VALGRIND ./$i 2>> ${TESTS_LOG}
	then
	    echo $i PASS
	else
	    echo "ERROR in test $i: here's ${TESTS_LOG}"
	    echo "------"
	    tail ${TESTS_LOG}
	    exit 1
	fi
    fi
done

echo ""
