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
struct SoftState
{
    uint64_t lead_;
    StateType raft_state_;
    SoftState(uint64_t lead, StateType raft_state):lead_(lead),raft_state_(raft_state){}
};
const std::string kCampaignPreElection = "CampaignPreElection";
const std::string kCampaignElection = "CampaignElection";
const std::string kCampaignTransfer = "CampaignTransfer";
struct Config {
    uint64_t id_;
    std::vector<uint64_t> peers_;
    std::vector<uint64_t> learners_;
    int32_t heart_beat_tick_;
    int32_t election_tick_;
    Storage* storage_;
    uint64_t applied_;
    uint64_t max_size_per_msg_;
    uint64_t max_inflight_msgs_;
    bool check_quorum_;
    bool pre_vote_;
    ReadOnlyOption read_only_option_;
    Logger* logger_;
    bool disable_proposal_forwarding_;

    bool Validate();
};

class Raft {
private:
    uint64_t id_;
    uint64_t term_;
    uint64_t vote_;
    std::vector<ReadState> read_states_;
    RaftLog* raft_log_;
    int32_t max_inflight_;
    uint64_t max_msg_size_;
    std::map<uint64_t, Progress*> peers_;
    std::map<uint64_t, Progress*> learner_peers_;
    StateType state_;
    bool is_learner_;
    std::map<uint64_t, bool> votes_;
    std::vector<raftpb::Message> msgs_;
    uint64_t lead_;
    uint64_t lead_transferee_;
    bool pending_conf_;
    ReadOnly* read_only_;
    int32_t election_elapsed_;
    bool check_quorum_;
    bool pre_vote_;
    int32_t heart_beat_timeout_;
    int32_t elction_timeout_;
    int32_t randomized_elction_timeout_;
    std::function<void()> tick_;
    std::function<void(raftpb::Message& msg)> step_;
    Logger* logger_;
public:
    Raft(uint64_t id, uint64_t lead, RaftLog* raft_log, uint64_t max_msg_size, int32_t max_inflight, int32_t heart_beat_timeout, int32_t elction_timeout, Logger* logger, bool check_quorum, bool pre_vote, ReadOnly* read_only);
    static Raft* NewRaft(Config* config);
    void LoadState(raftpb::HardState& state);
    void StepFollower(raftpb::Message& msg);
    void StepCandidate(raftpb::Message& msg);
    void StepLeader(raftpb::Message& msg);
};
}
#endif
