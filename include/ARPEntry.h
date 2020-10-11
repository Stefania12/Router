#include "skel.h"
#ifndef ARPENTRY_H
#define ARPENTRY_H
/* Holds the ip (in host byte order) and mac pairs. */
class ARPEntry
{
    public:
    __u32 ip;
    uint8_t mac[6];

    /* Constructor that initializes the ip and mac. */
    ARPEntry(__u32 ip, uint8_t mac[]);
};

#endif
