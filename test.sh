#!/bin/bash

# Trigger all your test cases with this script

#! /usr/bin/env sh
echo "##########################"
echo "### Running e2e tests! ###"
echo "##########################"
count=0 # number of test cases run so far

# Assume all `.in` and `.out` files are located in a separate `tests` directory


for test in Tests/*.in; do
    name=$(basename $test .in)
    expected=Tests/$name.out
    args=Tests/$name.in

    echo running $test
    xargs -a $args -I % ./test % 2>&1 | diff - $expected || echo "Test $name: failed!"


    count=$((count+1))
done

echo "Finished running $count tests!"