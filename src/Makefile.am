bin_PROGRAMS = spv
spv_SOURCES = addr.cc addr.h buffer.h client.cc client.h encoder.cc encoder.h logging.h main.cc util.cc util.h uvw.h
spv_CXXFLAGS = $(ERROR_CFLAGS) -I ../third_party/spdlog/include -I ../third_party -DDISABLE_IPV6 -std=c++17
spv_CFLAGS = $(libuv_CFLAGS)
spv_LDADD = $(libuv_LIBS)
