#include <mutex>
#include <queue>
#include <iostream>
#include <string>
#include <algorithm>

const uint8_t deliverable = 0xFF;
const uint8_t undeliverable = 0x00;

class SyncMsgQueue {
private:
    std::mutex _mutex;
    std::deque<std::string> _queue;
public:
    // SyncMsgQueue();
    // ~SyncMsgQueue();

    void push_back(const std::string& value) { 
        std::unique_lock<std::mutex> mlock(_mutex);
        _queue.push_back(value);
    }

    void pop_front() {
        std::unique_lock<std::mutex> mlock(_mutex);
        if (!_queue.empty())
            _queue.pop_front();
    }

    std::string front() {
        std::unique_lock<std::mutex> mlock(_mutex);
        return _queue.front();
    }

    size_t size() { 
        std::unique_lock<std::mutex> mlock(_mutex);
        return _queue.size(); 
    }

    bool empty() {
        std::unique_lock<std::mutex> mlock(_mutex);
        return _queue.empty(); 
    }
};

class SyncDeliverQueue : public SyncMsgQueue {
private:
    std::mutex _mutex;
    std::deque<std::string> _queue;
public:
    bool update_msg(uint16_t msg_id, uint16_t final_seq_num, int node_id) {
        std::unique_lock<std::mutex> mlock(_mutex);
        if (empty()) 
            return false;
        
        for (int i = 0; i < _queue.size(); i++) {
            if ((uint16_t)(_queue[i].c_str()[3]) == msg_id) {
                std::cout << "[DEBUG]: prev priority::" <<  (uint16_t)(((_queue[i][0] << 8)&0xFF00) | (_queue[i][1]&0x00FF)) << " delv::" <<  (uint16_t)_queue[i][2] << std::endl;
                _queue[i][0] = (char) (((final_seq_num*10+node_id) >> 8) & 0x00FF);
                _queue[i][1] = (char) (((final_seq_num*10+node_id)) & 0x00FF);
                _queue[i][2] = deliverable;
                std::cout << "[DEBUG]: f::" <<  final_seq_num << " n::" << node_id << std::endl;
                std::cout << "[DEBUG]: curr priority::" <<  (uint16_t)(((_queue[i][0] << 8)&0xFF00) | (_queue[i][1]&0x00FF)) << " delv::" <<  (uint16_t)_queue[i][2] << std::endl;
                break;
            }
        }

        // sort the deque
        std::sort(_queue.begin(), _queue.end());
        return true;
    }

    std::vector<std::string> get_deliverable_msg() {
        std::unique_lock<std::mutex> mlock(_mutex);
        std::vector<std::string> deliverable_msgs;
        int original_size = _queue.size();

        for (int i = 0; i < original_size; i++) {
            if ((uint8_t)_queue.front()[2] == deliverable) {
                deliverable_msgs.push_back(std::move(_queue.front()));
                _queue.pop_front();
            } else {
                _queue.push_back(std::move(_queue.front()));
                _queue.pop_front();
            }
        }

        return deliverable_msgs;
    }

    std::string update_and_deliver(uint16_t msg_id, uint16_t final_seq_num, int node_id) {
        std::unique_lock<std::mutex> mlock(_mutex);

        std::string deliverable_msg;
        int original_size = _queue.size();

        if (empty()) 
            return deliverable_msg;
        
        for (int i = 0; i < _queue.size(); i++) {
            if ((uint16_t)(_queue[i].c_str()[3]) == msg_id) {
                std::cout << "[DEBUG]: prev priority::" <<  (uint16_t)(((_queue[i][0] << 8)&0xFF00) | (_queue[i][1]&0x00FF)) << " delv::" <<  (uint16_t)_queue[i][2] << std::endl;
                _queue[i][0] = (char) (((final_seq_num*10+node_id) >> 8) & 0x00FF);
                _queue[i][1] = (char) (((final_seq_num*10+node_id)) & 0x00FF);
                _queue[i][2] = deliverable;
                std::cout << "[DEBUG]: f::" <<  final_seq_num << " n::" << node_id << std::endl;
                std::cout << "[DEBUG]: curr priority::" <<  (uint16_t)(((_queue[i][0] << 8)&0xFF00) | (_queue[i][1]&0x00FF)) << " delv::" <<  (uint16_t)_queue[i][2] << std::endl;
                break;
            }
        }

        // sort the deque
        std::sort(_queue.begin(), _queue.end());

        // get deliverable msgs
        if ((uint8_t)_queue.front()[2] == deliverable) {
            deliverable_msg = _queue.front();
            _queue.pop_front();
        } 

        return deliverable_msg;
    }
};

SyncMsgQueue transaction_queue;