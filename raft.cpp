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

int32_t Raft::Step(raftpb::Message& msg)
{
    if(msg.term() > term_)
    {
        if(msg.type() == raftpb::MsgVote || msg.type() == raftpb::MsgPreVote)
        {
            bool force = msg.context() == kCampaignTransfer;
            bool in_lease = check_quorum_ && lead_ != 0 && election_elapsed_ < elction_timeout_;
            if((!force) && in_lease)
            {
                return 0;
            }
        }
        if(msg.type() == raftpb::MsgPreVote || (msg.type() == raftpb::MsgPreVoteResp && !msg.reject()) )
        {
        }else
        {
            if(msg.type() == raftpb::MsgApp || msg.type() == raftpb::MsgHeartbeat || msg.type() == raftpb::MsgSnap)
            {
                BecomeFollower(term_, msg.from());
            }else
            {
                BecomeFollower(term_, 0);
            }
        }
    }else if(msg.term() < term_)
    {
        if(check_quorum_ && (msg.type() == raftpb::MsgApp || msg.type() == raftpb::MsgHeartbeat))
        {
            raftpb::Message msg;
            msg.set_to(msg.from());
            msg.set_type(raftpb::MsgAppResp);
            Send(msg);
        }
        return 0;
    }
    switch(msg.type())
    {
        case raftpb::MsgHup:
            if(state_ != StateLeader)
            {
                std::vector<raftpb::Entry> entries;
                int32_t ret = raft_log_->Slice(raft_log_->Applied() + 1, raft_log_->Commited() + 1, UINT64_MAX, entries);
                if(ret != 0)
                {
                    exit(1);
                }
                int32_t num_of_pending_conf = NumOfPendingConf(entries);
                if(num_of_pending_conf != 0 && raft_log_->Commited() > raft_log_->Applied())
                {
                    return 0;
                }
                if(pre_vote_)
                {
                    Compaign(kCampaignElection);
                }else
                {
                    Compaign(kCampaignPreElection);
                }
            }
            break;
        case raftpb::MsgVote:
        case raftpb::MsgPreVote:
            if(is_learner_)
            {
                return 0;
            }
            if((vote_ == 0 || msg.term() > term_ || vote_ == msg.from()) && raft_log_->IsUpToDate(msg.index(), msg.logterm()))
            {
                raftpb::Message send;
                send.set_to(msg.from());
                send.set_term(msg.term());
                send.set_type(VoteRespMsgType(msg.type()));
                Send(send);
                if(msg.type() == raftpb::MsgVote)
                {
                    election_elapsed_ = 0;
                    vote_ = msg.from();
                }
            }else
            {
                raftpb::Message send;
                send.set_to(msg.from());
                send.set_term(msg.term());
                send.set_type(VoteRespMsgType(msg.type()));
                send.set_reject(true);
                Send(send);
            }
            break;
        default:
            step_(msg);
    }
    return 0;
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
            int32_t entry_num = msg.entries_size();
            std::vector<raftpb::Entry> entries;
            for(int i = 0; i < entry_num; i++)
            {
                raftpb::Entry* entry = msg.mutable_entries(i);
                if(entry->type() == raftpb::EntryConfChange)
                {
                    if(pending_conf_)
                    {
                        raftpb::Entry replace_entry;
			            replace_entry.set_type(raftpb::EntryNormal);
			            *entry = replace_entry;
                    }
		            pending_conf_ = true;
                }
                entries.push_back(*entry);
            }
            AppendEntries(entries);
            BroadcastAppend();
            return;
       case raftpb::MsgReadIndex:
            if(Quorum() > 1)
            {
                uint64_t term;
                int32_t ret = raft_log_->Term(raft_log_->Commited(), term);
                if(raft_log_->ZeroTermOnErrCpmpacted(term, ret) != term_)
                {
                    return;
                }
                switch(read_only_->Option())
                {
                    case ReadOnlySafe:
                        read_only_->AddRequest(raft_log_->Commited(), msg);
                        BroadcastHeartbeatWithCtx(msg.entry(0).data());
                        break;
                    case ReadOnlyLeaseBased:
                        uint64_t raft_commited = raft_log_->Commited();
                        if(msg.from() == 0 || msg.from() == id_)
                        {
                            ReadState state(raft_commited, msg.entry(0).data());
                            read_states_.push_back(state);
                        }else
                        {
                            raftpb::Message meg_send;
                            meg_send.set_to(msg.from());
                            msg_send.set_type(raftpb::MsgReadIndexResp);
                            msg_send.set_index(raft_commited);
                            for(int i = 0; i < msg.entries_size(); i++)
                            {
                                raftpb::Entry* entry = msg_send.add_entries();
                                *entry = msg.entry(i);
                            }

                        }
                }
            }else
            {
                ReadState state(raft_log_->Commited(), msg.entry(0).data());
                read_states_.push_back(state);
            }
            return;
       case default:
            break;
    }

    Progress* pr = GetProgress(msg.from());
    if(!pr)
    {
        logger_->Info("no progress available");
        return;
    }
    switch(msg.type())
    {
        case raftpb::MsgAppResp:
            pr->SetRecentActive(true);
            if(msg.reject())
            {
                if(pr->MaybeDecrTo(msg.index(), msg.rejecthint()))
                {
                    if(pr->State() == ProgressStateReplicate)
                    {
                        pr->BecomeProbe();
                    }
                    SendAppend(msg.from());
                }
            }else
            {
                bool old_paused = pr->IsPaused();
                if(pr->MaybeUpdate(msg.index()))
                {
                    if(pr->State() == ProgressStateProbe)
                    {
                        pr->BecomeReplicate();
                    }else if(pr->State() == ProgressStateSnapshot && pr->NeedSnapshotAbort())
                    {
                        pr->BecomeProbe();
                    }else if(pr->State() == ProgressStateReplicate)
                    {
                        pr->GetInflights()->FreeTo(msg.index());
                    }
                    if(MaybeCommit())
                    {
                        BroadcastAppend();
                    }else if(old_paused)
                    {
                        SendAppend(msg.from());
                    }
                    if(msg.from() == lead_transferee_ && pr->Match() == raft_log_->LastIndex())
                    {
                        SendTimeoutNow(msg.from());
                    }
                }
            }
            break;
         case raftpb::MsgHeartbeatResp:
            pr->SetRecentActive(true);
            pr->Resume();
            if(pr->State() == ProgressStateReplicate && pr->GetInflights()->Full())
            {
                pr->GetInflights()->FreeFirstOne();
            }
            if(pr->Match() < raft_log_->LastIndex())
            {
                SendAppend(msg.from());
            }
            if(read_only_->Option() != ReadOnlySafe || msg.context().empty())
            {
                return;
            }
            int32_t ack_count = read_only_->RecvAck(msg);
            if(ack_count < Quorum())
            {
                return;
            }
            std::vector<ReadIndexStatus*> status;
            read_only_->Advance(msg, status);
            for(auto& st: status)
            {
                raftpb::Message req = st.GetRequest();
                if(req.from() == 0 || req.fro,() == id_)
                {
                    ReadState state(req.index(), req.entry(0).data());
                    read_states_.push_back(state);
                }else
                {
                    raftpb::Message meg_send;
                    meg_send.set_to(req.from());
                    msg_send.set_type(raftpb::MsgReadIndexResp);
                    msg_send.set_index(req.index());
                    for(int i = 0; i < req.entries_size(); i++)
                    {
                        raftpb::Entry* entry = msg_send.add_entries();
                        *entry = req.entry(i);
                    }
                    Send(msg_send);
                }
            }
            break;
        case raftpb::MsgSnapStatus:
            if(pr->State() != ProgressStateSnapshot)
            {
                return;
            }
            if(!msg.reject())
            {
                pr->BecomeProbe();
            }else
            {
                pr->SnapshotFailure();
                pr->BecomeProbe();
            }
            pr->Pause();
           break;
       case raftpb::MsgUnreachable:
           if(pr->Status() == ProgressStateReplicate)
           {
                pr->BecomeProbe();
           }
           break;
       case raftpb::MsgTransferLeader:
            if(pr->IsLearner())
            {
                return;
            }
            uint64_t lead_transferee = msg.from();
            uint64_t last_lead_transferee = lead_transferee_;
            if(last_lead_transferee != 0)
            {
                if(last_lead_transferee == lead_transferee)
                {
                    return;
                }
                AbortLeaderTransfer();
            }
            if(lead_transferee == id_)
            {
                return;
            }
            election_elapsed_ = 0;
            lead_transferee_ = lead_transferee;
            if(pr->Match() == raft_log_->LastIndex())
            {
                SendTimeoutNow(lead_transferee);
            }else
            {
                SendAppend(lead_transferee);
            }
            break;
        default:
            break;

    }
}

Progress* Raft::GetProgress(uint64_t id)
{
    if(peers_.find(id) != peers_.end())
    {
        return peers_[id];
    }
    if(learner_peers_.find(id) != learner_peers_.end())
    {
        return learner_peers_[id];
    }
    return nullptr;
}

void Raft::RemoveNode(uint64_t id)
{
    DelProgress(id);
    pending_conf_ = false;
    if(peers_.empty() && learner_peers_.empty())
    {
        return;
    }
    if(MaybeCommit())
    {
        BroadcastAppend();
    }
    if(state_ == StateLeader && lead_transferee_ == id)
    {
        AbortLeaderTransfer();
    }
}

void Raft::DelProgress(uint64_t id)
{
    auto it_learner = learner_peers_.find(id);
    if(it_learner != learner_peers_.end())
    {
        delete it_learner->second;
        peers_.erase(it_learner);
    }
    auto it = peers_.find(id);
    if(it != peers_.end())
    {
        delete it->second;
        peers_.erase(it);
    }
}
void Raft::SetProgress(uint64_t id, uint64_t match, uint64_t next, bool is_learner)
{
    if(!is_learner)
    {
        DelProgress(id);
        Progress* p = new Progress(next, match, Inflights:NewInflights(max_inflight_));
        peers_[id] = p;
        return;
    }
    auto it = peers_.find(id);
    if(it != peers_.end())
    {
        //todo
        logger_->Trace("can not change role from voter to learner");
        return;
    }
    Progress* p = new Progress(next, match, Inflights:NewInflights(max_inflight_), true);
    learner_peers_[id] = p;
}

void Raft::FromLearnerToVoter(uint64_t id)
{
    Progress* p = learner_peers_[id];
    peers_[id] = p;
    learner_peers_.erase(id);
}

void Raft::AddNodeOrLearnerNode(uint64_t id, bool is_learner)
{
    pending_conf_ = false;
    Progress* pr = GetProgress(id);
    if(!pr)
    {
        SetProgress(id, 0, raft_log_->LastIndex() + 1, is_learner);
    }else
    {
        if(is_learner && !pr->IsLearner())
        {
            return;
        }
        if(is_learner == pr->IsLearner())
        {
            return;
        }
        FromLearnerToVoter(id);
    }
    if(id == id_)
    {
        is_learner_ = is_learner;
    }
    Progress* pr = GetProgress(id);
    pr->SetRecentActive(true);
}


void Raft::AddNode(uint64_t id)
{
    AddNodeOrLearnerNode(id, false);
}

void Raft::AddLearner(uint64_t id)
{
    AddNodeOrLearnerNode(id, true);
}

bool Raft::Promotable(uint64_t id)
{
    return peers_.find(id) != peers_.end();
}

void Raft::RestPendingConf()
{
    pending_conf_ = false;
}

bool Raft::PastElectionTimeOut()
{
    return election_elapsed_ > randomized_elction_timeout_;
}

void Raft::ResetRandomizedElctionTimeout()
{
    randomized_elction_timeout_ = elction_timeout_ + RandomNum(elction_timeout_);
}
}

bool Raft::CheckQuorumActive()
{
    int32_t act = 0;
    for_each(peers_.begin(), peers_.end(), [&](std::peer<uint64_t, Progress*> p)
        {
            if(p.first == id_ || p.second->RecentActive())
            {
                act++;
            }
            p.second->SetRecentActive(false);
         });
    return act >= Quorum();
}

void Raft::SendTimeoutNow(uint64_t to)
{
    raftpb::Message msg;
    msg.set_to(to);
    msg.set_type(raftpb::MsgTimeoutNow);
    Send(msg);
}

void Raft::Send(raftpb::Message& msg)
{
    msg.set_from(id_);
    int32_t msg_type = msg.type();
    if(msg_type == raftpb::MsgVote || msg_type == raftpb::MsgVoteResp || msg_type == raftpb::MsgPreVote || msg_type == raftpb::MsgPreVoteResp)
    {
        if(msg.term() == 0)
        {
            //todo
            return;
        }
    }else
    {
        if(msg.term() != 0)
        {
            //todo
            return;
        }
        if(msg.term() != raftpb::MsgProp || msg.term() != raftpb::MsgReadIndex)
        {
            msg.set_term(term_);
        }
    }
    msgs_.push_back(msg);
}

void Raft::AbortLeaderTransfer()
{
    lead_transferee_ = 0;
}

int32_t Raft::NumOfPendingConf(std::vector<raftpb::Entry>& entries)
{
    return count_if(entries.begin(), entries.end(), [](raftpb::Entry& ent){return ent.type() == raftpb::EntryConfChange;});
}

void Raft::HandleAppendEntries(raftpb::Message& msg)
{
    if(msg.index() < raft_log_->Commited())
    {
        raftpb::Message send;
        send.set_to(msg.from());
        send.set_type(raftpb::MsgAppResp);
        send.set_index(raft_log_->Commited());
        Send(send);
        return;
    }
    std::vector<raftpb::Entry> entries;
    for(int i = 0; i < msg.entries_size(); i++)
    {
        entries.push_back(msg.entry(i));
    }
    uint64_t lastnewi;
    int32_t ret = raft_log_->MaybeAppend(msg.index(), msg.logterm(), msg.commit(), entries, lastnewi);
    if(ret == 0)
    {
        raftpb::Message send;
        send.set_to(msg.from());
        send.set_type(raftpb::MsgAppResp);
        send.set_index(lastnewi);
        Send(send);
    }else
    {
        raftpb::Message send;
        send.set_to(msg.from());
        send.set_type(raftpb::MsgAppResp);
        send.set_index(msg.index());
        send.set_reject(true);
        send.set_rejecthint(raft_log_->LastIndex());
        Send(send);
    }
}

void Raft::HandleHeartBeat(raftpb::Message& msg)
{
    raft_log_->CommitTo(msg.commit());
    raftpb::Message send;
    send.set_to(msg.from());
    send.set_type(raftpb::MsgHeartbeatResp);
    send.set_context(msg.context());
    Send(send);
}

void Raft::HandleSnap(raftpb::Message& msg)
{
    uint64_t snap_index = msg.snapshot().metadata().index();
    uint64_t snap_term = msg.snapshot().metadata().term();
    if(Restore(msg.snapshot()))
    {
        raftpb::Message send;
        send.set_to(msg.from());
        send.set_type(raftpb::MsgAppResp);
        send.set_index(raft_log_->LastIndex());
        Send(send);
    }else
    {
        raftpb::Message send;
        send.set_to(msg.from());
        send.set_type(raftpb::MsgAppResp);
        send.set_index(raft_log_->Commited());
        Send(send);
    }
}

bool Raft::Restore(const raftpb::Snapshot& snap)
{
    if(snap.metadata().index() <= raft_log_->Commited())
    {
        return false;
    }
    if(raft_log_->MatchTerm(snap.metadata().index(), snap.metadata().term()))
    {
        raft_log_->CommitTo(snap.metadata().index());
        return false;
    }
//    if(!is_learner_)
//    {
//        int32_t learner_num = snap.metadata().conf_state().learners_size();
//        for(int i = 0; i < learner_num; i++)
//        {
//            if(_id == snap.metadata().conf_state().learners(i))
//            {
//                return false;
//            }
//        }
//    }
    raft_log_->Restore(snap);
    peers_.clear();
    for(int i = 0; i < snap.metadata().conf_state().nodes_size(); i++)
    {
        uint64_t match = 0;
        uint64_t next = raft_log_->LastIndex() +  1;
        uint64_t node = snap.metadata().conf_state().nodes(i);
        if(node == id_)
        {
            match = next - 1;
        }
        SetProgress(node, match, next);
    }
    return true;
}

bool Raft::HasLeader()
{
    return lead_ != 0;
}

SoftState* Raft::GetSoftState()
{
    return new SoftState(lead_, state_);
}

raftpb::HardState Raft::GetHardState()
{
    raftpb::HardState hs;
    hs.set_term(term_);
    hs.set_vote(vote_);
    hs.set_commit(raft_log_->Commited());
    return hs;
}

std::vector<uint64_t> Raft::Nodes()
{
    std::vector<uint64_t> nodes;
    for(auto& peer: peers_)
    {
        nodes.push_back(peer.first);
    }
    return nodes;
}

void Raft::AllNodesExceptMe(std::vector<uint64_t>& nodes)
{
    for(auto& peer: peers_)
    {
        if(peer.first == id_)
        {
            continue;
        }
        nodes.push_back(peer.first);
    }
    for(auto& peer: learner_peers_)
    {
        if(peer.first == id_)
        {
            continue;
        }
        nodes.push_back(peer.first);
    }
}

void Raft::SendAppend(uint64_t to)
{
    Progress* pr = peers_[to];
    if(pr->IsPaused())
    {
        return;
    }
    raftpb::Message msg;
    msg.set_to(to);
    uint64_t term;
    int32_t term_ret = raft_log_->Term(pr->Next() - 1, term);
    std::vector<raftpb::Entry> entries;
    int32_t ents_ret = raft_log_->Entries(pr->Next(), max_msg_size_, entries);
    if(term_ret != 0 || ents_ret != 0)
    {
        if(!pr->RecentActive())
        {
            return;
        }
        msg.set_type(raftpb::MsgSnap);
        raftpb::Snapshot ss;
        int32_t snap_ret = raft_log_->Snapshot(ss);
        if(snap_ret != 0)
        {
            if(snap_ret == ErrSnapshotTemporarilyUnavailable)
            {
                return;
            }
            //todo
        }
        if(IsSnapshotEmpty(ss))
        {
            //todo
        }
        msg.set_allocated_snapshot(&ss);
        pr->BecomeSnapshot(ss.metadata().index());
    }else
    {
        msg.set_type(raftpb::MsgApp);
        msg.set_index(pr->Next() - 1);
        msg.set_logterm(term);
        for(auto& ent: entries)
        {
           raftpb::Entry* entry = msg.add_entries();
           *entry = ent;
        }
        msg.set_commit(raft_log_->Commited());
        if(!entries.empty())
        {
            switch(pr->State())
            {
                case ProgressStateReplicate:
                    uint64_t last_index = entries[entries.size() - 1].index();
                    pr->OptimisticUpdate(last_index);
                    pr->GetInflights()->Add(last_index);
                    break;
                case ProgressStateProbe:
                    pr->Pause();
                    break;
                default:
                    //todo
            }
        }
    }
    Send(msg);
}

void Raft::SendHeartbeat(uint64_t to, const std::string& ctx)
{
    Progress* pr = GetProgress(to);
    uint64_t commit = std::min(pr->Match(), raft_log_->Commited());
    raftpb::Message msg;
    msg.set_to(to);
    msg.set_type(raftpb::MsgHeartbeat);
    msg.set_commit(commit);
    msg.set_context(ctx);
    Send(msg);
}

void Raft::BroadcastAppend()
{
    std::vector<uint64_t> nodes;
    AllNodesExceptMe(nodes);
    for(auto id: nodes)
    {
        SendAppend(id);
    }
}

void Raft::BroadcastHeartbeatWithCtx(const std::string& ctx)
{
    std::vector<uint64_t> nodes;
    AllNodesExceptMe(nodes);
    for(auto id: nodes)
    {
        SendHeartbeat(id, ctx);
    }
}

void Raft::BroadcastHeartbeat()
{
    std::string msg = read_only_->LastPendingRequest();
    BroadcastHeartbeatWithCtx(msg);
}

bool Raft::MaybeCommit()
{
    std::vector<uint64_t> matchs;
    for(auto& p: peers_)
    {
        matchs.push_back(p.second->Match());
    }
    std::sort(matchs.begin(), matchs.end());
    std::reverse(matchs.begin(), matchs.end());
    uint64_t match = matchs[Quorum() - 1];
    return raft_log_->MaybeCommit(match, term_);
}

void Raft::Reset(uint64_t term)
{
    if(term != term_)
    {
        term_ = term;
        votes_ = 0;
    }
    lead_ = 0;
    election_elapsed_ = 0;
    heart_beat_elapsed_ = 0;
    ResetRandomizedElctionTimeout();
    AbortLeaderTransfer();
    votes_.clear();
    for(auto& p: peers_)
    {
        delete p.second;
        p.second = new Progress(raft_log_->LastIndex() + 1, 0, Inflights::NewInflight(max_inflight_msgs_, false));
        if(p.first == id_)
        {
            p.second->SetMatch(raft_log_->LastIndex());
        }
    }
    for(auto& p: learner_peers_)
    {
        delete p.second;
        p.second = new Progress(raft_log_->LastIndex() + 1, 0, Inflights::NewInflight(max_inflight_msgs_, true));
    }
    pending_conf_ = false;
    ReadOnly* tmp = read_only_;
    read_only_ = ReadOnly::NewReadOnly(tmp->Option());
    delete tmp;
}

void Raft::AppendEntries(std::vector<raftpb::Entry>& entries)
{
    uint64_t last_index = raft_log_->LastIndex();
    for(int i = 0; i < entries.size(); i++)
    {
        entries[i].set_term(term_);
        entries[i].set_index(last_index + 1 + i);
    }
    raft_log_->Append(entries, last_index);
    Progress* pr = GetProgress(id_);
    pr->MayUpdate(raft_log_->LastIndex());
    MaybeCommit();
}
















}