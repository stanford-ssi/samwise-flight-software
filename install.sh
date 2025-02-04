#!/bin/bash

find /dev -regextype sed -regex ".*/sd[a-z]\+[0-9]" -print | head -n 1 - | xargs -I "{}" sudo mount '{}' /mnt/pycubed
sudo cp build/src/samwise.uf2 /mnt/pycubed
sync
