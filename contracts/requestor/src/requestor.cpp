#include "../include/requestor.hpp";

// using namespace requestor;

//======================== admin actions ========================

ACTION requestor::init(string app_name, string app_version, name initial_admin) {
    
    //authenticate
    require_auth(get_self());
    
    //open config singleton
    config_singleton configs(get_self(), get_self().value);

    //validate
    check(!configs.exists(), "contract already initialized");
    check(is_account(initial_admin), "initial admin account doesn't exist");

    //initialize
    config initial_conf = {
        app_name, //app_name
        app_version, //app_version
        initial_admin, //admin
        uint64_t(1) //counter
    };

    //set initial config
    configs.set(initial_conf, get_self());

}

ACTION requestor::setversion(string new_version) {

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

ACTION requestor::setadmin(name new_admin) {

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

ACTION requestor::clearreq(uint64_t request_id, string memo) {

    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //authenticate
    require_auth(conf.admin);

    //open requests table, get request
    rngrequests_table rngrequests(get_self(), get_self().value);
    auto& req = rngrequests.get(request_id, "request not found");

    //erase request
    rngrequests.erase(req);

}

//======================== oracle actions ========================

ACTION requestor::upsertoracle(name oracle_name, public_key pub_key) {

    //open oracles table, find oracle
    oracles_table oracles(get_self(), get_self().value);
    auto orc_itr = oracles.find(oracle_name.value);

    //authenticate
    require_auth(oracle_name);

    if (orc_itr == oracles.end()) {

        //emplace new oracle
        oracles.emplace(oracle_name, [&](auto& col) {
            col.oracle_name = oracle_name;
            col.pub_key = pub_key;
        });

    } else {

        //update oracle
        oracles.modify(*orc_itr, same_payer, [&](auto& col) {
            col.pub_key = pub_key;
        });

    }
    
}

ACTION requestor::rmvoracle(name oracle_name, string memo) {

    //open oracles table, find oracle
    oracles_table oracles(get_self(), get_self().value);
    auto& orc = oracles.get(oracle_name.value, "oracle not found");

    //authenticate
    require_auth(orc.oracle_name);

    //erase oracle
    oracles.erase(orc);

}

//======================== request actions ========================

ACTION requestor::requestrand(uint64_t caller_id, 
                         uint64_t seed, 
                         const name& caller) {

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

    if (req_itr == rngrequests.end()) {
        //initialize
        uint8_t data[16];
        memcpy(data, &req_id, 8);
        memcpy(data + 8, &seed, 8);

        //calculate digest
        checksum256 digest = sha256((const char *)data, 128);

        //emplace new request
        rngrequests.emplace(get_self(), [&](auto& col) {
            col.request_id = req_id;
            col.caller_id = caller_id;
            col.digest = digest;
            col.request_time = now;
            col.caller = caller;
            col.oracle1 = name("eosio.null");
            col.oracle2 = name("eosio.null");
        });

    } else {

        //request already exists
        check(false, "request id already exists. contact app admin.");

    }

}

ACTION requestor::submitrand(uint64_t request_id, name oracle_name, signature sig) {
    //open oracles table, find oracle
    oracles_table oracles(get_self(), get_self().value);
    auto& orc = oracles.get(oracle_name.value, "oracle not found");

    //authenticate
    require_auth(orc.oracle_name);

    //open requests table, get request
    rngrequests_table rngrequests(get_self(), get_self().value);
    auto& req = rngrequests.get(request_id, "request not found");

    //validate
    check(orc.pub_key == recover_key(req.digest, sig), "signature not from registered public key");
    check(req.oracle1 != oracle_name && req.oracle2 != oracle_name, "Oracle account already submitted a signature");

    if (req.oracle1 == name("eosio.null")) {
        rngrequests.modify(req, same_payer, [&](auto& r) {
            r.oracle1 = oracle_name;
            r.sig1 = sig;
        });
    } else if (req.oracle2 == name("eosio.null")) {
        rngrequests.modify(req, same_payer, [&](auto& r) {
            r.oracle2 = oracle_name;
            r.sig2 = sig;
        });
    } else {

        //initialize
        char data[128];
        
        auto sig1_packed = eosio::pack(req.sig1);
        auto sig2_packed = eosio::pack(req.sig2);
        auto sig3_packed = eosio::pack(sig);
        uint8_t last_byte = (uint8_t) sig3_packed.back();

        if (last_byte % 2 == 0) {
            memcpy(data, &sig1_packed, 64);
            memcpy(data + 64, &sig2_packed, 64);
        } else {
            memcpy(data, &sig2_packed, 64);
            memcpy(data + 64, &sig1_packed, 64);
        }

        checksum256 random = sha256(data, 128);

        action(
            {get_self(), "active"_n}, 
            req.caller, "receiverand"_n,
            std::tuple(req.caller_id, random))
        .send();
        
        //erase request
        rngrequests.erase(req);

    }

}
