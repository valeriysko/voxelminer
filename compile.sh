#!/bin/sh

emcc --bind \
     -o resources/public/js/native.js \
     chunks.cpp
