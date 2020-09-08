#include "../include/ping.hpp";

//======================== config actions ========================

ACTION ping::init(string contract_name, string contract_version, name initial_admin)
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
        contract_name, //contract_name
        contract_version, //contract_version
        initial_admin //admin
    };

    //set initial config
    configs.set(initial_conf, get_self());
}

ACTION ping::setversion(string new_version)
{
    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //authenticate
    require_auth(conf.admin);

    //change contract version
    conf.contract_version = new_version;

    //set new config
    configs.set(conf, get_self());
}

ACTION ping::setadmin(name new_admin)
{
    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //authenticate
    require_auth(conf.admin);

    //validate
    check(is_account(new_admin), "new admin account does not exist");

    //change admin
    conf.admin = new_admin;

    //set new config
    configs.set(conf, get_self());
}

ACTION ping::setminpledge(asset new_min_pledge)
{
    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //authenticate
    require_auth(conf.admin);

    //validate
    check(new_min_pledge.symbol == TLOS_SYM, "pledge must be denominated in TLOS");

    //change minimum oracle pledge
    conf.min_oracle_pledge = new_min_pledge;

    //set new config
    configs.set(conf, get_self());
}

ACTION ping::setretiredur(uint32_t new_retire_duration)
{
    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //authenticate
    require_auth(conf.admin);

    //validate
    check(new_retire_duration > 0, "retire duration must be greater than zero");

    //change oracle retire duration
    conf.oracle_retire_duration = new_retire_duration;

    //set new config
    configs.set(conf, get_self());
}

ACTION ping::logreqid(uint64_t new_request_id)
{
    //authenticate
    require_auth(permission_level{get_self(), name("log")});
}

ACTION ping::bcastrng(uint64_t request_id, uint64_t rng_result, name recipient)
{
    //authenticate
    require_auth(permission_level{get_self(), name("broadcast")});

    //notify recipient
    require_recipient(recipient);
}

//======================== oracle actions ========================

ACTION ping::regoracle(name oracle_account, public_key pub_key)
{
    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //authenticate
    require_auth(conf.admin);

    //open oracles table, find oracle
    oracles_table oracles(get_self(), get_self().value);
    auto orc_itr = oracles.find(oracle_account.value);

    //validate
    check(orc_itr == oracles.end(), "oracle already registered");

    //initilialize
    time_point_sec now = time_point_sec(current_time_point());

    //emplace new oracle
    //ram payer: contract
    oracles.emplace(get_self(), [&](auto& col) {
        col.oracle_account = oracle_account;
        col.status = name("offline");
        col.pub_key = pub_key;
        col.pledged = asset(0, TLOS_SYM);
        col.pledge_release_time = now + uint32_t(315'360'000); //10 years in the future (reset when oracle starts retirement)
    });

    //update config
    conf.registered_oracles += 1;
    configs.set(conf, get_self());
}

ACTION ping::pledge(name oracle_account, asset pledge_amount)
{
    //authenticate
    require_auth(oracle_account);

    //validate
    check(pledge_amount.amount > 0, "must pledge positive amount");
    check(pledge_amount.symbol == TLOS_SYM, "pledge must be denominated in TLOS");

    //open oracles table, get oracle
    oracles_table oracles(get_self(), get_self().value);
    auto& orc = oracles.get(oracle_account.value, "oracle not found");

    //update oracle
    oracles.modify(orc, same_payer, [&](auto& col) {
        col.pledged += pledge_amount;
    });
}

ACTION ping::setpubkey(name oracle_account, public_key new_pub_key)
{
    //authenticate
    require_auth(oracle_account);

    //open oracles table, get oracle
    oracles_table oracles(get_self(), get_self().value);
    auto& orc = oracles.get(oracle_account.value, "oracle not found");

    //validate
    //TODO: check(new_pub_key.is_valid(orc.pub_key), "invalid public key");

    //update oracle
    oracles.modify(orc, same_payer, [&](auto& col) {
        col.pub_key = new_pub_key;
    });
}

ACTION ping::toggleonline(name oracle_account, string memo)
{
    //authenticate
    require_auth(oracle_account);

    //open oracles table, get oracle
    oracles_table oracles(get_self(), get_self().value);
    auto& orc = oracles.get(oracle_account.value, "oracle not found");

    //validate
    check(orc.status != name("frozen"), "oracle is frozen");
    check(orc.status != name("retiring"), "oracle is retiring");

    //initialize
    name new_status = orc.status == name("online") ? name("offline") : name("online");

    //update oracle
    oracles.modify(orc, same_payer, [&](auto& col) {
        col.status = new_status;
    });
}

ACTION ping::freezeoracle(name oracle_account, string memo)
{
    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //authenticate
    require_auth(conf.admin);

    //open oracles table, get oracle
    oracles_table oracles(get_self(), get_self().value);
    auto& orc = oracles.get(oracle_account.value, "oracle not found");

    //validate
    check(orc.status != name("frozen"), "oracle is already frozen");

    //update oracle
    oracles.modify(orc, same_payer, [&](auto& col) {
        col.status = name("frozen");
    });
}

ACTION ping::retireoracle(name oracle_account)
{
    //authenticate
    require_auth(oracle_account);

    //open oracles table, get oracle
    oracles_table oracles(get_self(), get_self().value);
    auto& orc = oracles.get(oracle_account.value, "oracle not found");

    //validate
    check(orc.status != name("frozen"), "cannot retire a frozen oracle");
    check(orc.status != name("retiring"), "oracle is already retiring");

    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //initialize
    time_point_sec now = time_point_sec(current_time_point());

    //update oracle
    oracles.modify(orc, same_payer, [&](auto& col) {
        col.status = name("retiring");
        col.pledge_release_time = now + conf.oracle_retire_duration;
    });
}

ACTION ping::claimpledge(name oracle_account)
{
    //authenticate
    require_auth(oracle_account);

    //open oracles table, get oracle
    oracles_table oracles(get_self(), get_self().value);
    auto& orc = oracles.get(oracle_account.value, "oracle not found");

    //initialize
    time_point_sec now = time_point_sec(current_time_point());

    //validate
    check(now > orc.pledge_release_time, "cannot claim pledge until after release time");

    //update balances
    add_balance(oracle_account, orc.pledged);
}

//======================== job actions ========================

ACTION ping::registerjob(name job_name, name job_category, asset price, string input_structure, string output_structure)
{
    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //authenticate
    require_auth(conf.admin);

    //open jobs table, find job
    jobs_table jobs(get_self(), get_self().value);
    auto j_itr = jobs.find(job_name.value);

    //validate
    check(j_itr == jobs.end(), "job name already exists");
    check(price.amount > 0, "job price must be greater than zero");
    check(price.symbol == TLOS_SYM, "job price must be denominated in TLOS");
    //TODO: validate job category

    //create new job
    jobs.emplace(get_self(), [&](auto& col) {
        col.job_name = job_name;
        col.job_category = job_category;
        col.job_price = price;
        col.creator = conf.admin;
        col.input_structure = input_structure;
        col.output_structure = output_structure;
    });
}

ACTION ping::setjobprice(name job_name, asset new_price)
{
    //open jobs table, get job
    jobs_table jobs(get_self(), get_self().value);
    auto& j = jobs.get(job_name.value, "job not found");

    //authenticate
    require_auth(j.creator);

    //validate
    check(new_price.amount > 0, "job price must be greater than zero");
    check(new_price.symbol == TLOS_SYM, "job price must be denominated in TLOS");

    //update job
    jobs.modify(j, same_payer, [&](auto& col) {
        col.job_price = new_price;
    });
}

//======================== request actions ========================

ACTION ping::newrequest(name job_name, name requestor, name recipient, string input)
{
    //authenticate
    require_auth(requestor);

    //open jobs table, get job
    jobs_table jobs(get_self(), get_self().value);
    auto& j = jobs.get(job_name.value, "job not found");

    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //initialize
    uint64_t new_req_id = conf.last_request_id + 1;
    time_point_sec now = time_point_sec(current_time_point());

    //validate
    check(is_account(recipient), "recipient account does not exist");

    //open requests table
    requests_table requests(get_self(), get_self().value);
    
    //create new request
    requests.emplace(get_self(), [&](auto& col) {
        col.request_id = new_req_id;
        col.job_name = job_name;
        col.requestor = requestor;
        col.recipient = recipient;
        col.request_time = now;
        col.job_input = input;
        col.job_output = "";
    });

    //update config
    conf.last_request_id = new_req_id;

    //set new config
    configs.set(conf, get_self());
}

ACTION ping::fulfillreq(uint64_t request_id, name oracle_account, string output, signature sig)
{
    //authenticate
    require_auth(oracle_account);

    //open oracles table, find oracle
    oracles_table oracles(get_self(), get_self().value);
    auto& orc = oracles.get(oracle_account.value, "oracle not found");

    //open requests table, get request
    requests_table requests(get_self(), get_self().value);
    auto& req = requests.get(request_id, "request not found");

    //initialize
    uint8_t data[8];
    memcpy(data, (uint8_t *)&request_id, 8);

    //calculate digest
    checksum256 digest = sha256((const char *)data, 8);

    //validate
    check(orc.pub_key == recover_key(digest, sig), "signature not from registered public key");

    //initialize
    uint64_t rand = 0;

    //generate number from signature
    // for (char c : sig.data) {
    //     int v = (int)c;
    //     if (rand % v == 0) {
    //         rand /= v;
    //     }
    //     rand += v;
    //     if (rand % v == 0) {
    //         rand /= v;
    //     }
    // }

    //send inline to bcastrng
    action(permission_level{get_self(), name("active")}, get_self(), name("bcastrng"), make_tuple(
        request_id, //request_id
        rand //rng_result
    )).send();

    //send inline to printdigest
    // action(permission_level{get_self(), name("active")}, get_self(), name("printdigest"), make_tuple(
    //     digest //digest
    // )).send();

    //erase request
    // requests.erase(req);
}

ACTION ping::cancelreq(uint64_t request_id, string memo)
{
    //open requests table, get request
    requests_table requests(get_self(), get_self().value);
    auto& req = requests.get(request_id, "request not found");

    //authenticate
    require_auth(req.requestor);

    //delete request
    requests.erase(req);
}

ACTION ping::clearreq(uint64_t request_id, string memo)
{
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

//======================== notification handlers ========================

void ping::catch_transfer(name from, name to, asset quantity, string memo)
{
    //TODO: handle transfer
}

//======================== contract functions ========================

void ping::add_balance(name account, asset amount)
{
    //open accounts table, find account
    accounts_table accounts(get_self(), account.value);
    auto acct_itr = accounts.find(amount.symbol.code().raw());

    //validate
    check(amount.amount > 0, "must add positive amount");

    //if account found
    if (acct_itr != accounts.end()) {
        //add to existing account
        accounts.modify(*acct_itr, same_payer, [&](auto& col) {
            col.balance += amount;
        });
    } else {
        //create new account
        accounts.emplace(get_self(), [&](auto& col) {
            col.balance = amount;
        });
    }
}

void ping::sub_balance(name account, asset amount)
{
    //open accounts table, get account
    accounts_table accounts(get_self(), account.value);
    auto& acct = accounts.get(amount.symbol.code().raw(), "account not found");

    //validate
    check(acct.balance >= amount, "insufficient funds");
    check(amount.amount > 0, "sub_balance: must subtract positive amount");

    //subtract from existing account
    accounts.modify(acct, same_payer, [&](auto& col) {
        col.balance -= amount;
    });
}

bool ping::is_qualified_oracle(name oracle_account)
{
    //open oracles table, find oracle
    oracles_table oracles(get_self(), get_self().value);
    auto oracle_itr = oracles.find(oracle_account.value);

    //if oracle account found
    if (oracle_itr != oracles.end()) {
        //open config singleton, get config
        config_singleton configs(get_self(), get_self().value);
        auto conf = configs.get();

        //validate oracle pledge
        if (oracle_itr->pledged >= conf.min_oracle_pledge) {
            return true;
        }
    }

    return false;
}