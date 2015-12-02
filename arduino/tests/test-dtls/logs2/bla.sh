#!/bin/bash

F1="dtls.0"
F2="dtls-simplified.0"

cat $F1 | grep Encrption | head -n 1200 | sed "s/Encrption took : //" > $F1.enc
cat $F1 | grep Decrption | head -n 1200 | sed "s/Decrption took : //" > $F1.dec
cat $F1 | grep duration | head -n 1200 | sed "s/duration: //" > $F1.dur

cat $F2 | grep Encrption | head -n 1200 | sed "s/Encrption took : //" > $F2.enc
cat $F2 | grep Decrption | head -n 1200 | sed "s/Decrption took : //" > $F2.dec
cat $F2 | grep duration | head -n 1200 | sed "s/duration: //" > $F2.dur
