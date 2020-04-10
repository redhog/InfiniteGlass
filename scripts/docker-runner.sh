#! /bin/bash

echo "$(ip -4 addr show docker0 | grep -Po 'inet \K[\d.]+') $(hostname)" >> /etc/hosts

su -c "bash -c 'cd /InfiniteGlass; make run'" glass
