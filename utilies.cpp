#include "utilies.h"
#include <time.h>
#include <algorithm>
#include <random>
namespace raft {
void LimitSize(std::vector<raftpb::Entry>& entries, uint64_t max_size)
{
    if (entries.empty()) {
        return;
    }
    uint64_t size = 0;
    uint64_t index = 0;
    for (; index < entries.size(); index++) {
        size += entries[index].ByteSize();
        if (size > max_size) {
            break;
        }
    }
    if (index == entries.size()) {
        return;
    }
    entries.erase(entries.begin() + index, entries.end());
}

bool IsHardStateEqual(raftpb::HardState& left, raftpb::HardState& right)
{
    return left.term() == right.term() && left.vote() == right.vote() && left.commit() == right.commit();
}

bool IsHardStateEmpty(raftpb::HardState& state)
{
    return state.term() == 0 && state.vote() == 0 && state.commit() == 0;
}

bool IsSnapshotEmpty(const raftpb::Snapshot& ss)
{
    return ss.metadata().index() == 0;
}

bool IsLocalMsg(raftpb::MessageType type)
{
    return type == raftpb::MsgHup || type == raftpb::MsgBeat || type == raftpb::MsgUnreachable || type == raftpb::MsgSnapStatus || type == raftpb::MsgCheckQuorum;
}

bool IsResponseMsg(raftpb::MessageType type)
{
    return type == raftpb::MsgAppResp || type == raftpb::MsgVoteResp || type == raftpb::MsgHeartbeatResp || type == raftpb::MsgUnreachable || type == raftpb::MsgPreVoteResp;
}

std::random_device rd;
int32_t RandomNum(int32_t scale)
{
    return rd() % scale;
}

raftpb::MessageType VoteRespMsgType(raftpb::MessageType type)
{
    if(type == raftpb::MsgVote)
    {
        return raftpb::MsgVoteResp;
    }else if(type == raftpb::MsgPreVote)
    {
        return raftpb::MsgPreVoteResp;
    }else
    {
        exit(1);
    }
}

template<typename T> ThreadSafeQueue<T>::ThreadSafeQueue(uint64_t cap)
{
    cap_ = cap;
}

bool template<typename T> ThreadSafeQueue<T>::Push(const T& elem)
{
    std::lock_guard<std::mutex> lock(mux_);
    if(queue_.size() == cap_)
    {
        return false;
    }
    queue_.push(elem);
    can_pop_cond_.notify_one();
    return true;
}

bool template<typename T> ThreadSafeQueue<T>::Pop(T& elem)
{
    std::lock_guard<std::mutex> lock(mux_);
    if(queue_.empty())
    {
        return false;
    }
    elem = queue_.front();
    queue_.pop();
    can_push_cond_.notify_one();
    return true;
}

void template<typename T> ThreadSafeQueue<T>::WaitAndPush(const T& elem)
{
    std::unique_lock<std::mutex> lock(mux_);
    can_pop_cond_.wait(lock, [this]{return queue_.size() < cap_;});
    queue_.push_back(elem);
    can_pop_cond_.notify_one();
}

void template<typename T> ThreadSafeQueue<T>::WaitAndPop(T& elem)
{
    std::unique_lock<std::mutex> lock(mux_);
    can_pop_cond_.wait(lock, [this]{return !queue_.empty();});
    elem = queue_.front();
    queue_.pop();
    can_push_cond_.notify_one();
}

bool template<typename T> ThreadSafeQueue<T>::Empty() const
{
    std::lock_guard<std::mutex> lock(mux_);
    if(queue_.empty())
    {
        return true;
    }
    return false;
}

bool template<typename T> ThreadSafeQueue<T>::Full() const
{
    std::lock_guard<std::mutex> lock(mux_);
    if(queue_.size() == cap_)
    {
        return true;
    }
    return false;
}



}