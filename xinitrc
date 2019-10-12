#! /bin/bash

export GLASS_SHADER_PATH=build
export MALLOC_CHECK_=2

if [ "$GLASS_DMALLOC" ]; then
  eval "$(dmalloc -l ./logfile -i 100 high)"
fi
  
. ./build/env/bin/activate

session_content() {
    
  glass-input 2>&1 | sed -e "s+^+INPUT: +g" &
  glass-widgets 2>&1 | sed -e "s+^+WIDGETS: +g" &
  glass-animator 2>&1 | sed -e "s+^+ANIMATOR: +g" &
  glass-wintest 2>&1 | sed -e "s+^+WINTEST: +g" &
  
  if [ "$GLASS_DEBUGGER" = "valgrind" ]; then
    echo "Debugging using Valgrind..."
    valgrind --vgdb=yes ./build/glass-renderer 2>&1 | sed -e "s+^+RENDERER: +g" &
  else
    if [ "$GLASS_DEBUGGER" = "gdb" ]; then
      echo "Debugging using GDB..."
      gdbserver :2048 ./build/glass-renderer 2>&1 | sed -e "s+^+RENDERER: +g" &
    else
      echo "Starting renderer without debugger"
      ./build/glass-renderer 2>&1 | sed -e "s+^+RENDERER: +g" &
    fi
  fi
  xterm
  exit 0
}

if [ "$GLASS_NOGHOSTS" ]; then
  session_content
else
  glass-ghosts | { read SESSION_MANAGER; export SESSION_MANAGER; cat & session_content; } 2>&1 | sed -e "s+^+GHOSTS: +g"
fi
