# Telos RNG Consumer guide

The RNG Oracle lets you request a random number for your smart contract

## How it works

- A consumer contracts makes a request for a random number to the Oracle
- The Oracle adds the request to its request table
- Listening Oracle actors then pick up that request and 3 of them must invoke the `submitrand` action to sign and generate the random number trustlessly
- The invocation by the third Oracle actor also invokes the `receiverand` callback action of the consumer contract, sending back the generated random number to the consumer

## Implement the callback

Your smart contract will need to implement a `receiverand(uint64_t caller_id, checksum256 random)` callback action that the Oracle will call once it has generated the number.

See our [Oracle Consumer contract](https://github.com/telosnetwork/rng-consumer) for a complete example of an implementation

## Make a request

To make a request the consumer must call the `requestrand(uint64_t caller_id, uint64_t seed, const name &caller)` action

- **caller_id**: your internal id for the request, so that you can keep track of what random numbers you receive correspond to
- **seed**: the seed number used for the random number generation
- **caller**: your smart contract account

See our [Oracle Consumer contract](https://github.com/telosnetwork/rng-consumer) for a complete example of an implementation

## Important considerations

- Your implementation of the callback should **never** revert !
- The Oracle should answer almost instantly but this **won't necessarily ALWAYS be the case** for technical reasons, in the worst case scenario the Oracle will answer under 30 seconds.
