#! /bin/bash

export GLASS_SHADER_PATH=build
export MALLOC_CHECK_=2

if [ "$GLASS_DMALLOC" ]; then
  eval "$(dmalloc -l ./logfile -i 100 high)"
fi
  
. ./build/env/bin/activate

if ! [ "$GLASS_NOGHOSTS" ]; then
  TMPFILE="$(tempfile)"
  FIFO="$TMPFILE.fifo"

  mkfifo "$FIFO"

  glass-ghosts | { read x; echo $x > "$FIFO"; cat; } &

  export SESSION_MANAGER="$(cat $FIFO)"
fi

glass-input 2>&1 &
glass-theme 2>&1 &
glass-widgets 2>&1 &
glass-animator 2>&1 &
glass-wintest 2>&1 &

if [ "$GLASS_DEBUGGER" = "valgrind" ]; then
  echo "Debugging using Valgrind..."
  valgrind --vgdb=yes ./build/glass-renderer &
else
  if [ "$GLASS_DEBUGGER" = "gdb" ]; then
    echo "Debugging using GDB..."
    gdbserver :2048 ./build/glass-renderer &
  else
    echo "Starting renderer without debugger"
    ./build/glass-renderer &
  fi
fi

xterm

