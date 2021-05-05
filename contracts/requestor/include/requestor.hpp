// Telos Random is...
//
// @author Craig Branscom
// @contract requestor
// @version v0.1.0

#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/crypto.hpp>

using namespace std;
using namespace eosio;

CONTRACT requestor : public contract {

public:

    requestor(name self, name code, datastream<const char*> ds) : contract(self, code, ds) {};

    ~requestor() {};

    //======================== admin actions ========================

    //intialize the contract
    ACTION init(string app_name, string app_version, name initial_admin);

    //set the contract version
    ACTION setversion(string new_version);

    //set new contract admin
    ACTION setadmin(name new_admin);

    //clear a request
    ACTION clearreq(uint64_t request_id, string memo);

    //======================== oracle actions ========================

    //add a new oracle public key
    ACTION upsertoracle(name oracle_name, public_key pub_key);

    //remove an oracle key
    ACTION rmvoracle(name oracle_name, string memo);

    //======================== request actions ========================

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

        EOSLIB_SERIALIZE(config, (app_name)(app_version)(admin)(counter))
    };
    typedef singleton<name("config"), config> config_singleton;

    //oracle keys
    //scope: self
    TABLE oracle {
        name oracle_name;
        public_key pub_key;

        uint64_t primary_key() const { return oracle_name.value; }

        EOSLIB_SERIALIZE(oracle, (oracle_name)(pub_key)) 
    };
    typedef multi_index<name("oracles"), oracle> oracles_table;

    //request entry
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

        uint64_t primary_key() const { return request_id; }
        EOSLIB_SERIALIZE(rngrequest, (request_id)(caller_id)(digest)(oracle1)(sig1)(oracle2)(sig2)(request_time)(caller))
    };
    typedef multi_index<name("rngrequests"), rngrequest> rngrequests_table;

};