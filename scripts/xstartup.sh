#! /bin/bash


[ "$XSERVER" ] || XSERVER=Xephyr

if ! [ "$XSERVEROPTS" ]; then
  if [ "$XSERVER" = "Xephyr" ]; then
    XSERVEROPTS=":100 -ac -screen 1280x768x24 -host-cursor -extension MIT-SHM -nolisten tcp"
  else
    XSERVEROPTS=":100 -ac"
  fi
fi

XSERVERPATH="$(whereis -b "$XSERVER" | cut -f2 -d' ')"

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
