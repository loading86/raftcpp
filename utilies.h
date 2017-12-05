#ifndef __UTILIES__H__
#define __UTILIES__H__
#include "raft.pb.h"
#include <vector>
namespace raft {
void LimitSize(std::vector<raftpb::Entry>& entries, uint64_t max_size);
}
#endif
