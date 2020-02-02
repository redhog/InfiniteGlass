#! /bin/bash

xauth nlist :0 | sed -e 's/^..../ffff/' > /tmp/.docker.xauth
docker run -ti -v /tmp/.X11-unix:/tmp/.X11-unix -v /tmp/.docker.xauth:/tmp/.docker.xauth -e XAUTHORITY=/tmp/.docker.xauth -e DISPLAY=:0 glass:0.0.1 /bin/bash -c "su -c \"bash -c 'cd /InfiniteGlass; make run'\" glass"

