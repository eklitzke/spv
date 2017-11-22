#pragma once

#include "./config.h"
#include "./encoder.h"
#include "./util.h"

namespace spv {

// copied from chainparams.cpp
static const std::vector<std::string> mainSeeds = {
    "seed.bitcoin.sipa.be",          "dnsseed.bluematt.me",
    "dnsseed.bitcoin.dashjr.org",    "seed.bitcoinstats.com",
    "seed.bitcoin.jonasschnelli.ch", "seed.btc.petertodd.org",
};

// copied from chainparams.cpp
static const std::vector<std::string> testSeeds = {
    "testnet-seed.bitcoin.jonasschnelli.ch", "seed.tbtc.petertodd.org",
    "testnet-seed.bluematt.me",
};

class Client {
 public:
  void send_version(Encoder *enc, int version, const NetAddr &addr) {
    uint64_t nonce = rand64();
    enc->push_int<int32_t>(version);  // version
    enc->push_int<uint64_t>(0);       // services
    enc->push_time<uint64_t>();       // timestamp
    enc->push_addr(addr);             // addr_recv
    enc->push_addr(NetAddr());        // addr_from
    enc->push_int<uint64_t>(nonce);   // nonce
    enc->push_string(USERAGENT);      // user-agent
    enc->push_int<uint32_t>(1);       // start height
    enc->push_bool(false);            // relay
  }

 private:
};
}  // namespace spv
