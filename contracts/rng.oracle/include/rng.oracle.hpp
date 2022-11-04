// Telos Random Number Generation Oracle
//
// @author Telos Core Devs
// @contract rngoracle
// @version v1.0.0

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

    //intialize the contract
    ACTION init(string app_name, string app_version, name initial_admin, uint8_t max_callback_tries, uint64_t max_request_timespan);

    //set the contract version
    ACTION setversion(string new_version);

    //set new contract admin
    ACTION setadmin(name new_admin);

    //set new max for timespan & callback tries
    ACTION setmax(uint8_t max_callback_tries, uint64_t max_request_timespan);

    //======================== oracle actions ========================

    //add a new oracle public key
    ACTION upsertoracle(name oracle_name, public_key pub_key);

    //remove an oracle key
    ACTION rmvoracle(name oracle_name, string memo);

    //======================== request actions ========================

    //clear a request
    ACTION clearreq();

    // remove a request
    ACTION rmvrequest(uint64_t request_id);

    //request a random value
    ACTION requestrand(uint64_t caller_id, uint64_t seed, const name& caller);

    //submit a random value
    ACTION submitrand(uint64_t request_id, name oracle_name, signature sig);

    //TODO: other types of requests

    //======================== contract tables ========================

    //config singleton
    //scope: self
    TABLE config {
        string app_name;
        string app_version;
        name admin;
        uint64_t counter;
        uint64_t max_request_timespan;
        uint8_t max_callback_tries;

        EOSLIB_SERIALIZE(config, (app_name)(app_version)(admin)(counter)(max_request_timespan)(max_callback_tries))
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
        uint8_t callback_tries; //account to require_recipient

        uint64_t primary_key() const { return request_id; }
        uint64_t by_timestamp() const {return request_time.sec_since_epoch(); }
        EOSLIB_SERIALIZE(rngrequest, (request_id)(caller_id)(digest)(oracle1)(sig1)(oracle2)(sig2)(request_time)(caller)(callback_tries))
    };
    typedef multi_index<name("rngrequests"), rngrequest,
           indexed_by<"timestamp"_n, const_mem_fun<rngrequest, uint64_t, &rngrequest::by_timestamp >>
    >  rngrequests_table;

};