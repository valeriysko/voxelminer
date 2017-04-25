#!/bin/sh

emcc -O3 \
     -s ALLOW_MEMORY_GROWTH=1 \
     -s WASM=1 \
     --bind \
     -o resources/public/js/native.js \
     chunks.cpp

# copy files to main webroot dir
cp resources/public/js/native.* resources/public/
