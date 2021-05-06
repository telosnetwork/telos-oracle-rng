const ecc = require("eosjs-ecc");
const HyperionStreamClient = require("@eosrio/hyperion-stream-client").default;
const fetch = require("node-fetch");

const REQUESTS_TABLE = "rngrequests";

class RequestOracle {
  constructor(
    oracleContract,
    oracleName,
    oraclePermission,
    rpc,
    api,
    signerKey
  ) {
    this.oracleContract = oracleContract;
    this.oracleName = oracleName;
    this.oraclePermission = oraclePermission;
    this.rpc = rpc;
    this.api = api;
    this.signerKey = signerKey;
  }

  async start() {
    await this.startStream();
    await this.doTableCheck();
    // TODO: maybe a 15 second setInterval against doTableCheck?  In case stream takes a crap?
  }

  async startStream() {
    this.streamClient = new HyperionStreamClient(
      process.env.HYPERION_ENDPOINT,
      {
        async: true,
        fetch: fetch,
      }
    );
    let getInfo = await this.rpc.get_info();
    let headBlock = getInfo.head_block_num;
    this.streamClient.onConnect = () => {
      this.streamClient.streamDeltas({
        code: this.oracleContract,
        table: REQUESTS_TABLE,
        scope: this.oracleContract,
        payer: "",
        start_from: headBlock,
        read_until: 0,
      });
    };

    this.streamClient.onData = async (data, ack) => {
      if (data.content.present) await this.signRow(data.content.data);

      ack();
    };

    this.streamClient.connect(() => {
      console.log("Connected to Hyperion Stream!");
    });
  }

  async doTableCheck() {
    console.log(`Doing table check...`);
    const results = await this.rpc.get_table_rows({
      code: this.oracleContract,
      scope: this.oracleContract,
      table: REQUESTS_TABLE,
      limit: 1000,
    });

    results.rows.forEach((row) => this.signRow(row));
    console.log(`Done doing table check!`);
  }

  async signRow(row) {
    console.log(`Signing request_id: ${row.request_id}...`)
    if (row.oracle1 == this.oracleName || row.oracle2 == this.oracleName)
      return;

    try {
      const result = await this.api.transact(
        {
          actions: [
            {
              account: this.oracleContract,
              name: "submitrand",
              authorization: [
                {
                  actor: this.oracleName,
                  permission: this.oraclePermission,
                },
              ],
              data: {
                request_id: row.request_id,
                oracle_name: this.oracleName,
                sig: ecc.signHash(row.digest, this.signerKey),
              },
            },
          ],
        },
        { blocksBehind: 10, expireSeconds: 60 }
      );
      console.log(`Signed request ${row.request_id}`);
    } catch (e) {
      console.error(`Submitting signature failed: ${e}`);
    }
  }
}

module.exports = RequestOracle;
