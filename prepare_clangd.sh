#!/bin/sh

cat > .clangd <<EOF
CompileFlags:
  Add: [-xc, -I$(pwd)/include/, -std=c99]
  RemapCxx: [.cpp, .cc, .cxx]
  RemapC: [.c]
EOF
