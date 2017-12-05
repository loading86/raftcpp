#include "storage.h"
#include <algorithm>
namespace raft {
MemoryStorage::MemoryStorage()
{
    raftpb::Entry entry;
    entries_.push_back(entry);
}

int32_t MemoryStorage::InitialState(raftpb::HardState& hs,
    raftpb::ConfState& cs)
{
    hs = hardstat_;
    cs = snapshot_.metadata().conf_state();
    return 0;
}

uint64_t MemoryStorage::firstIndex()
{
    return entries_[0].index() + 1;
}

uint64_t MemoryStorage::lastIndex()
{
    int len = entries_.size();
    return entries_[len - 1].index();
}

int32_t MemoryStorage::FirstIndex(uint64_t& index)
{
    index = firstIndex();
    return 0;
}

int32_t MemoryStorage::LastIndex(uint64_t& index)
{
    index = lastIndex();
    return 0;
}

int32_t MemoryStorage::SnapShot(raftpb::Snapshot& ss)
{
    ss = snapshot_;
    return 0;
}

int32_t MemoryStorage::Entries(uint64_t lo,
    uint64_t hi,
    uint64_t maxSize,
    std::vector<raftpb::Entry>& entries)
{
    if (hi <= lo) {
        return ErrParam;
    }
    if (lo < firstIndex()) {
        return ErrCompacted;
    }
    if (entries_.size() == 1 || (hi - lo) > (lastIndex() - firstIndex() + 1)) {
        return ErrUnavailable;
    }

    entries.assign(entries_.begin() + lo - (firstIndex() - 1),
        entries_.begin() + hi - (firstIndex() - 1));
    return 0;
}

int32_t MemoryStorage::Term(uint64_t index, uint64_t& term)
{
    if (index < firstIndex() - 1) {
        return ErrCompacted;
    }
    if (index > lastIndex()) {
        return ErrUnavailable;
    }
    term = entries_[index - firstIndex() + 1].term();
    return 0;
}

int32_t MemoryStorage::SetHardState(raftpb::HardState& hs)
{
    hardstat_ = hs;
    return 0;
}

int32_t MemoryStorage::ApplySnapshot(raftpb::Snapshot ss)
{
    uint64_t my_snap_index = snapshot_.metadata().index();
    uint64_t in_snap_index = ss.metadata().index();
    if (in_snap_index <= my_snap_index) {
        return ErrSnapOutOfDate;
    }
    snapshot_ = ss;
    raftpb::Entry entry;
    entry.set_index(in_snap_index);
    uint64_t in_term = ss.metadata().term();
    entry.set_term(in_term);
    entries_.clear();
    entries_.push_back(entry);
    return 0;
}

int32_t MemoryStorage::CreateSnapshot(uint64_t index,
    raftpb::ConfState cs,
    std::string& data,
    raftpb::Snapshot& ss)
{
    uint64_t my_snap_index = snapshot_.metadata().index();
    uint64_t in_snap_index = ss.metadata().index();
    if (in_snap_index <= my_snap_index) {
        return ErrSnapOutOfDate;
    }
    if (index > lastIndex()) {
        return ErrUnavailable;
    }
    uint64_t offset = firstIndex() - 1;
    snapshot_.mutable_metadata()->set_index(index);
    uint64_t term = 0;
    Term(index, term);
    snapshot_.mutable_metadata()->set_term(term);
    raftpb::ConfState* cf = new raftpb::ConfState(cs);
    snapshot_.mutable_metadata()->set_allocated_conf_state(cf);
    snapshot_.set_data(data);
    return 0;
}

int32_t MemoryStorage::Compact(uint64_t index)
{
    if (index <= firstIndex() - 1) {
        return ErrCompacted;
    }
    if (index > lastIndex()) {
        return ErrUnavailable;
    }
    uint64_t term = 0;
    Term(index, term);
    uint64_t len = lastIndex() - index; // need copy entries [index+1...]
    std::vector<raftpb::Entry> entries;
    entries.reserve(len + 1);
    raftpb::Entry entry;
    entry.set_index(index);
    entry.set_term(term);
    entries.push_back(entry);
    std::copy(entries_.begin() + index - firstIndex(), entries_.end(),
        entries.begin() + 1);
    entries.swap(entries_);
    return 0;
}

int32_t MemoryStorage::Append(std::vector<raftpb::Entry>& entries)
{
    if (entries.empty()) {
        return 0;
    }
    uint64_t last = entries[entries.size() - 1].index();
    if (last < firstIndex()) {
        return 0;
    }
    if (entries[entries.size() - 1].index() > lastIndex()) {
        return ErrUnavailable;
    }
    uint64_t offset = 0;
    if (firstIndex() > entries[0].index()) {
        offset = firstIndex() - entries[0].index();
    }
    uint64_t offset_index = entries[offset].index();
    uint64_t len = entries.size() - offset + 1;
    if (offset_index = lastIndex() + 1) {
        entries_.reserve(entries_.size() + len);
        std::copy(entries.begin() + offset, entries.end(), entries_.end());
    } else if (offset_index <= lastIndex()) {
        uint64_t final_len = entries[entries.size() - 1].index() - entries_[0].index() + 1;
        entries_.reserve(final_len);
        uint64_t origin_left_len = final_len - len;
        std::copy(entries.begin() + offset, entries.end(),
            entries_.begin() + origin_left_len);
    }
    return 0;
}
}