#include "../include/example.hpp";

// using namespace random;

//======================== admin actions ========================

ACTION random::init(string app_name, string app_version, name initial_admin) {
    
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
        initial_admin //admin
    };

    //set initial config
    configs.set(initial_conf, get_self());

}

ACTION random::setversion(string new_version) {

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

ACTION random::setadmin(name new_admin) {

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

ACTION random::addpubkey(name key_name, public_key new_key) {

    //TODO: implement

}

ACTION random::rmvpubkey(name key_name) {

    //TODO: implement

}

//======================== request actions ========================

ACTION random::requestrand(name recipient, optional<name> key_name) {

    //TODO: implement

}

ACTION random::submitrand(uint64_t request_id, checksum256 sig) {

    //TODO: implement

}
