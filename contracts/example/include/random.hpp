// Telos Random is...
//
// @author Craig Branscom
// @contract random
// @version v0.1.0

#include <eosio/eosio.hpp>

using namespace std;
using namespace eosio;

CONTRACT random : public contract {

public:

    random(name self, name code, datastream<const char*> ds) : contract(self, code, ds) {};

    ~random() {};

    //======================== admin actions ========================

    //intialize the contract
    ACTION init(string app_name, string app_version, name initial_admin);

    //set the contract version
    ACTION setversion(string new_version);

    //set new contract admin
    ACTION setadmin(name new_admin);

    //add new public key
    ACTION addpubkey(name key_name, public_key new_key);

    //remove a public key
    ACTION rmvpubkey(name key_name);

    //======================== request actions ========================

    //request a random value
    ACTION requestrand(name recipient, optional<name> key_name);

    //submit a random value
    ACTION submitrand(uint64_t request_id, singature sig);

    //======================== contract tables ========================

    //config singleton
    //scope: self
    TABLE config {
        string app_name;
        string app_version;
        name admin;
    };
    typedef singleton<name("config"), config> config_singleton;

    //key entry
    //scope: self
    TABLE key_entry {
        name key_name;
        public_key pubkey;

        uint64_t primary_key() const { return key_name.value; }
        EOSLIB_SERIALIZE(key_entry, (key_name)(pubkey))
    };
    typedef multi_index<name("keys"), key_entry> keys_table;

    //request entry
    //scope: self
    TABLE request {
        uint64_t request_id;
        name key_name;
        name recipient;

        uint46_t primary_key() const { return request_id; }
        EOSLIB_SERIALIZE(request, (request_id)(key_name)(recipient))
    };
    typedef multi_index<name("requests"), request> requests_table;

};