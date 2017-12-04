#include "log.h"
#include <algorithm>
#include "utilies.h"
namespace raft
{
RaftLog::RaftLog(Storage* storage, Logger* logger)
{
	m_storage = storage;
	m_logger = logger;
	m_unstable = new Unstable();
	m_commited = 0;
	m_applied = 0;
}

RaftLog* RaftLog::NewLog(Storage* storage, Logger* logger)
{
	if(storage == nullptr)
	{
		return nullptr;
	}
	RaftLog* log = new RaftLog(storage, logger);
	uint64_t first_index;
	int32_t ret = storage->FirstIndex(first_index);
	if(ret != 0)
	{
		return nullptr;
	}
        uint64_t last_index;
        ret = storage->LastIndex(last_index);
        if(ret != 0)
        {
                return nullptr;
        }
	log->m_unstable->setOffset(last_index + 1);
	log->m_unstable->setLogger(logger);
	log->m_commited = first_index - 1;
	log->m_applied = first_index - 1;
	return log;
}

int32_t RaftLog::firstIndex(uint64_t& index)
{
	int32_t ret = m_unstable->maybeFirstIndex(index);
	if(ret == 0)
	{
		return 0;
	}
	ret = m_storage->FirstIndex(index);
	if(ret == 0)
        {
                return 0;
        }
	return 1;
}

int32_t RaftLog::lastIndex(uint64_t& index)
{
        int32_t ret = m_unstable->maybeLastIndex(index);
        if(ret == 0)
        {
                return 0;
        }
        ret = m_storage->LastIndex(index);
        if(ret == 0)
        {
                return 0;
        }
        return 1;
}

void RaftLog::unstableEntries(std::vector<raftpb::Entry>& entries)
{
	m_unstable->Entries(entries);
}

int32_t RaftLog::maybeAppend(uint64_t index, uint64_t term, uint64_t commited, const std::vector<raftpb::Entry>& entries, uint64_t& lastnewi)
{//todo
	if(matchTerm(index, term))
	{
		lastnewi = index + entries.size();
		uint64_t ci = findConflict(entries);

	}
	return 0;
}

int32_t RaftLog::append(const std::vector<raftpb::Entry>& entries, uint64_t& index)
{
	if(entries.empty())
	{
		return lastIndex(index);
	}
	uint64_t after = entries[0].index() - 1;
	if(after < m_commited)
	{
		return 1;
	}	
	m_unstable->truncateAndAppend(entries);
	return lastIndex(index);
}	

uint64_t RaftLog::findConflict(const std::vector<raftpb::Entry>& entries)
{
	for(auto& entry: entries)
	{
		
	}
}

int32_t RaftLog::Term(uint64_t index, uint64_t& term)
{
	uint64_t first_index, last_index, dummy_index;
	if(firstIndex(first_index) != 0)
	{
		return 1;
	}
	dummy_index = first_index - 1;
	if(lastIndex(last_index) != 0)
        {
                return 1;
        }
	if(index < dummy_index || index > last_index)
	{
		term = 0;
		return 0;
	}
	int32_t ret = m_storage->Term(index, term);
	if(ret == 0)
	{
		return ret;
	}
	if(ret == ErrCompacted || ret == ErrUnavailable)
	{
		term = 0;
                return 0;
	}
	m_logger->Trace("RaftLog Term failed");
	return ret;
}

int32_t RaftLog::Entries(uint64_t index, uint64_t maxsize, std::vector<raftpb::Entry>& entries)
{
	uint64_t last_index;
	int32_t ret = lastIndex(last_index);
	if(index > last_index)
	{
		return 0;
	}
	return slice(index, last_index + 1, maxsize, entries);
}

int32_t RaftLog::allEntries(std::vector<raftpb::Entry>& entries)
{
	uint64_t first_index;
	int32_t ret = firstIndex(first_index);
	if(ret != 0)
	{
	    return ret;
	}
	ret = Entries(first_index, UINT64_MAX, entries);
	if(ret == 0)
	{
		return 0;
	}
	return 1;
}

int32_t RaftLog::mustCheckOutOfBounds(uint64_t lo, uint64_t hi)
{
	if(lo > hi)
	{
		m_logger->Trace("invalid slice");
		return 1;
	}
	uint64_t first_index;
	int32_t ret = firstIndex(first_index);
	if(ret != 0)
	{
		return 1;
	}
	uint64_t last_index;
	ret = lastIndex(last_index);
        if(ret != 0)
        {
                return 1;
        }
	if(lo < first_index)
	{
		return ErrCompacted;
	}
	uint64_t len = last_index - first_index + 1;
	if(lo < first_index || hi > first_index + len)
	{
		 m_logger->Trace("slice out of bound");
		 return 1;
	}
	return 0;

}


int32_t RaftLog::slice(uint64_t lo, uint64_t hi, uint64_t maxSize, std::vector<raftpb::Entry>& entries)
{
	int32_t ret = mustCheckOutOfBounds(lo, hi);
	if(ret != 0)
	{
		return ret;
	}
	if(lo == hi)
	{
		return 0;
	}
	if(lo < m_unstable->getOffset())
	{
		ret = m_storage->Entries(lo, std::min(hi, m_unstable->getOffset()), maxSize, entries);
		if(ret == ErrCompacted)
		{
			return 0;
		}else if(ret == ErrUnavailable)
		{
			m_logger->Trace("entries are unavailable from storage");
			return ret;
		}else
		{
			m_logger->Trace("entries are unavailable from storage");
                        return ret;
		}
	}
	if(hi > m_unstable->getOffset())
	{
		std::vector<raftpb::Entry> unstable_ents;
		m_unstable->slice(std::max(lo, m_unstable->getOffset()), hi, unstable_ents);
		if(!unstable_ents.empty())
		{
			entries.reserve(entries.size() + unstable_ents.size());
			std::copy(unstable_ents.begin(), unstable_ents.end(), entries.end());
		}
	}
	limitSize(entries, maxSize);
	return 0;
}

int32_t RaftLog::nextEnts(std::vector<raftpb::Entry>& entries)
{
	uint64_t first_index;
	int32_t ret = firstIndex(first_index);
	if(ret != 0)
	{
		return ret;
	}	
	uint64_t off = std::max(m_applied + 1, first_index);
	if(m_commited + 1 > off)
	{
		ret = slice(off, m_commited + 1, UINT64_MAX, entries);
		if(ret != 0)
		{
			m_logger->Trace("unexpected error when getting unapplied entries");
			return ret;
		}
	}
	return 0;
}


bool RaftLog::hasNextEnts()
{
	uint64_t first_index;
        int32_t ret = firstIndex(first_index);
        if(ret != 0)
        {
                return false;
        }
	uint64_t off = std::max(m_applied + 1, first_index);
	return m_commited + 1 > off;
}

int32_t RaftLog::snapshot(raftpb::Snapshot& ss)
{
	if(m_unstable->getSnapshot() != nullptr)
	{
		ss = *(m_unstable->getSnapshot());
		return 0;
	}
	return m_storage->SnapShot(ss);
}

int32_t RaftLog::commitTo(uint64_t commited)
{
	if(m_commited < commited)
	{
		uint64_t last_index;
		int32_t ret = lastIndex(last_index);
		if(ret != 0)
		{
			return ret;
		}
		if(last_index < commited)
		{
			m_logger->Trace("commited is out of range. Was the raft log corrupted, truncated, or lost?");
			return 1;
		}
		m_commited = commited;
		return 0;
	}
	return 0;
}

int32_t RaftLog::appliedTo(uint64_t applied)
{
	if(applied == 0)
	{
		return 0;
	}
	if(m_commited < applied || applied < m_applied)
	{
		m_logger->Trace("applied is out of range");
		return 1;
	}
	m_applied = applied;
	return 0;
}

void RaftLog::stableTo(uint64_t index, uint64_t term)
{
	m_unstable->stableTo(index, term);
}

void RaftLog::stableSnapTo(uint64_t index)
{
        m_unstable->stableSnapTo(index);
}

int32_t RaftLog::lastTerm(uint64_t& term)
{
        uint64_t last_index;
        int32_t ret = lastIndex(last_index);
        if(ret != 0)
        {
                return 1;
        }
	return  Term(last_index, term);
}

bool RaftLog::isUpToDate(uint64_t index, uint64_t term)
{
	uint64_t last_index;
        int32_t ret = lastIndex(last_index);
        if(ret != 0)
        {
                return false;
        }
	uint64_t last_term;
	ret = lastTerm(last_term);
        if(ret != 0)
        {
                return false;
        }
	return (term > last_term || (term == last_term && index >= last_index));
}

bool RaftLog::matchTerm(uint64_t index, uint64_t term)
{
	uint64_t real_term;
	int32_t ret = Term(index, real_term);
	if(ret != 0)
	{
		return false;
	}
	return real_term == term;
}

bool RaftLog::maybeCommit(uint64_t index, uint64_t term)
{
	if(index > m_commited)
	{
		if(!matchTerm(index, term))
		{
			return false;
		}
		if(0 != commitTo(index))
		{
			return false;
		}
		return true;
	}
	return false;
}

void RaftLog::restore(raftpb::Snapshot& ss)
{
	m_commited = ss.mutable_metadata()->index();
	m_unstable->restore(&ss);
}
}










