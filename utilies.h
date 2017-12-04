#ifndef __UTILIES__H__
#define __UTILIES__H__
#include <vector>
#include "raft.pb.h"
namespace raft
{
void limitSize(std::vector<raftpb::Entry>& entries, uint64_t maxSize);
}
#endif
