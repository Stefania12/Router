#include "skel.h"
#include "RTableEntry.h"
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#ifndef RTABLE_H
#define RTABLE_H

/* Comparator for the prefixes of IPs. */
class CompareByPrefix
{
    public:
    bool operator()(const __u32 pref1, const __u32 pref2)
    {
        return (pref1 < pref2);
    }
};

/* Comparator for the masks of IPs. */
class CompareByMask
{
    public:
    bool operator()(const __u32 m1, const __u32 m2)
    {
        return (m1 > m2);
    }
};

/* Parses the routing table and holds the routing table and gives the best matching entry for a given IP. */
class RTable
{
    public:
    /* Returns unique instance of the routing table. */
    static RTable* get_instance();

    /* Destructor that frees the allocated memory. */
    ~RTable();

    /* Returns the best mathing entry for the given IP in host byte order. */
    RTableEntry* get_best_entry(__u32 ip);

    private:
    /* Unique instance of the routing table. */
    static RTable* instance;

    /* Holds the routing table entries first by prefix then by mask. */
    std::map<__u32, std::map<__u32, RTableEntry*, CompareByMask>, CompareByPrefix> table;

    /* Constructor that parses the routing table. */
    RTable();

    /* Returns a routing table entry based of the string with its information. */
    RTableEntry* get_entry (std::string entry_info);
};

#endif
