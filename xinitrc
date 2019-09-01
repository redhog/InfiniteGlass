export GLASS_SHADER_PATH=build
. ./build/env/bin/activate
glass-input &
glass-widgets &
glass-ghosts &
glass-animator &
gdbserver :2048 ./build/glass-renderer &
xterm
