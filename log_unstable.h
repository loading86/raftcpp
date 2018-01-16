#ifndef __LOG_UNSTABLE__H__
#define __LOG_UNSTABLE__H__
#include "spdlog/logger.h"
#include "spdlog/spdlog.h"
#include "raftpb/raft.pb.h"
#include <vector>
namespace raft {
class Unstable {
private:
    raftpb::Snapshot* snapshot_ = nullptr;
    std::vector<raftpb::Entry> entries_;
    uint64_t offset_ = 0;
    std::shared_ptr<spdlog::logger> logger_ = nullptr;

public:
    Unstable();
    int32_t MaybeFirstIndex(uint64_t& index);
    int32_t MaybeLastIndex(uint64_t& index);
    int32_t MaybeTerm(uint64_t index, uint64_t& term);
    void StableTo(uint64_t index, uint64_t term);
    void StableSnapTo(uint64_t index);
    void Restore(const raftpb::Snapshot* ss);
    void TruncateAndAppend(const std::vector<raftpb::Entry>& entries);
    int32_t MustCheckOutOfBounds(uint64_t lo, uint64_t hi);
    void SetLogger(std::shared_ptr<spdlog::logger> logger) { logger_ = logger; }
    void SetOffset(uint64_t offset) { offset_ = offset; }
    uint64_t GetOffset() { return offset_; }
    raftpb::Snapshot* GetSnapshot() { return snapshot_; }
    void Entries(std::vector<raftpb::Entry>& entries) { entries = entries_; }
    int32_t Slice(uint64_t lo, uint64_t hi, std::vector<raftpb::Entry>& entries);
};
}
#endif
