// Telos Random Number Generation Oracle
//
// @author Telos Core Developers (telosnetwork)
// @contract rngoracle
// @version v1.0.1

#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/crypto.hpp>

using namespace std;
using namespace eosio;

CONTRACT rngoracle : public contract {

public:

    rngoracle(name self, name code, datastream<const char*> ds) : contract(self, code, ds) {};

    ~rngoracle() {};

    //======================== admin actions ========================

    // initialize the contract
    ACTION init(string app_name, string app_version, name initial_admin);

    // set the contract version
    ACTION setversion(string new_version);

    // set new contract admin
    ACTION setadmin(name new_admin);

    //======================== oracle actions ========================

    // add a new oracle public key
    ACTION upsertoracle(name oracle_name, public_key pub_key);

    // remove an oracle key
    ACTION rmvoracle(name oracle_name, string memo);

    //======================== request actions ========================

    // callback failure notification
    ACTION notifyfail(uint64_t request_id, name oracle_name);

    // remove a request
    ACTION rmvrequest(uint64_t request_id);

    // request a random value
    ACTION requestrand(uint64_t caller_id, uint64_t seed, const name& caller);

    // submit a random value
    ACTION submitrand(uint64_t request_id, name oracle_name, signature sig);

    //======================== contract tables ========================

    //config singleton
    //scope: self
    TABLE config {
        string app_name;
        string app_version;
        name admin;
        uint64_t counter;

        EOSLIB_SERIALIZE(config, (app_name)(app_version)(admin)(counter))
    };
    typedef singleton<name("config"), config> config_singleton;

    // Signers
    //scope: self
    TABLE oracle {
        name oracle_name;
        uint64_t sigcount;
        public_key pub_key;

        uint64_t primary_key() const { return oracle_name.value; }

        EOSLIB_SERIALIZE(oracle, (oracle_name)(sigcount)(pub_key)) 
    };
    typedef multi_index<name("oracles"), oracle> oracles_table;

    // Request
    //scope: self
    TABLE rngrequest {
        uint64_t request_id;
        uint64_t caller_id;
        checksum256 digest;
        name oracle1;
        signature sig1;
        name oracle2;
        signature sig2;
        time_point_sec request_time;
        name caller; //account to require_recipient
        name failed_callback_oracle;

        uint64_t primary_key() const { return request_id; }
        EOSLIB_SERIALIZE(rngrequest, (request_id)(caller_id)(digest)(oracle1)(sig1)(oracle2)(sig2)(request_time)(caller)(failed_callback_oracle))
    };
    typedef multi_index<name("rngrequests"), rngrequest> rngrequests_table;

};