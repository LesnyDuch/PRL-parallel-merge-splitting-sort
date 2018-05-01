#!/bin/bash

numbers=$1;
pcs=$2;

mpic++ --prefix /usr/local/share/OpenMPI -o sort_them merge-splitting.cpp > /dev/null

dd if=/dev/random bs=1 count=$numbers of=numbers status=none

mpirun --prefix /usr/local/share/OpenMPI -np $pcs sort_them $numbers

rm -f sort_them numbers