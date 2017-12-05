#include "log_unstable.h"
namespace raft {
Unstable::Unstable() {
  snapshot_ = nullptr;
  offset_ = 0;
}

int32_t Unstable::MaybeFirstIndex(uint64_t &index) {
  if (snapshot_ != nullptr) {
    index = snapshot_->metadata().index() + 1;
    return 0;
  }
  return 1;
}

int32_t Unstable::MaybeLastIndex(uint64_t &index) {
  if (!entries_.empty()) {
    index = offset_ + entries_.size() - 1;
    return 0;
  }
  if (snapshot_ != nullptr) {
    index = snapshot_->metadata().index();
    return 0;
  }
  return 1;
}

int32_t Unstable::MaybeTerm(uint64_t index, uint64_t &term) {
  if (index < offset_) {
    if (snapshot_ != nullptr) {
      if (snapshot_->metadata().index() == index) {
        term = snapshot_->metadata().term();
        return 0;
      }
    }
    return 1;
  }
  uint64_t last_index;
  int32_t ret = MaybeLastIndex(last_index);
  if (ret != 0 || index > last_index) {
    return 1;
  }
  term = entries_[index - offset_].term();
  return 0;
}

void Unstable::StableTo(uint64_t index, uint64_t term) {
  uint64_t real_term;
  int32_t ret = MaybeTerm(index, real_term);
  if (ret == 0 && real_term == term && index >= offset_) {
    entries_.erase(entries_.begin(),
                    entries_.begin() + index + 1 - offset_);
    offset_ = index + 1;
  }
}

void Unstable::StableSnapTo(uint64_t index) {
  if (snapshot_ != nullptr && snapshot_->metadata().index() == index) {
    snapshot_ = nullptr;
  }
}

void Unstable::Restore(raftpb::Snapshot *ss) {
  offset_ = ss->metadata().index() + 1;
  entries_.clear();
  snapshot_ = ss;
}

void Unstable::TruncateAndAppend(const std::vector<raftpb::Entry> &entries) {
  uint64_t after = entries[0].index();
  if (after == offset_ + entries_.size()) {
    entries_.reserve(entries_.size() + entries.size());
    std::copy(entries.begin(), entries.end(), entries_.end());
  } else if (after <= offset_) {
    offset_ = after;
    entries_ = entries;
  } else {
    uint64_t len = entries[entries.size() - 1].index() - offset_ + 1;
    entries_.reserve(len);
    uint64_t origin_left_len = len - entries.size();
    std::copy(entries.begin(), entries.end(),
              entries_.begin() + origin_left_len);
  }
}

int32_t Unstable::MustCheckOutOfBounds(uint64_t lo, uint64_t hi) {
  if (hi < lo) {
    return 1;
  }
  uint64_t upper = offset_ + entries_.size();
  if (lo < offset_ || hi > upper) {
    return 1;
  }
  return 0;
}

int32_t Unstable::Slice(uint64_t lo, uint64_t hi,
                        std::vector<raftpb::Entry> &entries) {
  int32_t ret = MustCheckOutOfBounds(lo, hi);
  if (ret != 0) {
    return ret;
  }
  uint64_t left = lo - offset_;
  uint64_t right = hi - offset_;
  entries.reserve(right - left + 1);
  std::copy(entries_.begin(), entries_.end(), entries.begin());
  return 0;
}
}
