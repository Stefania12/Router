#include "include/RTableEntry.h"
#include <iostream>

RTableEntry::RTableEntry(std::string prefix, std::string next_hop, std::string mask, __u32 interface)
{
    this->prefix = ntohl(inet_addr(prefix.c_str()));
    this->next_hop = ntohl(inet_addr(next_hop.c_str()));
    this->mask = ntohl(inet_addr(mask.c_str()));
    this->interface = interface;
}