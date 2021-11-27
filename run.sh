#!/usr/bin/env bash

# POOL_SIZE >= 14680064
# POOL_DIR must be a directory
# 8,589,934,592 Bytes = 8GBs
#export VMMALLOC_POOL_SIZE=8589934592
export VMMALLOC_POOL_SIZE=$((16*1024*1024*1024))
export VMMALLOC_POOL_DIR="/pmem4/pool"

taskset -c 20-39 make clean
taskset -c 20-39 make

#for i in {1..5}
#do
#    taskset -c 44 ./microbench 50000 100
#    sleep 5
#done
#
#for i in {1..5}
#do
#    taskset -c 44 ./microbench 100000 100
#    sleep 5
#done
#
#for i in {1..5}
#do
#    taskset -c 44 ./microbench 500000 100
#    sleep 5
#done
#
#for i in {1..5}
#do
#    taskset -c 44 ./microbench 750000 100
#    sleep 5
#done
#
#for i in {1..5}
#do
#    taskset -c 44 ./microbench 800000 100
#    sleep 5
#done
#
#for i in {1..5}
#do
#    taskset -c 66 ./microbench 1000000 100
#    sleep 5
#done
#
#for i in {1..5}
#do
#    taskset -c 44 ./microbench 1500000 100
#    sleep 5
#done
#
#for i in {1..5}
#do
#    taskset -c 44 ./microbench 2000000 100
#    sleep 5
#done

#taskset -c 0-19 ./microbenchPara 1000000 100 1
#sleep 5
#taskset -c 0-19 ./microbenchPara 1000000 100 2
#sleep 5
#taskset -c 0-19 ./microbenchPara 1000000 100 4
#sleep 5
#taskset -c 0-19 ./microbenchPara 1000000 100 8
#sleep 5
#taskset -c 0-19 ./microbenchPara 1000000 100 16
#sleep 5

taskset -c 20-39 ./microbenchPara 1000000 100 1
sleep 5
#taskset -c 20-39 ./microbenchPara 1000000 100 2
#sleep 5
#taskset -c 20-39 ./microbenchPara 1000000 100 4
#sleep 5
#taskset -c 20-39 ./microbenchPara 1000000 100 8
#sleep 5
#taskset -c 20-39 ./microbenchPara 1000000 100 16
#sleep 5

#taskset -c 22 ./microbenchPara 100 100
#sleep 5
#for i in {1..10000}
#do
#    echo "Test: $i"
#    taskset -c 20-39 ./microbenchPara 10000 100
#    sleep 2
#done
#taskset -c 20-39 ./microbenchPara 1500000 100
#sleep 5
#taskset -c 20-39 ./microbenchPara 2000000 100
#sleep 5
#taskset -c 20-39 ./microbenchPara 5000000 100
#sleep 5
#taskset -c 20-39 ./microbenchPara 10000000 100
#sleep 5
#taskset -c 20-39 ./microbenchPara 12500000 100
#sleep 5
#taskset -c 20-39 ./microbenchPara 15000000 100
#sleep 5
#taskset -c 20-39 ./microbenchPara 17500000 100
#sleep 5
#taskset -c 20-39 ./microbenchPara 20000000 100
#sleep 5
#taskset -c 20-39 ./microbenchPara 30000000 100
#sleep 5
#taskset -c 20-39 ./microbenchPara 50000000 100
#sleep 5

#taskset -c 20-39 ./microbenchPara 80000000 100
#sleep 5
#taskset -c 20-39 ./microbenchPara 100000000 100
#sleep 5

taskset -c 20-39 make clean
#rm /pmem4/ralloc_pool/*
