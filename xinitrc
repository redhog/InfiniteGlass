#! /bin/bash

export GLASS_SHADER_PATH=build
export MALLOC_CHECK_=2

. ./build/env/bin/activate

glass-input &
glass-widgets &
glass-animator &

if ! [ "$GLASS_NOGHOSTS" ]; then
  glass-ghosts &
fi

if [ "$GLASS_DEBUG" = "valgrind" ]; then
  echo "Debugging using Valgrind..."
  valgrind --vgdb=yes ./build/glass-renderer &
else
  if [ "$GLASS_DEBUG" = "gdb" ]; then
    echo "Debugging using GDB..."
    gdbserver :2048 ./build/glass-renderer &
  else
    echo "Starting renderer without debugger"
    ./build/glass-renderer &
  fi
fi
xterm
