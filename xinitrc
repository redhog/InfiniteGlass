#xlock -inwindow -mode atunnels &
xterm &
xclock -render -update 1 &

sleep 2

exec ./wm
