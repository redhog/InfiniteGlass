#! /bin/bash

if [ "$GLASS_DEBUGGER" = "valgrind" ]; then
  echo "Debugging using Valgrind..."
  exec valgrind --vgdb=yes $(which glass-renderer)
else
  if [ "$GLASS_DEBUGGER" = "gdb" ]; then
    echo "Debugging using GDB..."
    exec gdb $(which glass-renderer) 0<&3 1>&4 2>&5
  else
    echo "Starting renderer without debugger"
    exec $(which glass-renderer)
  fi
fi
