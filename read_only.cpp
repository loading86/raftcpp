#include "read_only.h"
namespace raft
{
	ReadIndexStatus::ReadIndexStatus()
	{
		index_ = 0;
	}
	
	ReadIndexStatus::ReadIndexStatus(const raftpb::Message& request, uint64_t index)
	{
		index_ = index;
		request_ = request;
	}
	
	void ReadIndexStatus::Ack(uint64_t index)
	{
		acks_[index] = true;
	}

	ReadOnly::ReadOnly(ReadOnlyOption& option)
	{
		option_ = option;
	}
	
	ReadOnly* ReadOnly::NewReadOnly(ReadOnlyOption& option)
	{	
		return new ReadOnly(option);
	}

	void ReadOnly::AddRequest(uint64_t index, const raftpb::Message& message)
	{
		std::string ctx = message.entries(0).data();
		if(pending_read_index_.find(ctx) != pending_read_index_.end())
		{
			return;
		}
		ReadIndexStatus* status = new ReadIndexStatus(message, index);
		pending_read_index_[ctx] = status;
		read_index_queue_.push_back(ctx);
	}

	int32_t ReadOnly::RecvAck(const raftpb::Message& message)
	{
		auto it = pending_read_index_.find(message.context());
		if(it == pending_read_index_.end())
                {
                        return;
                }
		it->Ack(message.from());
		return it->AckNum() + 1;		
	}
	
	int32_t ReadOnly::Advance(const raftpb::Message& message, std::vector<ReadIndexStatus*>& status)
	{
		int32_t offset = 0;
		bool found = false;
		std::string ctx = message.context();
		std::vector<ReadIndexStatus*> read_index_statuss;
		for(auto& okctx: read_index_queue_)
		{
			offset++;
			auto it = pending_read_index_.find(okctx);
			if(it == pending_read_index_.end())
			{
				//todo
			}
			read_index_statuss.push_back(it->second);
			if(okctx == ctx)
			{
				found = true;
				break;
			}
		}
		

	}
}
