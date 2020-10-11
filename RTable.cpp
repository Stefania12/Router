#include "RTable.h"

RTable* RTable::instance = nullptr;

RTable::RTable()
{
	std::ifstream fin("rtable.txt");
	std::string line;

	// Read line by line
	while (getline(fin, line))
	{
		// Make the entry
		RTableEntry* entry = get_entry(line);
		
		// Insert the map sorted by masks for the given prefix if it is not already there
		if (table.find(entry->prefix) == table.end())
			table.insert(std::make_pair(entry->prefix, std::map<__u32, RTableEntry*, CompareByMask>()));
		// Insert the entry in the table
		table.find(entry->prefix)->second.insert(std::make_pair(entry->mask, entry));
	}

	fin.close();
}

RTable::~RTable()
{
	for (std::pair<__u32, std::map<__u32, RTableEntry*, CompareByMask>> i : table)
		for (auto j : i.second)
			delete j.second;

}

RTable* RTable::get_instance()
{
	if (instance == nullptr)
		instance = new RTable();
	return instance;
}

RTableEntry* RTable::get_entry (std::string entry_info)
{
	// Retrieve the entry information from each word in the line
	std::string prefix, next_hop, mask;
	__u32 interface;
	int space_idx = entry_info.find(" ");
	prefix = entry_info.substr(0, space_idx);
	
	entry_info = entry_info.substr(space_idx+1);
	space_idx = entry_info.find(" ");
	next_hop = entry_info.substr(0, space_idx);

	entry_info = entry_info.substr(space_idx+1);
	space_idx = entry_info.find(" ");
	mask = entry_info.substr(0, space_idx);

	entry_info = entry_info.substr(space_idx+1);
	interface = atoi(entry_info.c_str());

	return new RTableEntry(prefix,next_hop, mask, interface);
}

RTableEntry* RTable::get_best_entry(__u32 ip)
{
	std::map<__u32,RTableEntry*>::iterator start;
	// Start looking for the match with the biggest mask and decrease it each step
	for (__u32 mask = 0xffffffff; mask > 0; mask = (mask << 1))
	{
		__u32 prefix = (ip & mask);
		std::map<__u32, std::map<__u32, RTableEntry*, CompareByMask>, CompareByPrefix>::iterator it = table.find(prefix);
		
		// if there is an element in the map with the given prefix, return the entry if looking for the mask yields something.
		if (it != table.end())
		{
			std::map<__u32, RTableEntry*, CompareByMask> m = it->second;
			std::map<__u32, RTableEntry*, CompareByMask>::iterator ip_it = m.find(mask);
			if (ip_it != m.end())
				return ip_it->second;
		}
	}

	// Return nullptr if no match was found
	return nullptr;
}
