#! /bin/bash

echo "$(ip -4 addr show docker0 | grep -Po 'inet \K[\d.]+') $(hostname)" >> /etc/hosts

echo "######################################################"
export | grep GLASS
echo "######################################################"

su -c "bash -c 'cd /InfiniteGlass; scripts/xstartup.sh'" glass
