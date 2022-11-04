#! /bin/bash
echo ">>> Building contract..."
if [ ! -d "$PWD/build" ]
then
  mkdir build
fi
eosio-cpp -I="./contracts/rng.oracle/include/" -R="./contracts/rng.oracle/resources" -o="./build/rng.oracle.wasm" -contract="rng.oracle" -abigen ./contracts/rng.oracle/src/rng.oracle.cpp