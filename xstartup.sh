#! /bin/bash

if [ "$GLASS_DEBUG" = gdb ]; then
  xinit ./xinitrc -- "$XSERVERPATH" $XSERVEROPTS &
  gdb -ex "target remote localhost:2048" -ex "continue" ./$BUILD/glass-renderer
else
  if [ "$GLASS_DEBUG" = valgrind ]; then
    xinit ./xinitrc -- "$XSERVERPATH" $XSERVEROPTS &
    gdb -ex "target remote | vgdb" -ex "continue" ./$BUILD/glass-renderer
  else
    xinit ./xinitrc -- "$XSERVERPATH" $XSERVEROPTS
  fi
fi
