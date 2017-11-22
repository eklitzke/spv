#pragma once

#include "./encoder.h"
#include "./util.h"

namespace spv {
class Client {
 public:
  void send_version(Encoder *enc, int version, const NetAddr &addr) {
    uint64_t nonce = rand64();
    enc->push_int<int32_t>(7001);    // version
    enc->push_int<uint64_t>(0);      // services
    enc->push_time<uint64_t>();      // timestamp
    enc->push_addr(addr);            // addr_recv
    enc->push_addr(NetAddr());       // addr_from
    enc->push_int<uint64_t>(nonce);  // nonce
    enc->push_string("eklitzke");    // user-agent
    enc->push_int<uint32_t>(1);      // start height
    enc->push_bool(false);           // relay
  }

 private:
};
}  // namespace spv
