#! /bin/bash

exec > /tmp/glass.log 2>&1

export GLASS_SHADER_PATH=/usr/local/share/glass

session_content() {
    glass-input &
    glass-widgets &
    glass-animator &
    glass-renderer &
    xterm
}

glass-ghosts | { read SESSION_MANAGER; export SESSION_MANAGER; session_content; }


