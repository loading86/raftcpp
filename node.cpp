#include "node.h"
namespace raft
{
bool Ready::ContainsUpdates()
{
    return soft_state_ != nullptr || !IsHardStateEmpty(hard_state_) || !IsSnapshotEmpty(snapshot_) || !entries_.empty() || !commited_entries_.empty() || !messages_.empty() || !read_only_option_.empty();
}

void node::Stop()
{
    //node_status_.compare_exchange_strong(NodeRunning, NodeStopping, std::memory_order_acquire);
    stop_queue_->Push(1);
    int done_ret = 0;
    done_queue_->WaitAndPop(done_ret);
}


node::node()
{
    prop_queue_ = new ThreadSafeQueue<raftpb::Message>(1);
    recv_queue_ = new ThreadSafeQueue<raftpb::Message>(1);
    confchange_queue_ = new ThreadSafeQueue<raftpb::ConfChange>(1);
    confstate_queue_ = new ThreadSafeQueue<raftpb::ConfState>(1);
    ready_queue_ = new ThreadSafeQueue<Ready>(1);
    advance_queue_ = new ThreadSafeQueue<int>(1);
    prop_queue_ = new ThreadSafeQueue<int>(1);
    tick_queue_ = new ThreadSafeQueue<int>(128);
    done_queue_ = new ThreadSafeQueue<int>(1);
    stop_queue_ = new ThreadSafeQueue<int>(1);
}

void node::Tick()
{
    tick_queue_.Push(1);
}

int32_t node::Campaign()
{
    raftpb::Message msg;
    msg.set_type(raftpb::MsgHup);
    return step(msg);
}

int32_t node::Propose(const std::string& data)
{
    raftpb::Message msg;
    msg.set_type(raftpb::MsgProp);
    msg.set_data(data);
    return step(msg);
}

int32_t node::Step(const raftpb::Message& msg)
{
    if(IsLocalMsg(msg.type()))
    {
        return 0;
    }
    return step(msg);
}

int32_t node::ProposeConfChange(const raftpb::ConfChange& cc)
{
    std::string data;
    cc.SerializeToString(&data);
    raftpb::Message msg;
    msg.set_type(raftpb::MsgProp);
    raftpb::Entry* ent = msg.add_entries();
    ent->set_type(raftpb::EntryConfChange);
    ent->set_data(data);
    Step(msg); 
}

int32_t node::step(const raftpb::Message& msg)
{
    if(msg.type() == raftpb::MsgProp)
    {
        prop_queue_->WaitAndPush(msg);
    }else
    {
        recv_queue_->WaitAndPush(msg);
    }
    return 0;
}

}