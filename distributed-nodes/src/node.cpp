extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
}

#include <thread>
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <vector>
#include "resolvehost.hpp"

const char delimiter = ' ';


int main(int argc, char* argv[])
{
    /* check cmd args */
	if (argc < 4) {
		std::cout << "Usage: ./node <node name> <ip addr/host name> <port #>" << std::endl;
		return EXIT_FAILURE;
	}

    /* read args */
    std::string node_name = argv[1];
	std::string ip = argv[2];
	int port = std::stoi(argv[3]);

    /* if host name provided, resolve to ip addr */
    std::vector<std::string> ipVec = resolveHostName(ip);
    if (ipVec.size() > 0)
        ip = ipVec[0];

	/* set up connection context */
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = port;
	server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    /* set up socket */
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0) {
		perror("[-]Error in socket");
		exit(1);
	} printf("[+]Node socket created successfully.\n");


    /* set up connection */
	int e = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    std::cout << sockfd << std::endl;
	if(e == -1) {
		perror("[-]Error in connection");
		exit(1);
	} printf("[+]Connected to Server.\n");

    /* prepare the node info */
    std::string node_info;
    node_info.append(node_name);
    node_info.push_back(delimiter);
    node_info.append(std::to_string(std::chrono::duration_cast<std::chrono::duration<double> >(std::chrono::system_clock::now().time_since_epoch()).count()));

    /* send node info */
    int ret = 0;
    while (ret == 0) {
        ret = send(sockfd, node_info.c_str(), (size_t)node_info.size(), 0);
    }

	/* prepare for receiving message from script and send */
	std::string time_stamp, msg;
    while (true) {
        // receive message
        std::cin >> time_stamp >> msg;
        
        // check msg validity
        if (time_stamp == "0" || msg == "0") 
            break;
        

        // prepare packet
        std::string pkt = node_name;
        pkt.push_back(delimiter);
        pkt.append(time_stamp);
        pkt.push_back(delimiter);
        pkt.append(msg);

        // send packet
        ret = 0;
        while (ret == 0)
            ret = send(sockfd, pkt.c_str(), (size_t)pkt.size(), 0);
        
        if (ret < 0) {
            std::cout << "[-]Error in sending" << std::endl;
            close(sockfd);
            return EXIT_FAILURE;
        }

        // debug purpose
        // std::cout << pkt << std::endl;
    }
	
	/* close */
	close(sockfd);

	return 0;
}
