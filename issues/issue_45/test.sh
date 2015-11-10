#!/bin/sh
#
# This script starts a performance test on malamute broker.
# it starts :
#  - a malamute broker server
#  - a consumer client
#  - a producer client

echo "Resetting test environment..."
killall -9 mlm_perf_send mlm_perf_recv
./c -l mlm_perf_send mlm_perf_recv

#echo "Starting Malamute broker..."
malamute &
sleep 1

echo "Starting receiver..."
./mlm_perf_recv &
sleep 1

echo "Running sender tests..."
./mlm_perf_send $1

echo "Stopping test processes..."
killall malamute
