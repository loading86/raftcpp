#ifndef __UTILIES__H__
#define __UTILIES__H__
#include "raft.pb.h"
#include <vector>
#include <queue>
#include <memory>  
#include <mutex>  
#include <condition_variable> 
namespace raft {
void LimitSize(std::vector<raftpb::Entry>& entries, uint64_t max_size);
bool IsHardStateEqual(raftpb::HardState& left, raftpb::HardState& right);
bool IsHardStateEmpty(raftpb::HardState& state);
int32_t RandomNum(int32_t scale);
bool IsSnapshotEmpty(const raftpb::Snapshot& ss);
raftpb::MessageType VoteRespMsgType(raftpb::MessageType type);
bool IsLocalMsg(raftpb::MessageType type);
bool IsResponseMsg(raftpb::MessageType type);

template<typename T>
class ThreadSafeQueue
{
    private:
        std::mutex mux_;
        std::condition_variable can_push_cond_;
        std::condition_variable can_pop_cond_;
        std::queue<T> queue_;
        uint64_t cap_;
    
    public:
        ThreadSafeQueue(uint64_t cap = UINT64_MAX);
        bool Push(const T& elem);
        bool Pop(T& elem);
        void WaitAndPush(const T& elem);
        void WaitAndPop(T& elem);
        bool Empty() const;
        bool Full() const;
        

};
}
#endif
