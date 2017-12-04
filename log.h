#include <string>
#include <inttypes.h>
#include "storage.h"
#include "log_unstable.h"
#include "logger.h"
using namespace std;
namespace raft
{
class RaftLog
{
    private:
        Storage* m_storage;
	Unstable* m_unstable;
	uint64_t m_commited;
	uint64_t m_applied;
	Logger* m_logger;

    private:
	RaftLog(Storage* storage, Logger* logger);
    public:
	static RaftLog* NewLog(Storage* storage, Logger* logger);
	int32_t firstIndex(uint64_t& index);
	int32_t lastIndex(uint64_t& index);
	void unstableEntries(std::vector<raftpb::Entry>& entries);
	int32_t maybeAppend(uint64_t index, uint64_t term, uint64_t commited, const std::vector<raftpb::Entry>& entries, uint64_t& lastnewi);
	int32_t append(const std::vector<raftpb::Entry>& entries, uint64_t& index);
	uint64_t findConflict(const std::vector<raftpb::Entry>& entries);
	int32_t Term(uint64_t index, uint64_t& term);
	int32_t Entries(uint64_t index, uint64_t maxsize, std::vector<raftpb::Entry>& entries);
	int32_t allEntries(std::vector<raftpb::Entry>& entries);
	int32_t slice(uint64_t lo, uint64_t hi, uint64_t maxSize, std::vector<raftpb::Entry>& entries);
	int32_t nextEnts(std::vector<raftpb::Entry>& entries);
	bool hasNextEnts();
	int32_t snapshot(raftpb::Snapshot& ss);
	int32_t commitTo(uint64_t commited);
	int32_t appliedTo(uint64_t applied);
	void stableTo(uint64_t index, uint64_t term);
	void stableSnapTo(uint64_t index);
	int32_t lastTerm(uint64_t& term);
	bool isUpToDate(uint64_t index, uint64_t term);
	bool matchTerm(uint64_t index, uint64_t term);
	bool maybeCommit(uint64_t index, uint64_t term);
	void restore(raftpb::Snapshot& ss);
	int32_t mustCheckOutOfBounds(uint64_t lo, uint64_t hi);
	
};
}
