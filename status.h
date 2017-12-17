#ifndef __STATUS__H__
#define __STATUS__H__
#include "progress.h"
#include "raft.pb.h"
#include "raft.h"
namespace raft {
    class Status
    {
        public:
            uint64_t id_;
            uint64_t applied_;
            std::map<uint64_t, Progress*> progress_;
            uint64_t lead_transferee_;
            raftpb::HardState hs_;
            SoftState ss_;
            Status();
    };

    Status GetStatus(Raft* rf);
}
#endif