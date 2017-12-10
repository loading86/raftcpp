#ifndef __STATUS__H__
#define __STATUS__H__
#include "progress.h"
#include "raft.pb.h"
namespace raft {
    class Status:public raftpb::HardState, public SoftState
}
#endif