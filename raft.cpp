#include "raft.h"
namespace raft {

bool Config::Validate()
{
    if (id_ < 0) {
        return false;
    }
    if (heart_beat_tick_ < 0) {
        return false;
    }
    if (election_tick_ <= heart_beat_tick_) {
        return false;
    }
    if (!storage_) {
        return false;
    }
    if (max_inflight_msgs_ <= 0) {
        return false;
    }
    if (!logger_) {
        return false;
    }
    if (read_only_option_ == ReadOnlyLeaseBased && !check_quorum_) {
        return false;
    }
    return true;
}
}
