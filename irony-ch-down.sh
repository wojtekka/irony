#!/bin/sh
tvtime-command CHANNEL_DEC 2> /dev/null
ps -C totem | grep -q totem && totem --seek-bwd
