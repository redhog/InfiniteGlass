#! /bin/bash

echo "$(ip -4 addr show docker0 | grep -Po 'inet \K[\d.]+') $(hostname)" >> /etc/hosts

echo "######################################################"
export | grep GLASS
echo "######################################################"

export XDG_RUNTIME_DIR=/run/user/$(id -u)
dbus-daemon --session --fork --print-address > /tmp/dbus_address
export DBUS_SESSION_BUS_ADDRESS=$(cat /tmp/dbus_address)

su -c "bash -c 'cd /InfiniteGlass; ENVDIR=/env scripts/xstartup.sh'" glass
