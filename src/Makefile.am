bin_PROGRAMS = spv
spv_SOURCES = main.cc encoder.h encoder.cc client.h client.cc util.h util.cc config.h netaddr.h buffer.h uvw.h logging.h
spv_CXXFLAGS = $(ERROR_CFLAGS) -I ../third_party/spdlog/include -I ../third_party -DDISABLE_IPV6
spv_CFLAGS = $(libuv_CFLAGS)
spv_LDADD = $(libuv_LIBS)
