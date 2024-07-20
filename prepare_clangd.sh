#!/bin/sh

cat > .clangd <<EOF
CompileFlags:
  Add: [-xc, -I$(pwd)/include/, -std=c89]
  RemapCxx: [.cpp, .cc, .cxx]
  RemapC: [.c]
EOF
