#! /bin/bash
echo ">>> Building contract..."
if [ ! -d "$PWD/build" ]
then
  mkdir build
fi
eosio-cpp -abigen -I="./include/" -R="./resources" -o="./build/rng.oracle.wasm" -contract="rngoracle" ./src/rng.oracle.cpp