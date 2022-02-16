#! /bin/bash

exec > ~/.glass.log 2>&1

ulimit -c unlimited

if [ -e ~/.config/glass/session.sh ]; then
  source ~/.config/glass/session.sh
fi

glass-config-init
glass-session
