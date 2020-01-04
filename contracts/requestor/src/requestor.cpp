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
    requests_table requests(get_self(), get_self().value);
    auto& req = requests.get(request_id, "request not found");

    //erase request
    requests.erase(req);

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

ACTION requestor::requestrand(name recipient) {

    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //initialize
    uint64_t req_id = conf.counter;
    time_point_sec now = time_point_sec(current_time_point());

    //validate
    check(is_account(recipient), "recipient account doesn't exist");

    //increment counter and save
    conf.counter += 1;
    configs.set(conf, get_self());

    //open requests table, find request
    requests_table requests(get_self(), get_self().value);
    auto req_itr = requests.find(req_id);

    if (req_itr == requests.end()) {

        //emplace new request
        requests.emplace(get_self(), [&](auto& col) {
            col.request_id = req_id;
            col.request_type = name("randomnumber");
            col.request_time = now;
            col.recipient = recipient;
        });

    } else {

        //request already exists
        check(false, "request id already exists. contact app admin.");

    }

}

ACTION requestor::submitrand(uint64_t request_id, name oracle_name, checksum256 digest, signature sig, uint64_t rand) {

    //open oracles table, find oracle
    oracles_table oracles(get_self(), get_self().value);
    auto& orc = oracles.get(oracle_name.value, "oracle not found");

    //open requests table, get request
    requests_table requests(get_self(), get_self().value);
    auto& req = requests.get(request_id, "request not found");

    //authenticate
    require_auth(orc.oracle_name);

    //validate
    check(orc.pub_key == recover_key(digest, sig), "public key mismatch");

    //forward to recipient
    require_recipient(req.recipient);

    //erase request
    requests.erase(req);

}
