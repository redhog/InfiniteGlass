#! /bin/bash

exec > /tmp/glass.log 2>&1

session_content() {
    glass-input &
    glass-theme &
    glass-widgets &
    glass-animator &
    glass-renderer &
    xterm
}

glass-ghosts | { read SESSION_MANAGER; export SESSION_MANAGER; session_content; } 2>&1 | tee ~/.glass-log.txt 
