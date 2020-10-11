#include "include/ARPEntry.h"

ARPEntry::ARPEntry(__u32 ip, uint8_t mac[6])
{
    this->ip = ip;
    memcpy(this->mac, mac, 6);
}