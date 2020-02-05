#! /bin/bash

IMAGE=redhogorg/glass:0.0.2

if [ "$(docker images -q $IMAGE)" == "" ]; then
  docker build -t $IMAGE .
fi

mkdir -p ~/.config/glass
xauth nlist :0 | sed -e 's/^..../ffff/' > /tmp/.docker.xauth
docker run -ti -v ~/.config/glass:/home/glass/.config/glass -v /tmp/.X11-unix:/tmp/.X11-unix -v /tmp/.docker.xauth:/tmp/.docker.xauth -e XAUTHORITY=/tmp/.docker.xauth -e DISPLAY=:0 $IMAGE /bin/bash -c "su -c \"bash -c 'cd /InfiniteGlass; make run'\" glass"

