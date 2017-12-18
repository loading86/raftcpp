#include "node.h"
namespace raft
{
bool Ready::ContainsUpdates()
{
    return soft_state_ != nullptr || !IsHardStateEmpty(hard_state_) || !IsSnapshotEmpty(snapshot_) || !entries_.empty() || !commited_entries_.empty() || !messages_.empty() || !read_only_option_.empty();
}

void node::Stop()
{
    node_status_.compare_exchange_strong(NodeRunning, NodeStopping, std::memory_order_acquire);
}


}