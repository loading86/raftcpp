#ifndef __NODE__H__
#define __NODE__H__
#include <vector>
namespace raft
{
    enum SnapshotStatus
    {
        SnapshotFinish = 1,
        SnapshotFailure = 2
    };

    struct Ready 
    {
        SoftState* soft_state_;
        raftpb::HardState hard_state_;
        std::vector<ReadState> read_states_;
        std::vector<raftpb::Entry> entries_;
        raftpb::Snapshot snapshot_;
        std::vector<raftpb::Entry> commited_entries_;
        std::vector<raftpb::Message> messages_;
        bool must_sync_;
        bool ContainsUpdates();
    };

    class Node
    {
        void Tick();

    }


}
#endif