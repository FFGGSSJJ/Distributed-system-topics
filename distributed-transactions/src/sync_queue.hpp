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

    void update_msg(uint16_t msg_id, uint16_t final_seq_num, int node_id) {
        std::unique_lock<std::mutex> mlock(_mutex);
        
        for (int i = 0; i < _queue.size(); i++) {
            if ((uint16_t)(_queue[i].c_str()[3]) == msg_id) {
                _queue[i][0] = (char) (((final_seq_num*10+node_id) >> 8) & 0x00FF);
                _queue[i][1] = (char) (((final_seq_num*10+node_id)) & 0x00FF);
                _queue[i][2] = deliverable;
                break;
            }
        }

        // sort the deque
        std::sort(_queue.begin(), _queue.end());
        return;
    }

    std::string update_and_deliver(uint16_t msg_id, uint16_t final_seq_num, int node_id) {

        std::unique_lock<std::mutex> mlock(_mutex);

        std::string deliverable_msg;
        int original_size = _queue.size();
        
        for (int i = 0; i < _queue.size(); i++) {
            if ((uint16_t)(_queue[i].c_str()[3]) == msg_id) {
                _queue[i][0] = (char) (((final_seq_num*10+node_id) >> 8) & 0x00FF);
                _queue[i][1] = (char) (((final_seq_num*10+node_id)) & 0x00FF);
                _queue[i][2] = deliverable;
                break;
            }
        }

        // sort the deque
        std::sort(_queue.begin(), _queue.end());

        // get deliverable msgs
        if (!_queue.empty() && (uint8_t)_queue.front()[2] == deliverable) {
            deliverable_msg.append(_queue.front().c_str());
            _queue.pop_front();
        }

        return deliverable_msg;
    }
};