#include "raft.h"
namespace raft {

bool Config::Validate()
{
    if (id_ < 0) {
        return false;
    }
    if (heart_beat_tick_ < 0) {
        return false;
    }
    if (election_tick_ <= heart_beat_tick_) {
        return false;
    }
    if (!storage_) {
        return false;
    }
    if (max_inflight_msgs_ <= 0) {
        return false;
    }
    if (!logger_) {
        return false;
    }
    if (read_only_option_ == ReadOnlyLeaseBased && !check_quorum_) {
        return false;
    }
    return true;
}

Raft::Raft(uint64_t id, uint64_t lead, RaftLog* raft_log, uint64_t max_msg_size, int32_t max_inflight, int32_t heart_beat_timeout, int32_t elction_timeout, Logger* logger, bool check_quorum, bool pre_vote, ReadOnly* read_only)
{
    id_ = id;
    lead_ = lead;
    raft_log_ = raft_log;
    max_msg_size_ = max_msg_size;
    max_inflight_ = max_inflight;
    heart_beat_timeout_ = heart_beat_timeout;
    elction_timeout_ = elction_timeout;
    logger_ = logger;
    check_quorum_ = check_quorum_;
    pre_vote_ = pre_vote;
    read_only_ = read_only;
    tick_ = nullptr;
    step_ = nullptr;
}

Raft* Raft::NewRaft(Config* config)
{
    if(!config->Validate())
    {
        return nullptr;
        //TODO
    }
    RaftLog* raft_log = RaftLog::NewLog(config->storage_, config->logger_);
    raftpb::HardState hs;
    raftpb::ConfState cs;
    int32_t ret = config->storage_->InitialState(hs, cs);
    if(ret != 0)
    {
        return nullptr;
        //TODO
    }
    std::vector<uint64_t> peers = config->peers_;
    if(cs.nodes_size() > 0)
    {
        if(!peers.empty())
        {
            return nullptr;
            //TODO
        }
        peers.clear();
        for(int i = 0; i < cs.nodes_size(); i++)
        {
            peers.push_back(cs.nodes(i));
        }
    }
    ReadOnly* read_only = NewReadOnly(config->read_only_option_);
    Raft* rf = new Raft(config->id_, -1, raft_log, config->max_msg_size_, config->max_inflight_msgs_, config->elction_timeout_, config->heart_beat_timeout_, config->logger_, config->check_quorum_, config->pre_vote_, read_only);
    for(auto p: peers)
    {
        raft->peers_[p] = new Progress(1, Inflights::NewInflight(rf->max_inflight_msgs_));
    }
    if(!IsHardStateEmpty(hs))
    {
        rf->LoadState(hs);
    }
    if(config->applied_ > 0)
    {
        raft_log_->AppliedTo(config->applied_);
    }


}


void Raft::BecomeFollower(uint64_t term, uint64_t lead)
{

}

void Raft::LoadState(raftpb::HardState& state)
{
    uint64_t last_index;
    int32_t ret = raft_log_->LastIndex(last_index);
    if(ret != 0)
    {
        return;
        //todo
    }
    if(state.commit() < raft_log_->Commited() || state.commit() > last_index)
    {
        return;
        //todo
    }
    raft_log_->SetCommited(state.commit());
    term_ = state.term();
    vote_ = state.vote();
}

void Raft::StepFollower(raftpb::Message& msg)
{
    switch(msg.type())
    {
        case raftpb::MsgProp:
            if(lead_ == -1)
            {
                logger_->Info("has not leader drop prop");
                return;
            }
            msg.set_to(lead_);
            Send(m);
            break;
        case raftpb::MsgApp:
            elction_timeout_ = 0;
            lead_ = msg.from();
            HandleAppendEntries(msg);
            break;
        case raftpb::MsgHeartbeat:
            elction_timeout_ = 0;
            lead_ = msg.from();
            HandleHeartBeat(msg);
            break;
        case raftpb::MsgSnap:
            elction_timeout_ = 0;
            lead_ = msg.from();
            HandleSnap(msg);
            break;
        case raftpb::MsgTransferLeader:
            if(lead_ == -1)
            {
                logger_->Info("has not leader drop MsgTransferLeader");
                return;
            }
            msg.set_to(lead_);
            Send(m);
            break;
        case raftpb::MsgTimeoutNow:
            if(Promotable())
            {
                Compaign(kCampaignTransfer);
            }else
            {
                logger_->Info("received MsgTimeoutNow, but is not promotable");
            }
            break;
        case raftpb::MsgReadIndex:
            if(lead_ == -1)
            {
                logger_->Info("has not leader drop MsgReadIndex");
                return;
            }
            msg.set_to(lead_);
            Send(m);
            break;
        case raftpb::MsgReadIndexResp:
            if(msg.entries_size() != 1)
            {
                logger_->Info("invalid format of MsgReadIndexResp, entries count not 1");
                return;
            }
            ReadState state(msg.index(), entries(0).data());
            read_states_.push_back(state);
            break;
    }
}

int32_t Raft::Poll(uint64_t id,  const raftpb::Message& msg, bool vote)
{
    if(votes_.find(id) == votes_.end())
    {
        votes_[id] = vote;
    }
    int32_t granted = 0;
    std::for_each(votes_.begin(), votes_.end(), [&](std::pair<uint64_t, bool> &pr){ if(pr.second){ granted++ };});
    return granted;
}

int32_t Raft::Quorum()
{
    reurn peers_.size()/2 + 1;
}

void Raft::StepCandidate(raftpb::Message& msg)
{
     switch(msg.type())
     {
        case raftpb::MsgProp:
            logger_->Info("has not leader drop prop");
            break;
        case raftpb::MsgApp:
            BecomeFollower(msg.term(), msg.from());
            HandleAppendEntries(msg);
            break;
        case raftpb::MsgHeartbeat:
            BecomeFollower(msg.term(), msg.from());
            HandleHeartBeat(msg);
            break;
        case raftpb::MsgSnap:
            BecomeFollower(msg.term(), msg.from());
            HandleSnap(msg);
            break;
        case raftpb::MsgPreVoteResp:
        case raftpb::MsgVoteResp:
            int32_t granted = Poll(msg.from(), msg, !msg.reject());
            int32_t quorum = Quorum();
            if(quorum == granted)
            {
                if(state_ == StatePreCandidate)
                {
                    Compaign(kCampaignElection);
                }else
                {
                    BecomeLeader();
                    BroadcastAppend();
                }
            }else if(quorum == votes_.size() - granted)
            {
                BecomeFollower(term_, -1);
            }
            break;
        case raftpb::MsgTimeoutNow:
            logger_->Info("ignore MsgTimeoutNow");
            break;
     }
}

void Raft::StepLeader(raftpb::Message& msg)
{
    switch(msg.type())
    {
        case raftpb::MsgBeat:
            BroadcastHeartbeat();
            return;
        case raftpb::MsgCheckQuorum:
            if(!CheckQuorumActive())
            {
                BecomeFollower(term_, -1);
            }
            return;
        case raftpb::MsgProp:
            if(msg.entries_size() == 0)
            {
                return;
            }
            if(peers_.find(id_) == peers_.end())
            {
                return;
            }
            if(lead_transferee_ != -1)
            {
                return;
            }
    }
}
}
}
