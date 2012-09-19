#include <string>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pthread.h>
#include "zmq.hpp"

using namespace std;

struct WorkerParam {
	string server_ip;
	string server_port;
	string addr;
	void *context;
};

void *WorkThreadProc(void *param)
{
	WorkerParam *worker_param = static_cast<WorkerParam *>(param);
	zmq::context_t *c = static_cast<zmq::context_t *>(worker_param->context);

	char addr_info[32] = {0};
	sprintf(addr_info, "tcp://%s:%s", worker_param->server_ip.data(), worker_param->server_port.data() );
	zmq::socket_t *sock = new zmq::socket_t(*c, ZMQ_REQ);
	
	// set options
	int linger = 0;
	sock->setsockopt(ZMQ_IDENTITY, worker_param->addr.data(), worker_param->addr.size() );
	sock->setsockopt(ZMQ_LINGER, &linger, sizeof(linger) );
	
	// connect to the server
	sock->connect(addr_info);
	
	while (1) {
		zmq::message_t req_msg(5);
		memcpy(req_msg.data(), "ready", 5);
		sock->send(req_msg);
		
		zmq::pollitem_t item = {*sock, 0, ZMQ_POLLIN, 0};
		string recv_str;
		int rc = zmq::poll(&item, 1, 5*1000*1000);
		if (rc == 0) {
			cerr << "recv timeout, reconnecting..." << endl;
			delete sock;
			sock = new zmq::socket_t(*c, ZMQ_REQ);
			sock->setsockopt(ZMQ_IDENTITY, worker_param->addr.data(), worker_param->addr.size() );
			sock->setsockopt(ZMQ_LINGER, &linger, sizeof(linger) );
			sock->connect(addr_info);
		} else {
			if (item.revents & ZMQ_POLLIN) {
				zmq::message_t resp_msg;
				
				sock->recv(&resp_msg);
				recv_str.assign( static_cast<string::value_type *>(resp_msg.data() ), resp_msg.size() );
			}
		}

		cout << recv_str << endl;
	}

	return reinterpret_cast<void *>(0);
}

int main(int argc, char **argv)
{
	int i, rc = 0;
	pthread_t th_id[5];
	WorkerParam param[5];

	if (argc < 4) {
		cerr << "Usage: "<< argv[0] << " [self addr] [server ip] [server port]" << endl;
		return (-1);
	}
	
	zmq::context_t context(1);

	for (i = 0; i < 5; ++i) {
		char name[32] = {0};
		sprintf(name, "%s:worker[%d]", argv[1], i);
		param[i].server_ip = argv[2];
		param[i].server_port = argv[3];
		param[i].context = &context;
		param[i].addr = name;
		if (pthread_create(&th_id[i], NULL, WorkThreadProc, &param[i]) != 0) {
			cerr << "Create thread" << "[" << i << "]" << "error!" << endl;
			return (-1);
		}
	}

	for (i = 0; i < 5; ++i) {
		pthread_join(th_id[i], (void **)&rc);
	}
	
	return 0;
}
