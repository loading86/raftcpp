#include "log.h"
#include "utilies.h"
#include <algorithm>
namespace raft {
RaftLog::RaftLog(Storage* storage, Logger* logger)
{
    storage_ = storage;
    logger_ = logger;
    unstable_ = new Unstable();
    commited_ = 0;
    applied_ = 0;
}

RaftLog* RaftLog::NewLog(Storage* storage, Logger* logger)
{
    if (storage == nullptr) {
        return nullptr;
    }
    RaftLog* log = new RaftLog(storage, logger);
    uint64_t first_index;
    int32_t ret = storage->FirstIndex(first_index);
    if (ret != 0) {
        return nullptr;
    }
    uint64_t last_index;
    ret = storage->LastIndex(last_index);
    if (ret != 0) {
        return nullptr;
    }
    log->unstable_->SetOffset(last_index + 1);
    log->unstable_->SetLogger(logger);
    log->commited_ = first_index - 1;
    log->applied_ = first_index - 1;
    return log;
}

int32_t RaftLog::FirstIndex(uint64_t& index)
{
    int32_t ret = unstable_->MaybeFirstIndex(index);
    if (ret == 0) {
        return 0;
    }
    ret = storage_->FirstIndex(index);
    if (ret == 0) {
        return 0;
    }
    return 1;
}

int32_t RaftLog::LastIndex(uint64_t& index)
{
    int32_t ret = unstable_->MaybeLastIndex(index);
    if (ret == 0) {
        return 0;
    }
    ret = storage_->LastIndex(index);
    if (ret == 0) {
        return 0;
    }
    return 1;
}

void RaftLog::UnstableEntries(std::vector<raftpb::Entry>& entries)
{
    unstable_->Entries(entries);
}

int32_t RaftLog::MaybeAppend(uint64_t index, uint64_t term, uint64_t commited,
    const std::vector<raftpb::Entry>& entries,
    uint64_t& lastnewi)
{ // todo
    if (MatchTerm(index, term)) {
        lastnewi = index + entries.size();
        uint64_t ci = FindConflict(entries);
        if(ci <= commited_){
            logger_->Trace("conflict with committed entry");
        }else if(ci != 0){
            uint64_t offset = index + 1;
            std::vector<raftpb::Entry> append_entries(entries.begin() + ci - offset, entries.end());
            uint64_t dummy_index;
            Append(append_entries, dummy_index);
            CommitTo(std::min(commited_, lastnewi));
            return 0;
        }
    }
    return 1;
}

int32_t RaftLog::Append(const std::vector<raftpb::Entry>& entries,
    uint64_t& index)
{
    if (entries.empty()) {
        return LastIndex(index);
    }
    uint64_t after = entries[0].index() - 1;
    if (after < commited_) {
        return 1;
    }
    unstable_->TruncateAndAppend(entries);
    return LastIndex(index);
}

uint64_t RaftLog::FindConflict(const std::vector<raftpb::Entry>& entries)
{
    for (auto& entry : entries) {
        if (!MatchTerm(entry.index(), entry.term())){
            uint64_t last_index;
            LastIndex(last_index);
            if (entry.index() <= last_index){
                logger_->Trace("found conflict");
                //todo
            }
            return entry.index();
        }
    }
    return 0;
}

int32_t RaftLog::Term(uint64_t index, uint64_t& term)
{
    uint64_t first_index, last_index, dummy_index;
    if (FirstIndex(first_index) != 0) {
        return 1;
    }
    dummy_index = first_index - 1;
    if (LastIndex(last_index) != 0) {
        return 1;
    }
    if (index < dummy_index || index > last_index) {
        term = 0;
        return 0;
    }
    int32_t ret = storage_->Term(index, term);
    if (ret == 0) {
        return ret;
    }
    if (ret == ErrCompacted || ret == ErrUnavailable) {
        term = 0;
        return 0;
    }
    logger_->Trace("RaftLog Term failed");
    return ret;
}

int32_t RaftLog::Entries(uint64_t index, uint64_t maxsize,
    std::vector<raftpb::Entry>& entries)
{
    uint64_t last_index;
    int32_t ret = LastIndex(last_index);
    if (index > last_index) {
        return 0;
    }
    return Slice(index, last_index + 1, maxsize, entries);
}

int32_t RaftLog::AllEntries(std::vector<raftpb::Entry>& entries)
{
    uint64_t first_index;
    int32_t ret = FirstIndex(first_index);
    if (ret != 0) {
        return ret;
    }
    ret = Entries(first_index, UINT64_MAX, entries);
    if (ret == 0) {
        return 0;
    }
    return 1;
}

int32_t RaftLog::MustCheckOutOfBounds(uint64_t lo, uint64_t hi)
{
    if (lo > hi) {
        logger_->Trace("invalid Slice");
        return 1;
    }
    uint64_t first_index;
    int32_t ret = FirstIndex(first_index);
    if (ret != 0) {
        return 1;
    }
    uint64_t last_index;
    ret = LastIndex(last_index);
    if (ret != 0) {
        return 1;
    }
    if (lo < first_index) {
        return ErrCompacted;
    }
    uint64_t len = last_index - first_index + 1;
    if (lo < first_index || hi > first_index + len) {
        logger_->Trace("Slice out of bound");
        return 1;
    }
    return 0;
}

int32_t RaftLog::Slice(uint64_t lo, uint64_t hi, uint64_t max_size,
    std::vector<raftpb::Entry>& entries)
{
    int32_t ret = MustCheckOutOfBounds(lo, hi);
    if (ret != 0) {
        return ret;
    }
    if (lo == hi) {
        return 0;
    }
    if (lo < unstable_->GetOffset()) {
        ret = storage_->Entries(lo, std::min(hi, unstable_->GetOffset()), max_size,
            entries);
        if (ret == ErrCompacted) {
            return 0;
        } else if (ret == ErrUnavailable) {
            logger_->Trace("entries are unavailable from storage");
            return ret;
        } else {
            logger_->Trace("entries are unavailable from storage");
            return ret;
        }
    }
    if (hi > unstable_->GetOffset()) {
        std::vector<raftpb::Entry> unstable_ents;
        unstable_->Slice(std::max(lo, unstable_->GetOffset()), hi, unstable_ents);
        if (!unstable_ents.empty()) {
            entries.reserve(entries.size() + unstable_ents.size());
            std::copy(unstable_ents.begin(), unstable_ents.end(), entries.end());
        }
    }
    LimitSize(entries, max_size);
    return 0;
}

int32_t RaftLog::NextEnts(std::vector<raftpb::Entry>& entries)
{
    uint64_t first_index;
    int32_t ret = FirstIndex(first_index);
    if (ret != 0) {
        return ret;
    }
    uint64_t off = std::max(applied_ + 1, first_index);
    if (commited_ + 1 > off) {
        ret = Slice(off, commited_ + 1, UINT64_MAX, entries);
        if (ret != 0) {
            logger_->Trace("unexpected error when getting unapplied entries");
            return ret;
        }
    }
    return 0;
}

bool RaftLog::HasNextEnts()
{
    uint64_t first_index;
    int32_t ret = FirstIndex(first_index);
    if (ret != 0) {
        return false;
    }
    uint64_t off = std::max(applied_ + 1, first_index);
    return commited_ + 1 > off;
}

int32_t RaftLog::Snapshot(raftpb::Snapshot& ss)
{
    if (unstable_->GetSnapshot() != nullptr) {
        ss = *(unstable_->GetSnapshot());
        return 0;
    }
    return storage_->SnapShot(ss);
}

int32_t RaftLog::CommitTo(uint64_t commited)
{
    if (commited_ < commited) {
        uint64_t last_index;
        int32_t ret = LastIndex(last_index);
        if (ret != 0) {
            return ret;
        }
        if (last_index < commited) {
            logger_->Trace("commited is out of range. Was the raft log corrupted, "
                           "truncated, or lost?");
            return 1;
        }
        commited_ = commited;
        return 0;
    }
    return 0;
}

int32_t RaftLog::AppliedTo(uint64_t applied)
{
    if (applied == 0) {
        return 0;
    }
    if (commited_ < applied || applied < applied_) {
        logger_->Trace("applied is out of range");
        return 1;
    }
    applied_ = applied;
    return 0;
}

void RaftLog::StableTo(uint64_t index, uint64_t term)
{
    unstable_->StableTo(index, term);
}

void RaftLog::StableSnapTo(uint64_t index) { unstable_->StableSnapTo(index); }

int32_t RaftLog::LastTerm(uint64_t& term)
{
    uint64_t last_index;
    int32_t ret = LastIndex(last_index);
    if (ret != 0) {
        return 1;
    }
    return Term(last_index, term);
}

bool RaftLog::IsUpToDate(uint64_t index, uint64_t term)
{
    uint64_t last_index;
    int32_t ret = LastIndex(last_index);
    if (ret != 0) {
        return false;
    }
    uint64_t last_term;
    ret = LastTerm(last_term);
    if (ret != 0) {
        return false;
    }
    return (term > last_term || (term == last_term && index >= last_index));
}

bool RaftLog::MatchTerm(uint64_t index, uint64_t term)
{
    uint64_t real_term;
    int32_t ret = Term(index, real_term);
    if (ret != 0) {
        return false;
    }
    return real_term == term;
}

bool RaftLog::MaybeCommit(uint64_t index, uint64_t term)
{
    if (index > commited_) {
        if (!MatchTerm(index, term)) {
            return false;
        }
        if (0 != CommitTo(index)) {
            return false;
        }
        return true;
    }
    return false;
}

void RaftLog::Restore(raftpb::Snapshot& ss)
{
    commited_ = ss.mutable_metadata()->index();
    unstable_->Restore(&ss);
}
}
