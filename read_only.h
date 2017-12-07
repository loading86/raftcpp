#ifndef __READ__ONLY__H__
#define __READ__ONLY__H__
#include "raft.h"
#include "raft.pb.h"
#include "read_only.h"
namespace raft {
struct ReadState {
    uint64_t index_;
    std::string request_ctx_;
};
class ReadIndexStatus {
private:
    raftpb::Message request_;
    uint64_t index_;
    std::map<uint64_t, bool> acks_;

public:
    ReadIndexStatus();
    ReadIndexStatus(const raftpb::Message& request, uint64_t index);
    void Ack(uint64_t index);
    int32_t AckNum() { return acks_.size(); }
    raftpb::Message& GetRequest() { return request_; }
};
class ReadOnly {
private:
    ReadOnlyOption option_;
    std::map<std::string, ReadIndexStatus*> pending_read_index_;
    std::vector<std::string> read_index_queue_;
    ReadOnly(ReadOnlyOption& option);

public:
    static ReadOnly* NewReadOnly(ReadOnlyOption& option);
    void AddRequest(uint64_t index, const raftpb::Message& message);
    int32_t RecvAck(const raftpb::Message& message);
    int32_t Advance(const raftpb::Message& message, std::vector<ReadIndexStatus*>& status);
    std::string LastPendingRequest();
};
}
#endif
