extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
}

#include <thread>
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <vector>
#include <map>
#include "rmulticast.hpp"
#include "config_parser.hpp"
#include "transaction.hpp"


int node_listen(int port)
{
	/* set up connection context */
	struct sockaddr_in server_addr;
	std::string ip = "127.0.0.1";
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = port;
	server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    /* create socket */
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0) {
		perror("[-]Error in socket");
		exit(1);
	} printf("[+]Server socket created successfully.\n");

    /* bind socket */
	int e = bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if(e < 0) {
		perror("[-]Error in bind");
		exit(1);
	} printf("[+]Binding successfull.\n");

    /* listen for connection */
	if(listen(sockfd, 10) == 0){
		printf("[+]Listening....%d\n", port);
	} else {
		perror("[-]Error in listening");
		exit(1);
	}

	return sockfd;
}

void node_connect(std::map<std::string, std::vector<std::string>>& nodes, std::map<std::string, int>& node_fds, int node_id)
{
	for (auto i = nodes.cbegin(); i != nodes.cend(); i++) {
		std::string node_name = i->first;
		std::string ip = i->second[0];
		int port = std::stoi(i->second[1]);

		/* set up connection context */
		struct sockaddr_in server_addr;
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = port;
		server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

		/* set up connection */
		bool connected = false;
		int connectfd = -1;
		while (!connected) {
			/* set up socket */
			connectfd = socket(AF_INET, SOCK_STREAM, 0);
			if(connectfd < 0) {
				perror("[-]Error in socket");
				exit(1);
			} printf("[+]Node socket created successfully.\n");

			/* connect to nodes */
			int err = connect(connectfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
			if (err < 0) {
				close(connectfd);
				char buffer[256];
    			strerror_r(errno, buffer, 256);
				std::cout << "[-]Error: " << buffer << std::endl;
				std::cout << "try connect " << node_name << "@" << server_addr.sin_port << std::endl;
				std::this_thread::sleep_for(std::chrono::seconds(5));
			} else {
				connected = true;
			}
		}
		std::cout << "[+] Connect to " << node_name << std::endl;

		/* prepare init */
		std::string init_info;
		init_info.push_back((char)INIT);
		init_info.push_back(delimiter);
		init_info.append("node");
		init_info.push_back((char)(node_id+'0'));

		/* send init info */
		int ret = 0;
		while (ret == 0) {
			ret = send(connectfd, init_info.c_str(), (size_t)init_info.size(), 0);
		}

		/* record the socket */
		node_fds[node_name] = connectfd;
	}

	return;

}


int main(int argc, char* argv[])
{
    /* check cmd args */
	if (argc < 4) {
		std::cout << "Usage: ./mp1_node <node id> <port #> <config file>" << std::endl;
		return EXIT_FAILURE;
	}
    
    /* read args */
    int node_id = std::stoi(argv[1]);
	int port = std::stoi(argv[2]);
	std::string config = argv[3];

	/* parse config file */
	int node_number = -1;
	auto nodes = parser(config, node_number);

	/* start connection */
	std::map<std::string, int> server_node_fds;
	std::thread connect_thread(node_connect, std::ref(nodes),  std::ref(server_node_fds), node_id);

	/* start listening */
	std::map<std::string, int> client_node_fds;
    int listenfd = node_listen(port);

	/* start application thread */
	std::queue<std::string> transaction_queue;
	std::thread transaction_handler(transaction_recv, std::ref(transaction_queue), node_id);

	/* start Relaibel multicast */
	std::thread rmulticastcast(rmulti_cast, node_id, node_number, std::ref(server_node_fds), std::ref(transaction_queue));
	std::thread rmulticastrecv(rmulti_recv, node_number, std::ref(client_node_fds), std::ref(transaction_queue));


	/* accept incoming connection */
	int new_sock = -1;
	struct sockaddr_in new_addr;
	socklen_t addr_size = sizeof(new_addr);
	std::vector<std::thread> threads;

    while ((new_sock = accept(listenfd, (struct sockaddr*)&new_addr, &addr_size)) > 0) {
        std::cout << client_node_fds.size() << " accept:" << new_sock << " port:" << new_addr.sin_port << std::endl;
        // create thread for node listening
        std::thread node_thread(client_node_listening, new_sock, std::ref(client_node_fds));
        threads.push_back(std::move(node_thread));
    }

	std::cout << "Done" << std::endl;

	/* close */
	close(listenfd);

	return 0;
}
