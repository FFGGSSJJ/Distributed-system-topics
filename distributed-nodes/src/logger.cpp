extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
}

#include <thread>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <chrono>
#include <mutex>
#include "resolvehost.hpp"

/* mutex to guarantee thread safety */
std::mutex logger_mutex;
std::mutex performance_mutex;

const int recv_size = 4096;
const char delimiter = ' ';
std::ofstream logger("logs/logger.txt", std::ios::out | std::ios::trunc);
std::ofstream performance("logs/performance.txt", std::ios::out | std::ios::trunc);

/* receive data */
void recv_data(int sockfd){
	
    /* init data buffer */
	char buffer[recv_size];
    memset(buffer, 0, (size_t)recv_size);

    /* receive node info */
    int ret = recv(sockfd, buffer, (size_t)recv_size, 0);
    std::string info = buffer;
    std::string node_name = info.substr(0, info.find(delimiter));
    std::string init_time = info.substr(info.find(delimiter)+1);
    
    /* get mutex */
    while(!logger_mutex.try_lock());
    logger << init_time << delimiter << node_name << " - connected" << std::endl;
    logger_mutex.unlock();

	/* receive data */
	while (true) {

        auto s = std::chrono::high_resolution_clock::now();
		ret = recv(sockfd, buffer, (size_t)recv_size, 0);
        auto e = std::chrono::high_resolution_clock::now();
        /* check recv bytes */
		if (ret <= 0) {
            logger << std::to_string(std::chrono::duration_cast<std::chrono::duration<double> >(std::chrono::system_clock::now().time_since_epoch()).count()) << delimiter << node_name <<" - disconnected" << std::endl;
            break;
        }
	
        /* prepare to write logger */
        std::string buffer_str = buffer;
        std::vector<std::string> pkt_strs;

        /* parse pkt */
        auto id = buffer_str.find(delimiter);
        while (id != std::string::npos) {
            pkt_strs.push_back(buffer_str.substr(0, id));
            buffer_str.erase(0, id+1);
            id = buffer_str.find(delimiter);
        } pkt_strs.push_back(buffer_str);
		
        /* write logger */
        std::string log_stamp = std::to_string(std::chrono::duration_cast<std::chrono::duration<double> >(std::chrono::system_clock::now().time_since_epoch()).count());
        std::string name = pkt_strs[0];
        std::string msg_stamp = pkt_strs[1];
        std::string msg = pkt_strs[2];
        while(!logger_mutex.try_lock());
        logger << log_stamp  << delimiter << node_name << std::endl << msg_stamp << delimiter << msg << std::endl; 
        logger_mutex.unlock();
		// std::cout << log_stamp  << delimiter << name << std::endl << msg_stamp << delimiter << msg << std::endl; 

        /* write report */
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(e - s).count();
        double duration_s = (double) ((double)duration/(double)1000000);
        double bs = (double)ret/duration_s;
        double delay = std::stod(log_stamp) - std::stod(msg_stamp);

        while(!performance_mutex.try_lock());
        performance << node_name << delimiter << delay << delimiter << bs << std::endl;
        performance_mutex.unlock();

        /* next loop */
        memset(buffer, 0, (size_t)recv_size);
	}

    /* thread ends */
	return;
}

int main(int argc, char* argv[])
{
    /* check args */
	if (argc < 2) {
		std::cout << "Usage: ./logger <ip> <port #>" << std::endl;
		return EXIT_FAILURE;
	}

    /* read args */
	int port = std::stoi(argv[2]);
	std::string ip = argv[1];

    /* if host name provided */
    std::vector<std::string> ipVec = resolveHostName(ip);
    if (ipVec.size() > 0) {
        ip = ipVec[0];
    }
    
	/* setup connection context */
	struct sockaddr_in server_addr, new_addr;
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
		printf("[+]Listening....\n");
	} else {
		perror("[-]Error in listening");
		exit(1);
	}

    /* prepare multi-thread */
    std::vector<std::thread> threads;
	socklen_t addr_size = sizeof(new_addr);
    int new_sock = -1;

    /* check for available node connection */
    while ((new_sock = accept(sockfd, (struct sockaddr*)&new_addr, &addr_size)) > 0) {
        std::cout << new_sock << std::endl;
        // create thread for node connection
        std::thread node_thread(recv_data, new_sock);
        threads.push_back(std::move(node_thread));
    }

    /* close */
    logger.close();
    performance.close();

	return 0;
}
