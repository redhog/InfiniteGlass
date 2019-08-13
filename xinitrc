xterm &
#xclock &
. ./build/env/bin/activate
python3 input.py &
python3 client.py &
gdbserver :2048 ./build/wm &
while sleep 1; do
  :
done
