#ifndef __STORAGE__H__
#define __STORAGE__H__
#include "raftpb/raft.pb.h"
#include <inttypes.h>
#include <string>
#include <vector>

enum StorageErr
{
    ErrCompacted = 1,
    ErrSnapOutOfDate = 2;
    ErrUnavailable = 3,
    ErrSnapshotTemporarilyUnavailable = 4,
    ErrParam = 5
};

class Storage
{
    public:
        int32_t InitialState(raftpb.HardState& hs, raftpb.ConfState& cs);
        int32_t Entries(uint64_t lo, uint64_t hi, uint64_t maxSize,std::vestor<raftpb.Entry>& entries);
        int32_t Term(uint64_t index, uint64_t& term);
        int32_t LastIndex(uint64_t& index);
        int32_t FirstIndex(uint64_t& index);
        int32_t SnapShot(raftpb.Snapshot& ss);
        virtual ~Storage() = 0;
};

class MemoryStorage:public Storage
{
    private:
        raftpb.HardState m_hardstat;
        raftpb.Snapshot m_snapshot;
        std::vector<raftpb.Entry> m_entries;
        uint64_t firstIndex();
        uint64_t lastIndex();
    public:
        MemoryStorage();
        int32_t SetHardState(raftpb.HardState& hs);
        int32_t ApplySnapshot(raftpb.Snapshot ss);
        int32_t CreateSnapshot(uint64_t index, raftpb.ConfState cs, std::string& data, raftpb.Snapshot& ss);
        int32_t Compact(uint64_t index);
        int32_t Append(std::vestor<raftpb.Entry>& entries);
};

#endif
