#! /bin/bash

exec > /tmp/glass.log 2>&1

if [ -e ~/.config/glass/session.sh ]; then
  source ~/.config/glass/session.sh
fi

glass-ghosts
