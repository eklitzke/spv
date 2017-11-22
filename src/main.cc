#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "./client.h"
#include "./encoder.h"
#include "./util.h"

using namespace spv;

int main(int argc, char** argv) {
  NetAddr addr;
  Encoder enc("version");
  Client client;
  client.send_version(&enc, 100, addr);
  std::cout << string_to_hex(enc.serialize()) << "\n";
  return 0;
}
