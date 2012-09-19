#include <string>
#include <iostream>
#include <cstdio.h>
#include <cstdlib.h>
#include <cstring.h>
#include "zmq.hpp"

using namespace std;

int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "%s [port]");
	}
	
	zmq::context_t context(1);
	
	// bind service
	char addr_info[32] = {0};
	sprintf(addr_info, "tcp://*:%s", argv[1]);	
	zmq::socket_t sock(context, ZMQ_XREP);
	sock.bind(addr_info);
	
	// main loop
	while (1) {
		
		zmq::message_t addr_msg;
		zmq::message_t empty_msg;	
		zmq::message_t req_msg;
		
		// receive request
		sock.recv(&addr_msg);
		sock.recv(&empty_msg);
		sock.recv(&req_msg);
		
		// debug output
		string addr;
		string req;
		addr.assign(static_cast<string::value_type *>(addr_msg.data() ), addr_msg.size() );
		req.assign(static_cast<string::value_type *>(req_msg.data() ), req_msg.size() );
		
		// send response
		string resp = "resp-->";
		resp += addr;
		zmq::message_t resp_msg(resp.size() );
		memcpy(resp_msg.data(), resp.data(), resp.size());
		sock.send(addr_msg, ZMQ_SNDMORE);
		sock.send(empty_msg, ZMQ_SNDMORE);
		sock.send(resp_msg);
	}
	
	return 0;
}
