// Telos Random Number Generation Oracle
//
// @author Telos Core Devs
// @contract rngoracle
// @version v1.0.0

#include "../include/rng.oracle.hpp";

// using namespace rngoracle;

//======================== admin actions ========================

ACTION rngoracle::init(string app_name, string app_version, name initial_admin, uint8_t max_callback_tries, uint64_t max_request_timespan)
{

    //authenticate
    require_auth(get_self());

    //open config singleton
    config_singleton configs(get_self(), get_self().value);

    //validate
    check(!configs.exists(), "contract already initialized");
    check(is_account(initial_admin), "initial admin account doesn't exist");

    //initialize
    config initial_conf = {
        app_name,      //app_name
        app_version,   //app_version
        initial_admin, //admin
        uint64_t(1),    //counter
        max_request_timespan, // max timespan we keep requests
        max_callback_tries // max callback tries before we delete requests
    };

    //set initial config
    configs.set(initial_conf, get_self());
}

ACTION rngoracle::setmax(uint8_t max_callback_tries, uint64_t max_request_timespan)
{
    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //authenticate
    require_auth(conf.admin);

    //change max variables
    conf.max_callback_tries = max_callback_tries;
    conf.max_request_timespan = max_request_timespan;

    //set new config
    configs.set(conf, get_self());
}

ACTION rngoracle::setversion(string new_version)
{

    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //authenticate
    require_auth(conf.admin);

    //change version
    conf.app_version = new_version;

    //set new config
    configs.set(conf, get_self());
}

ACTION rngoracle::setadmin(name new_admin)
{

    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //authenticate
    require_auth(conf.admin);

    //change version
    conf.admin = new_admin;

    //set new config
    configs.set(conf, get_self());
}

//======================== oracle actions ========================

ACTION rngoracle::upsertoracle(name oracle_name, public_key pub_key)
{

    //open oracles table, find oracle
    oracles_table oracles(get_self(), get_self().value);
    auto orc_itr = oracles.find(oracle_name.value);

    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    if (orc_itr == oracles.end())
    {
        //authenticate admin to add a new oracle
        require_auth(conf.admin);

        //emplace new oracle
        oracles.emplace(conf.admin, [&](auto &col) {
            col.oracle_name = oracle_name;
            col.pub_key = pub_key;
        });
    }
    else
    {
        //authenticate oracle to update their key
        require_auth(oracle_name);

        //update oracle
        oracles.modify(*orc_itr, same_payer, [&](auto &col) {
            col.pub_key = pub_key;
        });
    }
}

ACTION rngoracle::rmvoracle(name oracle_name, string memo)
{

    //open oracles table, find oracle
    oracles_table oracles(get_self(), get_self().value);
    auto &orc = oracles.get(oracle_name.value, "oracle not found");

    //authenticate
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //authenticate
    if (!has_auth(conf.admin))
        require_auth(orc.oracle_name);

    //erase oracle
    oracles.erase(orc);
}

//======================== request actions ========================
ACTION rngoracle::clearreq(){

    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //open requests table, get request where timestamp & max tries match
    rngrequests_table rngrequests(get_self(), get_self().value);
    auto requests_by_timestamp = rngrequests.get_index<"timestamp"_n>();
    auto upper = requests_by_timestamp.upper_bound(current_time_point().sec_since_epoch() - conf.max_request_timespan); // remove 60s so we get only requests that are at least 1mn old
    uint64_t count = 15; // max 15 refunds
    for(auto itr = requests_by_timestamp.begin(); count > 0 && itr != upper; count--) {
        if(itr->callback_tries >= conf.max_callback_tries){
            itr = requests_by_timestamp.erase(itr);
        }
    }

}
ACTION rngoracle::rmvrequest(uint64_t request_id){

    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //open requests table, get request
    rngrequests_table rngrequests(get_self(), get_self().value);
    auto &req = rngrequests.get(request_id, "request not found");

    //authenticate as caller or admin
    check(has_auth(req.caller) || has_auth(conf.admin), "Only the request caller on admin can remove requests");

    //erase request
    rngrequests.erase(req);

}

ACTION rngoracle::requestrand(uint64_t caller_id,
                              uint64_t seed,
                              const name &caller)
{

    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //initialize
    uint64_t req_id = conf.counter;
    time_point_sec now = time_point_sec(current_time_point());

    //validate
    require_auth(caller);

    //increment counter and save
    conf.counter += 1;
    configs.set(conf, get_self());

    //open requests table, find request
    rngrequests_table rngrequests(get_self(), get_self().value);
    auto req_itr = rngrequests.find(req_id);

    if (req_itr == rngrequests.end())
    {
        //initialize
        uint8_t data[16];
        memcpy(data, &req_id, 8);
        memcpy(data + 8, &seed, 8);

        //calculate digest
        checksum256 digest = sha256((const char *)data, 128);

        //emplace new request
        rngrequests.emplace(caller, [&](auto &col) {
            col.request_id = req_id;
            col.caller_id = caller_id;
            col.digest = digest;
            col.request_time = now;
            col.caller = caller;
            col.oracle1 = name("eosio.null");
            col.oracle2 = name("eosio.null");
        });
    }
    else
    {

        //request already exists
        check(false, "request id already exists. contact app admin.");
    }
}

ACTION rngoracle::submitrand(uint64_t request_id, name oracle_name, signature sig)
{
    //open oracles table, find oracle
    oracles_table oracles(get_self(), get_self().value);
    auto &orc = oracles.get(oracle_name.value, "oracle not found");

    //authenticate
    require_auth(orc.oracle_name);

    //increment oracle sigcount
    oracles.modify(orc, same_payer, [&](auto &r) {
        r.sigcount = r.sigcount + 1;
    });

    //open requests table, get request
    rngrequests_table rngrequests(get_self(), get_self().value);
    auto &req = rngrequests.get(request_id, "request not found");

    //validate
    check(orc.pub_key == recover_key(req.digest, sig), "signature not from registered public key");
    check(req.oracle1 != oracle_name && req.oracle2 != oracle_name, "Oracle account already submitted a signature");

    if (req.oracle1 == name("eosio.null"))
    {
        rngrequests.modify(req, same_payer, [&](auto &r) {
            r.oracle1 = oracle_name;
            r.sig1 = sig;
        });
    }
    else if (req.oracle2 == name("eosio.null"))
    {
        rngrequests.modify(req, same_payer, [&](auto &r) {
            r.oracle2 = oracle_name;
            r.sig2 = sig;
        });
    }
    else
    {
        auto sig1_packed = eosio::pack(req.sig1);
        auto sig2_packed = eosio::pack(req.sig2);
        auto sig3_packed = eosio::pack(sig);
        uint8_t last_byte = (uint8_t)sig3_packed.back();

        auto total_size = sig1_packed.size() + sig2_packed.size();
        char data[total_size];
        if (last_byte % 2 == 0)
        {
            memcpy(data, sig1_packed.data(), sig1_packed.size());
            memcpy(data + sig1_packed.size(), sig2_packed.data(), sig2_packed.size());
        }
        else
        {
            memcpy(data, sig2_packed.data(), sig2_packed.size());
            memcpy(data + sig2_packed.size(), sig1_packed.data(), sig1_packed.size());
        }

        checksum256 random = sha256(data, total_size);

        rngrequests.modify(req, same_payer, [&](auto &r) {
            r.callback_tries = r.callback_tries + 1;
        });

        action(
            {get_self(), "active"_n},
            req.caller, "receiverand"_n,
            std::tuple(req.caller_id, random))
            .send();

        //erase request
        rngrequests.erase(req);
    }
}
