#!/bin/sh
ps -C tvtime | grep -q tvtime && exit 0
killall totem
killall mplayer
tvtime --fullscreen &
