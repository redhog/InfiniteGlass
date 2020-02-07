#! /bin/bash

if [ "$GLASS_DEBUGGER" = "valgrind" ]; then
  echo "Debugging using Valgrind..."
  exec valgrind --vgdb=yes ./build/glass-renderer
else
  if [ "$GLASS_DEBUGGER" = "gdb" ]; then
    echo "Debugging using GDB..."
    exec gdbserver :2048 ./build/glass-renderer
  else
    echo "Starting renderer without debugger"
    exec ./build/glass-renderer
  fi
fi
