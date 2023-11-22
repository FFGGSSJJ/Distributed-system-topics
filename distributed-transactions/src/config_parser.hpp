#include <fstream>
#include <string>
#include <chrono>
#include <vector>
#include <map>

const char delimiter = ' ';

/* node name: [ip, port] */
std::map<std::string, std::vector<std::string>> parser(std::string config_file, int& node_number)
{
    std::map<std::string, std::vector<std::string>> res;

    std::ifstream config(config_file, std::ios::in);
	std::string nodenum;
	std::getline(config, nodenum);
    node_number = std::stoi(nodenum);

    for (int i = 0; i < std::stoi(nodenum); i++) {
        std::vector<std::string> node_info;
		std::string node_name;
		std::string ip;
		std::string port;

		/* read the fields */
		std::getline(config, node_name, delimiter);
		std::getline(config, ip, delimiter);
		std::getline(config, port, '\n');

        /* store */
        node_info.push_back(ip);
        node_info.push_back(port);
        res[node_name] = node_info;
    }

    return res;
}