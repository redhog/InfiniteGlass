#! /bin/bash

if [ "$GLASS_DEBUGGER" = gdb ]; then
  xinit ./scripts/xinitrc -- "$XSERVERPATH" $XSERVEROPTS &
  gdb -ex "target remote localhost:2048" -ex "continue" ./$BUILD/env/bin/glass-renderer
else
  if [ "$GLASS_DEBUGGER" = valgrind ]; then
    xinit ./scripts/xinitrc -- "$XSERVERPATH" $XSERVEROPTS &
    gdb -ex "target remote | vgdb" -ex "continue" ./$BUILD/env/bin/glass-renderer
  else
    xinit ./scripts/xinitrc -- "$XSERVERPATH" $XSERVEROPTS
  fi
fi
