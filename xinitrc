#! /bin/bash

export GLASS_SHADER_PATH=build
export MALLOC_CHECK_=2

if [ "$GLASS_DMALLOC" ]; then
  eval "$(dmalloc -l ./logfile -i 100 high)"
fi
  
. ./build/env/bin/activate

glass-ghosts
