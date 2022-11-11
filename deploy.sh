#! /bin/bash
#network
if [[ "$1" == "mainnet" ]]; then
    url="https://mainnet.telos.caleos.io" # Telos Mainnet
    network="mainnet"
    account="rng.oracle"
elif [[ "$1" == "testnet" ]]; then
    url="https://testnet.telos.caleos.io" # Telos Testnet
    network="testnet"
    account="rng.oracle"
else
    url=http://127.0.0.1:8888
    network="localhost"
    account="rng.oracle"
fi

echo ">>> Deploying contract to $network..."

# eosio v1.8.0
cleos --url "$url" set contract rng.oracle "$PWD/build" rng.oracle.wasm rng.oracle.abi -p "$account"