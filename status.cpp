#include "status.h"
namespace raft
{

Status::Status()
{
}

Status GetStatus(Raft* rf)
{
    Status ss;
    ss.id_ = rf->GetID();
    ss.lead_transferee_ = rf->GetLeadTransferee();
    ss.hs_ = rf->GetHardState();
    ss.ss_ = *(rf->GetSoftState());
    ss.applied_ = rf->GetRaftLog()->Applied();
    if(ss.ss_.raft_state_ == StateLeader)
    {
        rf->AllProgress(ss.progress_);
    }
}
}