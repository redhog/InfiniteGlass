#xlock -inwindow -mode atunnels &
xterm &
xclock -digital  -fg red -bg blue &

sleep 2

exec ./wm
