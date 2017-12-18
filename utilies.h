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

template<typename T>
class ThreadSafeQueue
{
    private:
        std::mutex mux_;
        std::condition_variable cond_;
        std::queue<T> queue_;
    
    public:
        ThreadSafeQueue();
        void Push(const T& elem);
        bool Empty() const;
        bool TryPop(T& elem);
        void WaitAndPop(T& elem);

};
}
#endif
