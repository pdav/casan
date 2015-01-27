#/bin/bash
TESTS=`ls tests/*.txt`
DOCTEST_FLAGS=-v
for FILE in $TESTS
do
    python3 -m doctest $DOCTEST_FLAGS $FILE
done
