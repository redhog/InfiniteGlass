#! /bin/bash

[ "$DOCKEROS" != "" ] || DOCKEROS=ubuntu
IMAGE=redhogorg/glass-$DOCKEROS:0.0.2

if [ "$1" == "clean" ]; then
  docker rm glass
  docker rmi -f "$IMAGE"
  exit 0
fi

if [ "$(docker images -q $IMAGE)" == "" ]; then
  docker build -t $IMAGE -f Dockerfile.$DOCKEROS .
fi

XAUTH="/tmp/.docker.$(echo "$DISPLAY" | tr ":" "_").xauth"

mkdir -p ~/.config/glass
xauth nextract - $DISPLAY | XAUTHORITY=$XAUTH xauth nmerge -

if [ "$(docker ps -a -q -f name=glass)" != "" ]; then
  docker rm glass
fi

ENVS="$(export | grep GLASS_ | sed -e "s+declare -x \([^=]*\)=.*+-e \1+g")"

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
       -v "$XAUTH:$XAUTH" \
       -e "XAUTHORITY=$XAUTH" \
       -e DISPLAY \
       $ENVS \
       $IMAGE $DOCKERCOMMAND
