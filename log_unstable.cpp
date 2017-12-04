#include "log_unstable.h"

Unstable::Unstable()
{
    m_snapshot = nullptr;
    m_offset = 0;
}

int32_t Unstable::maybeFirstIndex(uint64_t& index)
{
    if(m_snapshot != nullptr)
    {
        index = m_snapshot->metadata().index() + 1;
        return 0;
    }
    return 1;
}

int32_t Unstable::maybeLastIndex(uint64_t& index)
{
    if(!m_entries.empty())
    {
        index = m_offset + m_entries.size() - 1;
        return 0;
    }
    if(m_snapshot != nullptr)
    {
        index = m_snapshot->metadata().index();
        return 0;
    }
    return 1;
}

int32_t Unstable::maybeTerm(uint64_t index, uint64_t& term)
{
    if(index < m_offset)
    {
        if(m_snapshot != nullptr)
        {
            if(m_snapshot->metadata().index() == index)
            {
                term = m_snapshot->metadata().term();
                return 0;
            }
        }
        return 1;
    }
    uint64_t last_index;
    int32_t ret = maybeLastIndex(last_index);
    if(ret != 0 || index > last_index)
    {
        return 1;
    }
    term = m_entries[index - m_offset].term();
    return 0;
}

void Unstable::stableTo(uint64_t index, uint64_t term)
{
    uint64_t real_term;
    int32_t ret = maybeTerm(index, real_term);
    if(ret == 0 && real_term == term && index >= m_offset)
    {
        m_entries.erase(m_entries.begin(), m_entries.begin() + index + 1 - m_offset);
        m_offset = index + 1;
    }
}

void Unstable::stableSnapTo(uint64_t index)
{
    if(m_snapshot != nullptr && m_snapshot->metadata().index() == index)
    {
        m_snapshot = nullptr;
    }
}

void Unstable::restore(raftpb.Snapshot* ss)
{
    m_offset = ss->metadata().index() + 1;
    m_entries.clear();
    m_snapshot = ss;
}

void Unstable::truncateAndAppend(const std::vector<raftpb.Entry>& entries)
{
    uint64_t after = entries[0].index();
    if(after == m_offset + m_entries.size())
    {
        m_entries.reserve(m_entries.size() + entries.size());
        std::copy(entries.begin(), entries.end(), m_entries.end());
    }else if(after <= m_offset)
    {
        m_offset = after;
        m_entries = entries;
    }else
    {
        uint64_t len = entries[entries.size() - 1].index() - m_offset + 1;
        entries.reserve(len);
        uint64_t origin_left_len = len - entries.size();
        std::copy(entries.begin(), entries.end(), m_entries.begin() + origin_left_len);
    }
}



int32_t Unstable::mustCheckOutOfBounds(uint64_t lo, uint64_t hi)
{
    if(hi < lo)
    {
        return 1;
    }
    uint64_t upper = m_offset + m_entries.size();
    if(lo < m_offset || hi > upper)
    {
        return 1;
    }
    return 0;
}

int32_t Unstable::slice(uint64_t lo, uint64_t hi, std::vector<raftpb.Entry>& entries);
{
	int32_t ret = mustCheckOutOfBounds(lo, hi);
	if(ret != 0)
	{
		return ret;
	}	
	uint64_t left = lo - m_offset;
	uint64_t right = hi - m_offset;
	entries.reserve(right - left + 1);
	std::copy(m_entries.begin(), m_entries.end(), entries.begin());
	return 0;
}



