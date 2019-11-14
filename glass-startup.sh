#! /bin/bash

exec > /tmp/glass.log 2>&1

TMPFILE="$(tempfile)"
FIFO="$TMPFILE.fifo"

mkfifo "$FIFO"

glass-ghosts | { read x; echo $x > "$FIFO"; cat; } &

export SESSION_MANAGER="$(cat $FIFO)"

glass-input &
glass-theme &
glass-widgets &
glass-animator &
glass-renderer &

xterm

rm "$FIFO" "$TMPFILE"
