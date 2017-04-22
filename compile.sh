#!/bin/sh

emcc -O2 \
     -s ALLOW_MEMORY_GROWTH=1 \
     --bind \
     -o resources/public/js/native.js \
     chunks.cpp

# copy memory initialization file to main webroot dir
cp resources/public/js/native.js.mem resources/public/
