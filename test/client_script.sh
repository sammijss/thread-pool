#!/bin/bash

for i in {1..10}
do
    echo -n "Hello Server, I am client $i" | nc $1 $2 &
done
