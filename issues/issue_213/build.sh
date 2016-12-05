#!/bin/bash
rm -f run
gcc reproduce.c -lczmq -lmlm -o run
