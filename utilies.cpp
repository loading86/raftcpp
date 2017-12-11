#include "utilies.h"
#include <time.h>
#include <algorithm>
#include <random>
namespace raft {
void LimitSize(std::vector<raftpb::Entry>& entries, uint64_t max_size)
{
    if (entries.empty()) {
        return;
    }
    uint64_t size = 0;
    uint64_t index = 0;
    for (; index < entries.size(); index++) {
        size += entries[index].ByteSize();
        if (size > max_size) {
            break;
        }
    }
    if (index == entries.size()) {
        return;
    }
    entries.erase(entries.begin() + index, entries.end());
}

bool IsHardStateEqual(raftpb::HardState& left, raftpb::HardState& right)
{
    return left.term() == right.term() && left.vote() == right.vote() && left.commit() == right.commit();
}

bool IsHardStateEmpty(raftpb::HardState& state)
{
    return state.term() == 0 && state.vote() == 0 && state.commit() == 0;
}

bool IsSnapshotEmpty(const raftpb::Snapshot& ss)
{
    return ss.metadata().index() == 0;
}
std::random_device rd;
int32_t RandomNum(int32_t scale)
{
    return rd() % scale;
}
}