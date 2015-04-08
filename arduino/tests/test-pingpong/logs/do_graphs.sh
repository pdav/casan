#!/bin/bash


for i in 11 15 20 25
do
    cat $i | ./cdf > $i.cdf
done
