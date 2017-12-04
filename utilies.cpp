#include <algorithm>
#include "utilies.h"
namespace raft
{
void limitSize(std::vector<raftpb::Entry>& entries, uint64_t maxSize)
{
	if(entries.empty())
	{
		return;
	}
	uint64_t size = 0;
	uint64_t index = 0;
	for(; index < entries.size(); index++)
	{
		size += entries[index].ByteSize();
		if(size > maxSize)
		{
			break;
		}
	}
	if(index == entries.size())
	{
		return;
	}
	entries.erase(entries.begin() + index, entries.end());
}
}