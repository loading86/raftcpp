#include "progress.h"
namespace raft {
Inflights::Inflights(int size)
{
    size_ = size;
    count_ = 0;
}
Inflights* Inflights::NewInflights(int32_t size)
{
    return new Inflights(size);
}
void Inflights::Add(uint64_t inflight)
{
    if (Full()) {
        //todo
        return;
    }
    buffer_.push_back(inflight);
    count_++;
}
void Inflights::GrowBuf()
{
}
void Inflights::FreeTo(uint64_t to)
{
    while ((!buffer_.empty()) && buffer_.front() <= to) {
        buffer_.pop_front();
        count_--;
    }
}
void Inflights::FreeFirstOne()
{
    if ((!buffer_.empty())) {
        buffer_.pop_front();
        count_--;
    }
}
bool Inflights::Full()
{
    return size_ == count_;
}
void Inflights::Reset()
{
    buffer_.clear();
    count_ = 0;
}

Progress::Progress(uint64_t next, Inflights* inflight):next_(next),inflights_(inflight)
{
}

Progress::Progress(uint64_t next, uint64_t match, Inflights* inflight):next_(next),match_(match),inflights_(inflight)
{
}

Progress::Progress(uint64_t next, uint64_t match, Inflights* inflight, bool is_learner):next_(next),match_(match),inflights_(inflight), is_learner_(is_learner)
{
}

void Progress::ResetState(ProgressStateType state)
{
    paused_ = false;
    pending_snaphot_ = 0;
    state_ = state;
    inflights_->Reset();
}
void Progress::BecomeProbe()
{
    if (state_ == ProgressStateSnapshot) {
        uint64_t pending_snaphot = pending_snaphot_;
        ResetState(ProgressStateProbe);
        next_ = std::max(match_ + 1, pending_snaphot + 1);
    } else {
        ResetState(ProgressStateProbe);
        next_ = match_ + 1;
    }
}
void Progress::BecomeReplicate()
{
    ResetState(ProgressStateReplicate);
    next_ = match_ + 1;
}
void Progress::BecomeSnapshot(uint64_t snapshot_index)
{
    ResetState(ProgressStateSnapshot);
    pending_snaphot_ = snapshot_index;
}
bool Progress::MaybeUpdate(uint64_t index)
{
    bool update = false;
    if (match_ < index) {
        match_ = index;
        update = true;
        Resume();
    }
    if (next_ < index + 1) {
        next_ = index + 1;
    }
    return update;
}
void Progress::OptimisticUpdate(uint64_t index)
{
    next_ = index + 1;
}
bool Progress::MaybeDecrTo(uint64_t rejected, uint64_t last)
{
    if (state_ == ProgressStateReplicate) {
        if (rejected <= match_) {
            return false;
        }
        next_ = match_ + 1;
        return true;
    }
    if (next_ != rejected) {
        return false;
    }
    next_ = std::min(rejected, last + 1);
    if (next_ < 1) {
        next_ = 1;
    }
    Resume();
    return true;
}
void Progress::Pause()
{
    paused_ = true;
}
void Progress::Resume()
{
    paused_ = false;
}
bool Progress::IsPaused()
{
    switch (state_) {
    case ProgressStateProbe:
        return paused_;
    case ProgressStateReplicate:
        return inflights_->Full();
    case ProgressStateSnapshot:
        return true;
    default:
        //todo
        return false;
    }
}
void Progress::SnapshotFailure()
{
    pending_snaphot_ = 0;
}
bool Progress::NeedSnapshotAbort()
{
    return state_ == ProgressStateSnapshot && match_ >= pending_snaphot_;
}
}