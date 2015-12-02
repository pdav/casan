#!/bin/bash

for i in r_scr*
do
    R --no-save < "${i}"
    [ -e Rplots.pdf ] && mv Rplots.pdf "${i}.pdf"
done
