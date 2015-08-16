#!/bin/sh
tvtime-command TOGGLE_FULLSCREEN 2> /dev/null
ps -C totem | grep -q totem && totem --fullscreen
