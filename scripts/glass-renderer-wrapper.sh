#! /bin/bash

if [ "$GLASS_DEBUGGER" = "valgrind" ]; then
  echo "Debugging using Valgrind..."
  exec valgrind --vgdb=yes ./build/glass-renderer
else
  if [ "$GLASS_DEBUGGER" = "gdb" ]; then
    echo "Debugging using GDB..."
    exec gdb ./build/env/bin/glass-renderer 0<&3 1>&4 2>&5
  else
    echo "Starting renderer without debugger"
    exec ./build/env/bin/glass-renderer
  fi
fi
