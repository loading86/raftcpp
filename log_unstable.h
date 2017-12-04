#ifndef __LOG_UNSTABLE__H__
#define __LOG_UNSTABLE__H__
#include "raftpb/raft.pb.h"
#include <vector>
class Unstable
{
    private:
        raftpb.Snapshot* m_snapshot;
        std::vector<raftpb.Entry> m_entries;
        uint64_t m_offset;
        Logger* m_logger;
    public:
        Unstable();
        int32_t maybeFirstIndex(uint64_t& index);
        int32_t maybeLastIndex(uint64_t& index);
        int32_t maybeTerm(uint64_t index, uint64_t& term);
        void stableTo(uint64_t index, uint64_t term);
        void stableSnapTo(uint64_t index);
        void restore(raftpb.Snapshot* ss);
        void truncateAndAppend(const std::vector<raftpb.Entry>& entries);
        int32_t mustCheckOutOfBounds(uint64_t lo, uint64_t hi);
	void setLogger(Logger* logger){m_logger = logger;}
	void setOffset(uint64_t offset){m_offset = offset;}
	uint64_t getOffset(){return m_offset;}
	raftpb.Snapshot* getSnapshot(){return m_snapshot;}
	void Entries(std::vector<raftpb.Entry>& entries){entries =  m_entries;}
	int32_t slice(uint64_t lo, uint64_t hi, std::vector<raftpb.Entry>& entries);
};
#endif
