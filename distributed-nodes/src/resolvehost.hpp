extern "C" {
#include <netdb.h>
#include <arpa/inet.h>
}

#include <string>
#include <vector>
#include <iostream>

/**
 * @brief resolve hostname into IP address
*/
std::vector<std::string> resolveHostName(std::string& hostname) {
    std::vector<std::string> ipAddrs;
    struct addrinfo hints;
    struct addrinfo* result = new addrinfo();

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;     // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // Stream socket

    // resolve hostname
    if (getaddrinfo(hostname.data(), NULL, &hints, &result) != 0) {
        perror("[-]Error in resolving host name");
        return ipAddrs;
    }

    // Iterate through the results and print IP addresses
    for (auto i = result; i != NULL; i = i->ai_next) {
        void* addr;
        char ipstr[INET6_ADDRSTRLEN];

        // Get the pointer to the address itself
        if (i->ai_family == AF_INET) {
            struct sockaddr_in* ipv4 = (struct sockaddr_in*)i->ai_addr;
            addr = &(ipv4->sin_addr);
        } else {
            struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)i->ai_addr;
            addr = &(ipv6->sin6_addr);
        }

        // Convert the IP address to a string
        inet_ntop(i->ai_family, addr, ipstr, sizeof(ipstr));
        ipAddrs.push_back(ipstr);
        std::cout << "IP Address: " << ipstr << std::endl;
    }

    return ipAddrs;
}