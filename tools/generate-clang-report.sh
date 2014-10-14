#!/usr/bin/env sh
mkdir -p reports/clang
mkdir -p build_clang_scan
cd build_clang_scan
rm -rf ./*

cmake -DCMAKE_C_COMPILER=ccc-analyzer -DCMAKE_CXX_COMPILER=ccc-analyzer ..
scan-build -maxloop 20 -enable-checker alpha -o ../reports/clang/ make
