xterm &
#xclock &
./build/widgets \
  IG_LAYER=IG_LAYER_OVERLAY \
  IG_X=0.0 IG_Y=0.70 \
  DISPLAYSVG=@fontawesome-free-5.9.0-desktop/svgs/solid/search-plus.svg \
  IG_ZOOM IG_LAYER_DESKTOP 0.9090909090909091 &
./build/widgets \
  IG_LAYER=IG_LAYER_OVERLAY \
  IG_X=0.0 IG_Y=0.60 \
  DISPLAYSVG=@fontawesome-free-5.9.0-desktop/svgs/solid/search-minus.svg \
  IG_ZOOM IG_LAYER_DESKTOP 1.1 &
gdbserver :2048 ./build/wm
