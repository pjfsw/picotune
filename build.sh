#!/bin/sh

cd target
cmake --build . && \
 picotool load -v picotune.uf2 && \
 picotool reboot

