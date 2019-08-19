xterm &
#xclock &
. ./build/env/bin/activate
python3 input.py &
python3 widgets.py &
python3 ghosts.py &
python3 animator.py &
gdbserver :2048 ./build/wm &
while sleep 1; do
  :
done
