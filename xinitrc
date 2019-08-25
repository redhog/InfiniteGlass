xterm &
#xclock &
. ./build/env/bin/activate
glass-input &
glass-widgets &
glass-ghosts &
glass-animator &
gdbserver :2048 ./build/glass-renderer &
while sleep 1; do
  :
done
