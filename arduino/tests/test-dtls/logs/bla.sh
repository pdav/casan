#!/bin/bash

F1="screenlog-dtls.0"
F2="screenlog-simplified.0"

cat $F1 | grep Encryption | sed "s/Encryption took : //" > $F1.enc
cat $F1 | grep Decryption | sed "s/Decryption took : //" > $F1.dec
cat $F1 | grep duration | sed "s/duration: //" > $F1.dur

cat $F2 | grep Encryption | sed "s/Encryption took : //" > $F2.enc
cat $F2 | grep Decryption | sed "s/Decryption took : //" > $F2.dec
cat $F2 | grep duration | sed "s/duration: //" > $F2.dur
