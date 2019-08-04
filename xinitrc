xterm &
#xclock &
./build/widgets \
  IG_LAYER=IG_LAYER_OVERLAY \
  IG_X=0.01 IG_Y=0.70 IG_W=0.05 IG_H=0.05 \
  DISPLAYSVG=@fontawesome-free-5.9.0-desktop/svgs/solid/search-plus.svg \
  IG_ZOOM IG_LAYER_DESKTOP 0.9090909090909091 &
./build/widgets \
  IG_LAYER=IG_LAYER_OVERLAY \
  IG_X=0.01 IG_Y=0.65 IG_W=0.05 IG_H=0.05 \
  DISPLAYSVG=@fontawesome-free-5.9.0-desktop/svgs/solid/search-minus.svg \
  IG_ZOOM IG_LAYER_DESKTOP 1.1 &
./build/widgets \
  IG_LAYER=IG_LAYER_OVERLAY \
  IG_X=0.01 IG_Y=0.60 IG_W=0.05 IG_H=0.05 \
  DISPLAYSVG=@fontawesome-free-5.9.0-desktop/svgs/solid/search-location.svg \
  IG_ZOOM IG_LAYER_DESKTOP -1.0 &
./build/widgets \
  IG_LAYER=IG_LAYER_OVERLAY \
  IG_X=0.01 IG_Y=0.55 IG_W=0.05 IG_H=0.05 \
  DISPLAYSVG=@fontawesome-free-5.9.0-desktop/svgs/solid/eject.svg \
  IG_EXIT &
gdbserver :2048 ./build/wm &
while sleep 1; do
  :
done
