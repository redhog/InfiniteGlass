#! /bin/bash

IMAGE=redhogorg/glass:0.0.2

if [ "$1" == "clean" ]; then
  docker rm glass
  docker rmi -f "$IMAGE"
  exit 0
fi

if [ "$(docker images -q $IMAGE)" == "" ]; then
  docker build -t $IMAGE .
fi

mkdir -p ~/.config/glass
xauth nlist :0 | sed -e 's/^..../ffff/' > /tmp/.docker.xauth

if [ ! "$(docker ps -a -q -f name=glass)" ]; then
  docker run \
         --name glass \
         --memory 2gb \
         -ti \
         --net=host \
         --ipc=host \
         --user id -u root \
         --cap-add=ALL \
         -v ~/.config/glass:/home/glass/.config/glass \
         -v /tmp/.X11-unix:/tmp/.X11-unix:rw \
         -v /tmp/.ICE-unix:/tmp/.ICE-unix:rw \
         -v /tmp/.docker.xauth:/tmp/.docker.xauth \
         -e XAUTHORITY=/tmp/.docker.xauth \
         -e DISPLAY=:0 \
         $IMAGE
else
    docker start -a -i glass
fi
