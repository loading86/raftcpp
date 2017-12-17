#ifndef __RAFT__H__
#define __RAFT__H__
#include "log.h"
#include "logger.h"
#include "progress.h"
#include "read_only.h"
#include "storage.h"
#include <functional>
#include <inttypes.h>
#include <map>
#include <vector>
namespace raft {
enum StateType {
    StateFollower = 0,
    StateCandidate = 1,
    StateLeader = 2,
    StatePreCandidate = 3
};
struct SoftState {
    uint64_t lead_ = 0;
    StateType raft_state_ = StateFollower;
    SoftState(uint64_t lead, StateType raft_state)
        : lead_(lead)
        , raft_state_(raft_state)
    {
    }
    SoftState(){}
};
const std::string kCampaignPreElection = "CampaignPreElection";
const std::string kCampaignElection = "CampaignElection";
const std::string kCampaignTransfer = "CampaignTransfer";
struct Config {
    uint64_t id_ = 0;
    std::vector<uint64_t> peers_;
    std::vector<uint64_t> learners_;
    int32_t heart_beat_tick_ = 0;
    int32_t election_tick_ = 0;
    Storage* storage_ = nullptr;
    uint64_t applied_ = 0;
    uint64_t max_size_per_msg_ = 0;
    uint64_t max_inflight_msgs_ = 0;
    bool check_quorum_ = false;
    bool pre_vote_ = false;
    ReadOnlyOption read_only_option_ = ReadOnlySafe;
    Logger* logger_ = nullptr;
    bool disable_proposal_forwarding_ = false;

    bool Validate();
};

class Raft {
private:
    uint64_t id_ = 0;
    uint64_t term_ = 0;
    uint64_t vote_ = 0;
    std::vector<ReadState> read_states_;
    RaftLog* raft_log_ = nullptr;
    int32_t max_inflight_ = 0;
    uint64_t max_msg_size_ = 0;
    std::map<uint64_t, Progress*> peers_;
    std::map<uint64_t, Progress*> learner_peers_;
    StateType state_ = StateFollower;
    bool is_learner_ = false;
    std::map<uint64_t, bool> votes_;
    std::vector<raftpb::Message> msgs_;
    uint64_t lead_ = 0;
    uint64_t lead_transferee_ = 0;
    bool pending_conf_ = false;
    ReadOnly* read_only_ = nullptr;
    int32_t election_elapsed_ = 0;
    int32_t heart_beat_elapsed_ = 0;
    bool check_quorum_ = false;
    bool pre_vote_ = false;
    int32_t heart_beat_timeout_ = 0;
    int32_t elction_timeout_ = 0;
    int32_t randomized_elction_timeout_ = 0;
    std::function<void()> tick_ = nullptr;
    std::function<void(raftpb::Message& msg)> step_ = nullptr;
    Logger* logger_ = nullptr;

public:
    Raft(uint64_t id, uint64_t lead, RaftLog* raft_log, uint64_t max_msg_size, int32_t max_inflight, int32_t heart_beat_timeout, int32_t elction_timeout, Logger* logger, bool check_quorum, bool pre_vote, ReadOnly* read_only);
    Raft* NewRaft(Config* config);
    void BecomeFollower(uint64_t term, uint64_t lead);
    void LoadState(raftpb::HardState& state);
    int32_t Step(raftpb::Message& msg);
    void StepFollower(raftpb::Message& msg);
    int32_t Poll(uint64_t id, const raftpb::MessageType& msg_type, bool vote);
    int32_t Quorum();
    void StepCandidate(raftpb::Message& msg);
    void StepLeader(raftpb::Message& msg);
    Progress* GetProgress(uint64_t id);
    void RemoveNode(uint64_t id);
    void DelProgress(uint64_t id);
    void SetProgress(uint64_t id, uint64_t match, uint64_t next, bool is_learner);
    void FromLearnerToVoter(uint64_t id);
    void AddNodeOrLearnerNode(uint64_t id, bool is_learner);
    void AddNode(uint64_t id);
    void AddLearner(uint64_t id);
    bool Promotable(uint64_t id);
    void RestPendingConf();
    bool PastElectionTimeOut();
    void ResetRandomizedElctionTimeout();
    bool CheckQuorumActive();
    void SendTimeoutNow(uint64_t to);
    void Send(raftpb::Message& msg);
    void AbortLeaderTransfer();
    int32_t NumOfPendingConf(std::vector<raftpb::Entry>& entries);
    void HandleAppendEntries(raftpb::Message& msg);
    void HandleHeartBeat(raftpb::Message& msg);
    void HandleSnap(raftpb::Message& msg);
    bool Restore(const raftpb::Snapshot& snap);
    bool HasLeader();
    SoftState* GetSoftState();
    raftpb::HardState GetHardState();
    std::vector<uint64_t> Nodes();
    void AllNodesExceptMe(std::vector<uint64_t>& nodes);
    void SendAppend(uint64_t to);
    void SendHeartbeat(uint64_t to, const std::string& ctx);
    void BroadcastAppend();
    void BroadcastHeartbeatWithCtx(const std::string& ctx);
    void BroadcastHeartbeat();
    bool MaybeCommit();
    void Reset(uint64_t term);
    void AppendEntries(std::vector<raftpb::Entry>& entries);
    void TickElection();
    void TickHeartbeat();
    void BecomeCandidate();
    void BecomePreCandidate();
    void BecomeLeader();
    void Campaign(const std::string& compaign_type);
    uint64_t GetID(){return id_;}
    uint64_t GetLeadTransferee(){return lead_transferee_;}
    RaftLog* GetRaftLog(){return raft_log_;}
    void AllProgress(std::map<uint64_t, Progress*>& prog);
    void RestoreNode(std::vector<uint64_t>& nodes, bool is_learner);
};
}
#endif
