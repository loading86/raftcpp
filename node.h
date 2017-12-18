#ifndef __NODE__H__
#define __NODE__H__
#include <vector>
#include <string>
#include <atomic>
namespace raft
{
    enum SnapshotStatus
    {
        SnapshotFinish = 1,
        SnapshotFailure = 2
    };

    enum NodeStatus
    {
        NodeRunning = 1,
        NodeStopping = 2,
        NodeStopped = 3
    }

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
        int32_t Campaign();
        int32_t Propose(const std::string& data);
        int32_t ProposeConfChange(const raftpb::ConfChange& cc);
        int32_t Step(const raftpb::Message& msg);
        void Advance();
        raftpb::ConfState* ApplyConfChange(const raftpb::ConfChange& cc);
        void TransferLeadership(uint64_t lead, uint64_t transferee);
        int32_t ReadIndex(const std::string& read_contex);
        Status GetStatus();
        void ReportUnreachable(uint64_t id);
        void ReportSnapshot(uint64_t id, SnapshotStatus status);
        void Stop();
        Ready* GetReady();
    };

    enum ReadyQueueNodeType
    {
        ReadyQueueNodeProposal = 1,
        ReadyQueueNodeRecv = 2,
        ReadyQueueNodeConf = 3,
        ReadyQueueNodeConfState = 4,
        

    };

    struct ReadyQueueNode
    {
        int32_t type_;
    }

    class node: public Node 
    {
        private:
            std::atomic_int node_status_;
            
    }


}
#endif