#include "include/ARPTable.h"

ARPTable* ARPTable::instance = nullptr;

ARPTable::~ARPTable()
{
    for (auto i : table)
        delete i.second;
}

ARPTable* ARPTable::get_instance()
{
    if (!instance)
        instance = new ARPTable();
    return instance;
}

void ARPTable::insert(__u32 ip, uint8_t mac[])
{
    table.insert(std::make_pair(ip, new ARPEntry(ip, mac)));
}

ARPEntry* ARPTable::get_entry(__u32 ip)
{
    std::map<__u32, ARPEntry*>::iterator it = table.find(ip);
    if (it == table.end())
        return nullptr;
    return it->second;
}

