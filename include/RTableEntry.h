#include "skel.h"
#include <string>

#ifndef RTABLEENTRY_H
#define RTABLEENTRY_H

/* Holds the routing table information. */
class RTableEntry
{
    public:
    /* Information in host byte order. */
    __u32 prefix, next_hop, mask, interface;

    /* Returns the routing table entry based on the string information given. */
    RTableEntry(std::string prefix, std::string next_hop, std::string mask, __u32 interface);
};

#endif
