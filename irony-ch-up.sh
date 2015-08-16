#!/bin/sh
tvtime-command CHANNEL_INC 2> /dev/null
ps -C totem | grep -q totem && totem --seek-fwd
