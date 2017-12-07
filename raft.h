#ifndef __RAFT__H__
#define __RAFT__H__
#include "logger.h"
#include "storage.h"
#include <inttypes.h>
namespace raft {
enum StateType {
    StateFollower = 0,
    StateCandidate = 1,
    StateLeader = 2,
    StatePreCandidate = 3
};

enum ReadOnlyOption {
    ReadOnlySafe = 0,
    ReadOnlyLeaseBased = 1
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
//
//	class Raft
//	{
//		private:
//			uint64_t id_;
//			uint64_t term_;
//			uint64_t vote_;
//
//	};
}
#endif
