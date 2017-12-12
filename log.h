#include "log_unstable.h"
#include "logger.h"
#include "storage.h"
#include <inttypes.h>
#include <string>
using namespace std;
namespace raft {
class RaftLog {
private:
    Storage* storage_;
    Unstable* unstable_;
    uint64_t commited_;
    uint64_t applied_;
    Logger* logger_;

private:
    RaftLog(Storage* storage, Logger* logger);

public:
    static RaftLog* NewLog(Storage* storage, Logger* logger);
    int32_t FirstIndex(uint64_t& index);
    int32_t LastIndex(uint64_t& index);
    void UnstableEntries(std::vector<raftpb::Entry>& entries);
    int32_t MaybeAppend(uint64_t index,
        uint64_t term,
        uint64_t commited,
        const std::vector<raftpb::Entry>& entries,
        uint64_t& lastnewi);
    int32_t Append(const std::vector<raftpb::Entry>& entries, uint64_t& index);
    uint64_t FindConflict(const std::vector<raftpb::Entry>& entries);
    int32_t Term(uint64_t index, uint64_t& term);
    int32_t Entries(uint64_t index,
        uint64_t maxsize,
        std::vector<raftpb::Entry>& entries);
    int32_t AllEntries(std::vector<raftpb::Entry>& entries);
    int32_t Slice(uint64_t lo,
        uint64_t hi,
        uint64_t max_size,
        std::vector<raftpb::Entry>& entries);
    int32_t NextEnts(std::vector<raftpb::Entry>& entries);
    bool HasNextEnts();
    int32_t Snapshot(raftpb::Snapshot& ss);
    int32_t CommitTo(uint64_t commited);
    int32_t AppliedTo(uint64_t applied);
    void StableTo(uint64_t index, uint64_t term);
    void StableSnapTo(uint64_t index);
    int32_t LastTerm(uint64_t& term);
    bool IsUpToDate(uint64_t index, uint64_t term);
    bool MatchTerm(uint64_t index, uint64_t term);
    bool MaybeCommit(uint64_t index, uint64_t term);
    void Restore(const raftpb::Snapshot& ss);
    int32_t MustCheckOutOfBounds(uint64_t lo, uint64_t hi);
    uint64_t Commited(){return commited_;}
    void SetCommited(uint64_t commited){ commited_ = commited;}
    uint64_t ZeroTermOnErrCpmpacted(uint64_t term, int32_t error);
    uint64_t Applied(){return applied_;}
};
}
