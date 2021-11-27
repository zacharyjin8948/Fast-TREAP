#!/usr/bin/env bash

for i in 0 1 2 3;
do
    sudo umount /pmem$i;
done

sudo ~blepers/test_pmem/stripped.pl /dev/pmem0 /dev/pmem1 /dev/pmem2 /dev/pmem3

