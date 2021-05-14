# Setup instructions

This is for testnet, change to mainnet where appropriate

```
git clone https://github.com/telosnetwork/telos-oracle-rng.git testnet-rng-oracle
cd testnet-rng-oracle/oracle
npm install
sudo npm install pm2 -g
cp .env.example .env
vi .env 
#change values as needed
pm2 start index.js --name testnet-rng-oracle
```
