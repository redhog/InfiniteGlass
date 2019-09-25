#! /bin/bash

export GLASS_SHADER_PATH=build
export MALLOC_CHECK_=2

if [ "$GLASS_DMALLOC" ]; then
  eval "$(dmalloc -l ./logfile -i 100 high)"
fi
  
. ./build/env/bin/activate

session_content() {
    
  glass-input &
  glass-widgets &
  glass-animator &
  glass-wintest &

  sleep 5
  
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
  exit 0
}

if [ "$GLASS_NOGHOSTS" ]; then
  session_content
else
  glass-ghosts | { read SESSION_MANAGER; export SESSION_MANAGER; session_content; }
fi
