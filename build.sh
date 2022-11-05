#! /bin/bash
echo ">>> Building contract..."
if [ ! -d "$PWD/build" ]
then
  mkdir build
fi
eosio-cpp -I="./include/" -R="./resources" -o="./build/rng.oracle.wasm" -contract="rng.oracle" -abigen ./src/rng.oracle.cpp