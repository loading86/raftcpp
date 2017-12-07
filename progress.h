#ifndef __PROGRESS__H__
#define __PROGRESS__H__
#include <inttypes.h>
#include <list>
namespace raft {
enum ProgressStateType {
    ProgressStateProbe = 0,
    ProgressStateReplicate = 1,
    ProgressStateSnapshot = 2
};

class Inflights {
private:
    int32_t start_;
    int32_t count_;
    int32_t size_;
    std::list<uint64_t> buffer_;
    Inflights(int size);

public:
    static Inflights* NewInflights(int32_t size);
    void Add(uint64_t inflight);
    void GrowBuf();
    void FreeTo(uint64_t to);
    void FreeFirstOne();
    bool Full();
    void Reset();
};
class Progress {
private:
    uint64_t match_;
    uint64_t next_;
    ProgressStateType state_;
    bool paused_;
    uint64_t pending_snaphot_;
    bool recent_active_;
    Inflights* inflights_;

public:
    void ResetState(ProgressStateType state);
    void BecomeProbe();
    void BecomeReplicate();
    void BecomeSnapshot(uint64_t snapshot_index);
    bool MaybeUpdate(uint64_t index);
    void OptimisticUpdate(uint64_t index);
    bool MaybeDecrTo(uint64_t rejected, uint64_t last);
    void Pause();
    void Resume();
    bool IsPaused();
    void SnapshotFailure();
    bool NeedSnapshotAbort();
};
}
#endif