#! /bin/bash

grep GLASS_DEBUG.renderer | sed -e "s+:.*++g" | sort | uniq | tr . _
