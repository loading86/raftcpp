#include "storage.h"
#include <algorithm>
MemoryStorage::MemoryStorage()
{
    raftpb.Entry entry;
    m_entries.push_back(entry);
}

int32_t MemoryStorage::InitialState(raftpb.HardState& hs, raftpb.ConfState& cs)
{
    hs = m_hardstat;
    cs = m_snapshot.metadata().conf_state();
    return 0;
}

uint64_t MemoryStorage::firstIndex()
{
    return m_entries[0].index() + 1;
}

uint64_t MemoryStorage::lastIndex()
{
    int len = m_entries.size();
    return m_entries[len - 1].index();
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

int32_t MemoryStorage::SnapShot(raftpb.Snapshot& ss)
{
    ss = m_m_snapshot;
    return 0;
}

int32_t MemoryStorage::Entries(uint64_t lo, uint64_t hi, uint64_t maxSize,std::vestor<raftpb.Entry>& entries)
{
    if(hi <= lo)
    {
        return ErrParam;
    }
    if(lo < firstIndex())
    {
        return ErrCompacted;
    }
    if(m_entries.size() == 1 || (hi - lo) > (lastIndex - firstIndex + 1))
    {
        return ErrUnavailable;
    }

    entries.assign(m_entries.begin() + lo - (firstIndex() - 1), m_entries.begin() + hi - (firstIndex() - 1));
    return 0;
}

int32_t MemoryStorage::Term(uint64_t index, uint64_t& term)
{
    if(index < firstIndex() - 1)
    {
        return ErrCompacted;
    }
    if(index > lastIndex())
    {
        return ErrUnavailable;
    }
    term = m_entries[index - firstIndex() + 1].term();
    return 0;
}

int32_t MemoryStorage::SetHardState(raftpb.HardState hs)
{
    m_hardstat = hs;
    return 0;
}

int32_t MemoryStorage::ApplySnapshot(raftpb.Snapshot ss)
{
    uint64_t my_snap_index = m_snapshot.metadata().index();
    uint64_t in_snap_index = ss.metadata().index();
    if(in_snap_index <= my_snap_index)
    {
        return ErrSnapOutOfDate;
    }
    m_snapshot = ss;
    raftpb.Entry entry;
    entry.set_index(in_snap_index);
    uint64_t in_term = ss.metadata().term();
    entry.set_term(in_term);
    m_entries.clear();
    m_entries.push_back(entry);
    return 0;
}

int32_t MemoryStorage::CreateSnapshot(uint64_t index, raftpb.ConfState cs, std::string& data, raftpb.Snapshot& ss)
{
    uint64_t my_snap_index = m_snapshot.metadata().index();
    uint64_t in_snap_index = ss.metadata().index();
    if(in_snap_index <= my_snap_index)
    {
        return ErrSnapOutOfDate;
    }
    if(index > lastIndex())
    {
        return ErrUnavailable;
    }
    uint64_t offset = firstIndex() - 1;
    m_snapshot.metadata().set_index(index);
    uint64_t term = 0;
    Term(index, term);
    m_snapshot.metadata().set_term(term);
    raftpb.ConfState* cf =  new raftpb.ConfState(cs);
    m_snapshot.metadata().set_allocated_conf_state(cf);
    m_snapshot.set_data(data);
    return 0;
}

int32_t MemoryStorage::Compact(uint64_t index)
{
    if(index <= firstIndex() - 1)
    {
        return ErrCompacted;
    }
    if(index > lastIndex())
    {
        return ErrUnavailable;
    }
    uint64_t term = 0;
    Term(index, term);
    uint64_t len = lastIndex() - index;//need copy entries [index+1...]
    std::vector<raftpb.Entry> entries;
    entries.reserve(len + 1);
    raftpb.Entry entry;
    entry.set_index(index);
    entry.set_term(term);
    entries.push_back(entry);
    std::copy(m_entries.begin() + index - firstIndex(), m_entries.end(), entries.begin() + 1);
    entries.swap(m_entries);
    return 0;
}

int32_t MemoryStorage::Append(std::vestor<raftpb.Entry>& entries)
{
    if(entries.empty())
    {
        return 0;
    }
    uint64_t last = entries[entries.size() - 1].index();
    if(last < firstIndex())
    {
        return 0;
    }
    if(entries[entries.size() - 1].index() > lastIndex())
    {
        return ErrUnavailable;
    }
    uint64_t offset = 0;
    if(firstIndex() > entries[0].index())
    {
        offset = firstIndex() - entries[0].index();
    }
    uint64_t offset_index = entries[offset].index();
    uint64_t len = entries.size() - offset + 1;
    if(offset_index = lastIndex() + 1)
    {
        m_entries.reserve(m_entries.size() + len);
        std::copy(entries.begin() + offset, entries.end(), m_entries.end());
    }else if(offset_index <= lastIndex())
    {
        uint64_t final_len = entries[entries.size() - 1].index() - m_entries[0].index() + 1
        m_entries.reserve(final_len);
        uint64_t origin_left_len = final_len - len;
        std::copy(entries.begin() + offset, entries.end(), m_entries.begin() + origin_left_len);
    }
    return 0;
}