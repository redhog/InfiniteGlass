#! /bin/bash

if [ "$GLASS_DEBUGGER" = gdb ]; then
  setsid xinit ./scripts/xinitrc -- "$XSERVERPATH" $XSERVEROPTS 3<&0 4>&1 5>&2 </dev/null
else
  if [ "$GLASS_DEBUGGER" = valgrind ]; then
    xinit ./scripts/xinitrc -- "$XSERVERPATH" $XSERVEROPTS &
    gdb -ex "target remote | vgdb" -ex "continue" ./$BUILD/env/bin/glass-renderer
  else
    xinit ./scripts/xinitrc -- "$XSERVERPATH" $XSERVEROPTS
  fi
fi
