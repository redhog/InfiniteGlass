#!/bin/bash

TMPFILE="$(tempfile)"
FIFO="$TMPFILE.fifo"

mkfifo "$FIFO"

{ glass-ghosts | { read x; echo $x > "$FIFO"; cat; } } 2>&1 | tee foo &

export SESSION_MANAGER="$(cat $FIFO)"

exec bash
