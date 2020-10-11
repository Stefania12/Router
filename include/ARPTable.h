#include "skel.h"
#include "ARPEntry.h"
#include <map>

#ifndef ARPTABLE_H
#define ARPTABLE_H

/* Holds the ARP table entries and inserts/retrieves entries based on the IP (and mac). */
class ARPTable
{
    public:
    /* Returns the unique instance of the ARP table. */
    static ARPTable* get_instance();

    /* Destructor that frees the allocated memory. */
    ~ARPTable();
    
    /* Constructs and inserts an ARP entry into the tabele. */
    void insert(__u32 ip, uint8_t mac[]);

    /* Returns the entry with the given IP or nullptr if it is not in the table. */
    ARPEntry* get_entry(__u32 ip);


    private:
    /* Unique instance of the ARP table. */
    static ARPTable* instance;

    /* Holds the entries of the ARP table by IP. */
    std::map<__u32, ARPEntry*> table;
};

#endif
