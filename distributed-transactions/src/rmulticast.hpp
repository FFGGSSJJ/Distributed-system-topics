#include <thread>
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <vector>
#include <map>
#include <mutex>
#include <queue>
#include <algorithm>
#include <arpa/inet.h>

#include "sync_queue.hpp"

/* mutex to guarantee thread safety */
std::mutex nodemap_mutex;
std::mutex seq_mutex;

static SyncMsgQueue transaction_queue;  // shared memory between application and multicast
static SyncMsgQueue msg_queue;
static SyncMsgQueue deliver_queue;
static uint16_t seq_num;
static uint16_t msg_unq_id;

const int recv_size = 4096;
const char msg_delimiter = 0xEE;

enum INFO_TYPE
{
    MSG,
    INIT,
    PROPS_SEQ_NUM,
    FINAL_SEQ_NUM
};


/* helper functions */

/* parse the received messages from server nodes */
std::vector<std::string> parse_recvmsgs(std::string& msg) 
{
    std::vector<std::string> res;
    while (!msg.empty()) {
        std::string submsg = msg.substr(0, msg.find_first_of(msg_delimiter));
        msg.erase(0, msg.find_first_of(msg_delimiter)+1);
        res.push_back(submsg);
    }

    return res;
}



/* pack msg into the packet that is to be pushed into dekiver queue*/
/* 0++++++1+++++++2++++++3+++++++4+++++++++++++++++n
 * |   Priority   |Delive| MsgID |     Message     |
 * +++++++++++++++++++++++++++++++++++++++++++++++++
*/
void pack_msg2deliver(std::string& msg2deliver, std::string msg, uint16_t unq_id, uint16_t priority, uint8_t flag) 
{
    msg2deliver.push_back((char)((priority >> 8) & 0x00FF));
    msg2deliver.push_back((char)((priority) & 0x00FF));
    msg2deliver.push_back(flag);
    msg2deliver.push_back((char)unq_id);
    msg2deliver.append(msg);
    return;
}

/* pack msg into the packet that is to be sent */
// TODO:: Seq# currently takes 1 byte
/* 0++++++1+++++++2++++++++++++++4++++++++5+++++++++++++++++n
 * | TYPE | Node# |     Seq#     | Msg ID |     Message     |
 * +++++++++++++++|        optional       |++++++++++++++++++
*/
void pack_msg2send(std::string& msg2send, std::string msg, int8_t msgtype, uint8_t node_id, uint16_t unq_id = 0, uint16_t seq = 0)
{
    msg2send.push_back(msgtype);
    msg2send.push_back(node_id);

    if (msgtype == FINAL_SEQ_NUM || msgtype == PROPS_SEQ_NUM) {
        msg2send.push_back((char)seq);
        msg2send.push_back((char)unq_id);
    }

    msg2send.append(msg);
    msg2send.push_back(msg_delimiter);
    return;
}


/* handle received message with type MSG */
int message_handler(std::string msg, std::map<std::string, int>& nodes)
{
    std::cout << "msg handler" << msg.size() << std::endl;

    /* reply with proposed seq num */
    std::string nodename = "node";
    int node_id = (int)msg.c_str()[1];
    nodename.push_back((char)(node_id + '0'));

    /* update seq num and msg id */
    while(!seq_mutex.try_lock());
    seq_num += 1;
    msg_unq_id += 1;
    uint16_t msg_id = msg_unq_id;
    uint16_t prop_seq = seq_num;
    seq_mutex.unlock();

    /* push msg into queue */
    std::string mainmsg = msg.substr(2);
    std::string msg2deliver;
    std::string propmsg;

    /* pack message */
    pack_msg2send(propmsg, "", PROPS_SEQ_NUM, 0, msg_id, prop_seq);
    pack_msg2deliver(msg2deliver, mainmsg, msg_id, (uint16_t)prop_seq*10 + node_id, undeliverable);

    /* push */
    deliver_queue.push_back(msg2deliver);

    /* reply */
    int ret = 0;
    while (ret <= 0) {
        ret = send(nodes[nodename], propmsg.c_str(), (size_t)propmsg.size(), 0);
        std::cout <<"[DEBUG]: reply prop::" << (uint16_t)propmsg.c_str()[2] << " msgid::" << (uint16_t)propmsg.c_str()[3] << std::endl;
    }

    if (ret < 0) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}


/* handle received message with type FINAL_SEQ_NUM */
int finalseq_handler(std::string msg)
{   
    std::cout << "final handler" << std::endl;

    int msg_id = (int)msg.c_str()[3];
    int final_seq_num = (int)msg.c_str()[2];
    int node_id = (int)msg.c_str()[1];

    /* update seq num */
    while(!seq_mutex.try_lock());
    seq_num = final_seq_num > seq_num ? final_seq_num : seq_num;
    seq_mutex.unlock();

    /* update the msg in deliver queue */
    deliver_queue.update_msg(msg_id, final_seq_num, node_id);

    if ((uint8_t)deliver_queue.front()[2] == deliverable) {
        transaction_queue.push_back(std::move(deliver_queue.front()));
        deliver_queue.pop_front();
    }

    return EXIT_SUCCESS;
}




/* thread fuinctions */
/**
 * @brief thread function for listening client nodes
 * @param sockfd: sockfd for client node
*/
void client_node_listening(int sockfd, std::map<std::string, int>& nodes)
{
    /* init data buffer */
	char buffer[recv_size];
    memset(buffer, 0, (size_t)recv_size);

    /* receive node info */
    int ret = recv(sockfd, buffer, (size_t)recv_size, 0);
    std::string info = buffer;
    
    if (buffer[0] == INIT) {
        while(!nodemap_mutex.try_lock());
        std::cout << "Add " << info.substr(info.find(' ')+1) << " to map" << std::endl;
        nodes[info.substr(info.find(' ')+1)] = sockfd;
        nodemap_mutex.unlock();
    }

    /* keep listenning the client node */
	while (true) {
		ret = recv(sockfd, buffer, (size_t)recv_size, 0);

        /* check recv bytes */

        // if error happened in client node
		if (ret < 0) {
            std::cout << "[INFO] node error " << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
            continue;
        }
        
        // if client node disconnected
        if (ret == 0) {
            std::cout << std::to_string(std::chrono::duration_cast<std::chrono::duration<double> >(std::chrono::system_clock::now().time_since_epoch()).count()) 
                << " " << info.substr(info.find(' ')+1) <<" - disconnected" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
            continue;
        }
	
        /* prepare msg */
        std::string msg;
        for (int i = 0; i < ret; i++) {
            msg.push_back(buffer[i]);
        }

        /* parse msg to avoid error */
        std::vector<std::string> msgs = parse_recvmsgs(msg);

        /* insert into message queue */
        for (int i = 0; i < (int)msgs.size(); i++)
            msg_queue.push_back(std::move(msgs[i]));
		
        /* next loop */
        memset(buffer, 0, (size_t)recv_size);
	}
	return;
}




/* handling msg from the server nodes */
void rmulti_recv(int node_num, std::map<std::string, int>& nodes)
{
    /* wait... */
    while ((int)nodes.size() < node_num)
        std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout << "[INFO]: Multicast Revc Started" << std::endl;

    /* initialize seq num*/
    while(!seq_mutex.try_lock());
    seq_num = 0;
    msg_unq_id = 0;
    seq_mutex.unlock();

    /* handle message queue */
    while (true)
    {
        if (msg_queue.empty()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        /* get msg */
        std::string msg = msg_queue.front();
        msg_queue.pop_front();

        /* switch msg handler */
        switch (msg.c_str()[0])
        {
        case MSG:
            message_handler(msg, nodes);
            break;
        case FINAL_SEQ_NUM:
            finalseq_handler(msg);
            break;
        default:
            break;
        }

    }
    return;
}


/* cast msg as server node */
void rmulti_cast(int node_id, int node_num, std::map<std::string, int>& nodes)
{
    /* wait... */
    while ((int)nodes.size() < node_num)
        std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout <<"[INFO]: Multicast Sender Started" << std::endl;

    /* keep track of node situations */
    std::map<std::string, bool> node_status;
    for (auto i = nodes.cbegin(); i != nodes.cend(); i++) {
        node_status[i->first] = true;
    }

    /* start broad cast */
    while (true) {
        // receive message
        std::string msg, msg2queue, msg2send;
        std::string msg_type;
        std::string account1, account2, amount, temp;

        std::cin >> msg_type;
        if (msg_type == "DEPOSIT") {
            // read msg
            std::cin >> account1 >> amount;
            std::cout << msg_type << " " << account1 << " " << amount << std::endl;
            msg.append(msg_type);
            msg.push_back(' ');
            msg.append(account1);
            msg.push_back(' ');
            msg.append(amount);
        } else if (msg_type == "TRANSFER") {
            std::cin >> account1 >> temp >> account2 >> amount;
            std::cout << msg_type << " " << account1 << " " << account2 << " " << amount << std::endl;
            msg.append(msg_type);
            msg.push_back(' ');
            msg.append(account1);
            msg.push_back(' ');
            msg.append(account2);
            msg.push_back(' ');
            msg.append(amount);
        } else {
            continue;
        }

        // get self proposed seq
        while(!seq_mutex.try_lock());
        seq_num += 1;
        msg_unq_id += 1;
        uint16_t proposed_seq = seq_num;
        uint16_t self_unq_id = msg_unq_id;
        seq_mutex.unlock();

        // pack msg
        pack_msg2deliver(msg2queue, msg, self_unq_id, (uint16_t)proposed_seq*10 + node_id, undeliverable);
        pack_msg2send(msg2send, msg, MSG, node_id);

        // push msg to deliver queue
        deliver_queue.push_back(msg2queue);

        // cast msg to all nodes
        int majority_cnt = 0;
        uint16_t final_seq = proposed_seq;
        // unq_ids is used to keep track of the msg_id of each node, as it may differ by nodes
        std::map<std::string, uint16_t> unq_ids;

        // loop node lists to multicast messages
        for (auto i = nodes.cbegin(); i != nodes.cend(); i++) {
            int ret = 0;
            char buffer[recv_size];
		    while (ret <= 0) {
                if (!node_status[i->first])
                    break;
			    ret = send(i->second, msg2send.c_str(), (size_t)msg2send.size(), 0);
                std::cout << (int)msg2send.c_str()[0] << (int)msg2send.c_str()[1] << ret << std::endl;
		    }
            std::cout <<"[INFO]: Multicast Send Messages" << std::endl;

            // wait for reply
            ret = recv(i->second, buffer, (size_t)recv_size, 0);
            
            // if the node fails, continue to the next node
            if (ret <= 0) {
                std::cout <<"[INFO]: " << i->first << " Fails" << std::endl;
                node_status[i->first] = false;
                continue;
            }

            // if the node succeeds
            node_status[i->first] = true;
            std::cout <<"[INFO]: Multicast Get Proposed Seq#" << std::endl;
            
            // update final seq num
            if (buffer[0] == PROPS_SEQ_NUM) {
                majority_cnt++;
                unq_ids[i->first] = (uint16_t)buffer[3];
                final_seq = final_seq <= (uint16_t)buffer[2] ? (uint16_t)buffer[2] : final_seq;
            }
        }

        // if majority not reached, skip this msg
        // not necessary to update deliver_queue as the msg is marked as undeliverable
        // and for other nodes this msg will be marked as undeliverable also
        if (majority_cnt < node_num/2) {
            std::cout <<"[INFO]: Majority Not Reached" << std::endl;
            continue;
        }

        // update seq num
        while(!seq_mutex.try_lock());
        seq_num = final_seq;
        seq_mutex.unlock();

        // cast final seq to all nodes
        for (auto i = nodes.cbegin(); i != nodes.cend(); i++) {
            // skip the failed node
            if (!node_status[i->first])
                continue;

            // repack msg
            msg2send.clear();
            pack_msg2send(msg2send, msg, FINAL_SEQ_NUM, node_id, unq_ids[i->first], final_seq);

            int ret = send(i->second, msg2send.c_str(), (size_t)msg2send.size(), 0);
            if (ret <= 0) 
                node_status[i->first] = false;
        }

        // update deliver queue
        deliver_queue.update_msg(self_unq_id, final_seq, node_id);

        // inform application layer
        if ((uint8_t)deliver_queue.front()[2] == deliverable) {
            transaction_queue.push_back(std::move(deliver_queue.front()));
            deliver_queue.pop_front();
        }
    }
    return;
}