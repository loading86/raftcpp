#include "ulities.h"
void limitSize(std::vector<raftpb.Entry>& entries, uint64_t maxSize)
{
	if(entries.empty())
	{
		return entries;
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
	std::erase(entries.begin() + index, entries.end());
}
