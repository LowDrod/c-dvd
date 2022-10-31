#!/bin/sh

cc -o ./screensaver ./screensaver.c && \
    ./screensaver -p ./dvd.txt -s 25
