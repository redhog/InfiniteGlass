#! /bin/bash

exec > /tmp/glass.log 2>&1

export GLASS_SHADER_PATH=/usr/local/share/glass
export GLASS_ERROR_EXIT=True
export GLASS_ERROR_BAD_WINDOW=True
  
glass-input &
glass-widgets &
glass-ghosts &
glass-animator &
glass-renderer &
xterm
