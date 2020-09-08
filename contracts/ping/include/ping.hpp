// 
//
// @author Craig Branscom
// @contract requestor
// @version v0.1.0

//oracle statuses: online, offline, retiring, frozen

#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/crypto.hpp>
#include <eosio/action.hpp>
#include <eosio/asset.hpp>

using namespace std;
using namespace eosio;

CONTRACT ping : public contract 
{
    public:

    ping(name self, name code, datastream<const char*> ds) : contract(self, code, ds) {};
    ~ping() {};

    static constexpr symbol TLOS_SYM = symbol("TLOS", 4); //X.XXXX TLOS
    // const asset DEFAULT_MIN_PLEDGE = asset(10000'0000, TLOS_SYM); //10,000 TLOS
    // const asset DEFAULT_REQUEST_PRICE = asset(100, TLOS_SYM); //0.01 TLOS
    // const uint32_t DEFAULT_RETIRE_DURATION = 7'776'000; //90 days in seconds
    const uint32_t TEN_YEARS_SEC = 315'360'000; //10 years in seconds

    //======================== config actions ========================

    //intialize the contract
    //auth: self
    ACTION init(string contract_name, string contract_version, name initial_admin);

    //set the contract version
    //auth: admin
    ACTION setversion(string new_version);

    //set new contract admin
    //auth: admin
    ACTION setadmin(name new_admin);

    //set minimum oracle pledge
    //auth: admin
    ACTION setminpledge(asset new_min_pledge);

    //set oracle retire duration (in seconds)
    //auth: admin
    ACTION setretiredur(uint32_t new_retire_duration);

    //log new request id
    //auth: contract@log
    ACTION logreqid(uint64_t new_request_id);

    //broadcast a fulfilled rng request
    //auth: contract@broadcast
    ACTION bcastrng(uint64_t request_id, uint64_t rng_result, name recipient);

    //approve a request recipient
    //auth: admin
    // ACTION approverecip(name recipient);

    //======================== oracle actions ========================

    //register a new oracle
    //auth: admin
    ACTION regoracle(name oracle_account, public_key pub_key);

    //pledge an amount to an oracle account
    //auth: oracle
    ACTION pledge(name oracle_account, asset pledge_amount);

    //change an oracle's public key
    //auth: oracle
    ACTION setpubkey(name oracle_account, public_key new_pub_key);

    //toggle oracle online/offline status
    //auth: oracle
    ACTION toggleonline(name oracle_account, string memo);

    //freeze/unfreeze an oracle
    //auth: admin
    ACTION freezeoracle(name oracle_account, string memo);

    //retire an oracle
    //auth: oracle
    ACTION retireoracle(name oracle_account);

    //reclaim an oracle pledge
    //pre: now > pledge_release_time
    //auth: oracle
    ACTION claimpledge(name oracle_account);

    //======================== job actions ========================

    //register a new job
    //auth: admin
    ACTION registerjob(name job_name, name job_category, asset price, string input_structure, string output_structure);

    //set new job price
    //auth: creator
    ACTION setjobprice(name job_name, asset new_price);

    //change job package version and hash
    //auth: admin
    // ACTION setjobpkg(name job_name, string new_package_version, string new_package_hash);

    //======================== request actions ========================

    //submit a new request
    //auth: requestor
    ACTION newrequest(name job_name, name requestor, name recipient, string input);

    //commit a data signature
    //auth: oracle
    // ACTION commitsig(uint64_t request_id, name oracle_account, signature commit_sig);

    //reveal a secret
    //auth: oracle
    // ACTION reveal(uint64_t request_id, name oracle_account, string json_result);

    //fulfill a request
    //auth: oracle
    ACTION fulfillreq(uint64_t request_id, name oracle_account, string output, signature sig);

    //cancel an existing request
    //auth: requestor
    ACTION cancelreq(uint64_t request_id, string memo);

    //clear a request
    //auth: admin
    ACTION clearreq(uint64_t request_id, string memo);

    //======================== notification handlers ========================

    //catch and handle a transfer from eosio.token
    [[eosio::on_notify("eosio.token::transfer")]]
    void catch_transfer(name from, name to, asset quantity, string memo);

    //======================== contract functions ========================

    //add to a balance
    //pre: amount > 0
    void add_balance(name account, asset amount);

    //subtract from a balance
    //pre: balance exists, sufficient funds in balance, amount > 0
    void sub_balance(name account, asset amount);

    //return true if oracle is qualified to be online
    bool is_qualified_oracle(name oracle_account);

    //======================== contract tables ========================

    //config singleton
    //scope: self
    TABLE config {
        string contract_name;
        string contract_version;
        name admin;
        uint64_t last_request_id = 0;

        asset min_oracle_pledge = asset(10000'0000, TLOS_SYM); //default 10k TLOS
        uint32_t oracle_retire_duration = 7'776'000; //default 90 days in seconds
        uint32_t registered_oracles = 0; //number of registered oracles
        uint32_t online_oracles = 0; //number of online oracles
        asset total_pledged = asset(0, TLOS_SYM); //total TLOS pledged among all oracles

        uint32_t registered_jobs = 0; //number of registered jobs
        uint64_t total_requests = 0; //lifetime job requests
        uint64_t total_fulfilled = 0; //lifetime job fulfillments
        uint32_t total_pending = 0; //total requests pending fulfillment

        uint32_t balance_count = 0; //number of open accounts
        asset total_balances = asset(0, TLOS_SYM); //total TLOS account balances secured by contract

        double ping_service_share = 1.0; //default 1%

        vector<name> job_categories = { name("rng") }; //list of valid job categories

        EOSLIB_SERIALIZE(config, (contract_name)(contract_version)(admin)(last_request_id)
            (min_oracle_pledge)(oracle_retire_duration)(registered_oracles)(online_oracles)(total_pledged)
            (registered_jobs)(total_requests)(total_fulfilled)(total_pending)
            (balance_count)(total_balances)
            (ping_service_share)
            (job_categories))
    };
    typedef singleton<name("config"), config> config_singleton;

    //oracles table
    //scope: self
    TABLE oracle {
        name oracle_account;
        name status; //online, offline, retiring, frozen
        public_key pub_key; //used to recover signatures
        asset pledged; //TLOS only
        time_point_sec pledge_release_time; //time pledge can be claimed by retired oracle

        //uint64_t lifetime_fulfilled;
        //uint32_t average_response_time;

        uint64_t primary_key() const { return oracle_account.value; }
        uint64_t by_status() const { return status.value; }

        EOSLIB_SERIALIZE(oracle, (oracle_account)(status)(pub_key)(pledged)(pledge_release_time)) 
    };
    typedef multi_index<name("oracles"), oracle,
        indexed_by<name("bystatus"), const_mem_fun<oracle, uint64_t, &oracle::by_status>>
    > oracles_table;

    //jobs table
    //scope: self
    //ram payer: creator
    TABLE job {
        name job_name;
        name job_category; //rng, pricefeed, sportsresult
        asset job_price; //in TLOS
        name creator;

        string input_structure; //JSON input
        string output_structure; //JSON output

        //uint64_t last_request_id; //request id of last request for this job

        // double creator_share; //creator share percentage

        // uint64_t lifetime_requests;
        // uint64_t lifetime_fulfilled;
        // uint32_t pending_requests;

        // string package_name;
        // string package_version;
        // string package_hash;

        uint64_t primary_key() const { return job_name.value; }
        uint64_t by_category() const { return job_category.value; }
        EOSLIB_SERIALIZE(job, (job_name)(job_category)(job_price)(creator)
            (input_structure)(output_structure))
    };
    typedef multi_index<name("jobs"), job,
        indexed_by<name("bycategory"), const_mem_fun<job, uint64_t, &job::by_category>>
    > jobs_table;

    //requests table
    //scope: self
    //ram payer: requestor
    TABLE request {
        uint64_t request_id; //global request id
        name job_name; //corresponding job name from jobs table
        name requestor; //account that placed the request
        name recipient; //account to notify with response
        time_point_sec request_time; //time request was placed

        string job_input; //JSON
        string job_output; //JSON

        // uint16_t commits_requested = 0;
        // map<name, signature> commits; oracle_account -> commit_signature
        // map<name, string> secrets; oracle_account -> JSON output

        uint64_t primary_key() const { return request_id; }
        uint64_t by_job_name() const { return job_name.value; }
        uint64_t by_requestor() const { return requestor.value; }
        uint64_t by_recipient() const { return recipient.value; }
        uint64_t by_req_time() const { return static_cast<uint64_t>(request_time.utc_seconds); }

        EOSLIB_SERIALIZE(request, (request_id)(job_name)(requestor)(recipient)(request_time)
            (job_input)(job_output))
    };
    typedef multi_index<name("requests"), request,
        indexed_by<name("byjobname"), const_mem_fun<request, uint64_t, &request::by_job_name>>,
        indexed_by<name("byrequestor"), const_mem_fun<request, uint64_t, &request::by_requestor>>,
        indexed_by<name("byrecipient"), const_mem_fun<request, uint64_t, &request::by_recipient>>,
        indexed_by<name("byreqtime"), const_mem_fun<request, uint64_t, &request::by_req_time>>
    > requests_table;

    //accounts table
    //scope: account
    //ram payer: account
    TABLE account {
        asset balance;

        uint64_t primary_key() const { return balance.symbol.code().raw(); }

        EOSLIB_SERIALIZE(account, (balance))
    };
    typedef multi_index<name("accounts"), account> accounts_table;

    //recipients table
    //scope: self
    //ram payer: self
    // TABLE recipient {
    //     name recipient_name;
    //     bool approved;
    //     uint64_t primary_key() const { return recipient_name.value; }
    //     EOSLIB_SERIALIZE(recipient, (recipient_name)(approved))
    // };
    // typedef multi_index<name("recipients"), recipient> recipients_table;

};