%.pb.cc %.pb.h: %.proto
	$(PROTOC) --proto_path=$(srcdir) --cpp_out=$(srcdir) $^

bin_PROGRAMS = spv
spv_SOURCES = addr.cc addr.h buffer.cc buffer.h chain.cc chain.h client.cc client.h connection.cc connection.h constants.cc constants.h decoders.cc encoders.cc fields.cc fields.h logging.h main.cc message.h peer.cc peer.h pow.cc pow.h spv.pb.cc spv.pb.h util.cc util.h uvw.cc uvw.h
spv_CFLAGS = $(libuv_CFLAGS) $(protobuf_CFLAGS)
spv_LDADD = $(libuv_LIBS) $(protobuf_LIBS)
