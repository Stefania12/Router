#include "skel.h"
#include "RTable.h"
#include "ARPTable.h"
#include <map>

#ifndef ROUTER_H
#define ROUTER_H

class Router
{
    public:
    /* Destructor that frees memory allocated for rtable and arp_table. */
    ~Router();
    
    /* Retrieves the unique Router instance. */
    static Router* get_instance();

    /* Manages the ARP message m as it calls manage_arp_request or manage_arp_reply, depending on m's type (ARP REQUEST/REPLY) */
    void manage_arp_message(packet m);

    /* Manages the IP message m:
    1) drops packet if checksum is incorrect
    2) if the message is for the router, calls  manage_ip_msg_for_me, otherwise it follows the next steps
    3) drops packet if ttl <= 1 and calls send_time_exceeded
    4) looks for a suitable entry in the routing table that matches the destination IP
    5) if no entry matches the destination IP, calls send_desination_unreachable
    6) if there is a matching entry in the routing table, check if in the ARP table if the mac is known
    7) if the mac is unknown, calls send_arp_request and inserts the message in the multimap called pending_messages
    8) if the mac is known, then decrease ttl and recalculate checksum, updates the packet interface and ether header and sends the packet */
    void manage_ip_message(packet m);


    private:
    /* Unique instance of class */
    static Router* instance;

    /* The routing table of the router */
    RTable* rtable;

    /* The ARP table of the router */
    ARPTable* arp_table;

    /* Holds messages that have to be forwarded when the router learns about the destination mac */
    std::multimap<__u32, packet> pending_messages;

    /* Constructor that initializes the routing table and ARP table */
    Router();

    /* Computes checksum over vdata and its length */
    uint16_t get_checksum(void *vdata, size_t length);

    /* Is called when the router receives an ARP REQUEST type of message. 
    If the packet is for the router, then it answers with ARP REPLY. */
    void manage_arp_request(packet m);

    /* Is called when the router receives an ARP reply message.
    The ip-mac pair is saved into the ARP table and all pending packets for that IP are sent by calling send_pending_messages. */
    void manage_arp_reply(packet m);

    /* Finds the range of packets for the specified IP, updates the ether header and interface and decreases ttl of IP messages and recomputes checksum.
    Sends those packets and removes them from the pending messages multimap. */
    void send_pending_messages(__u32 ip, uint8_t mac[]);

    /* Is called when the router receives an IP message for it. 
    If the packet m is of type ICMP ECHO, it replies with ICMP ECHOREPLY. */
    void manage_ip_msg_for_me(packet m);

    /* Is called when the original IP packet has a ttl <= 1. Sends back an ICMP TIME EXCEEDED packet. */
    void send_time_exceeded(packet original_message);

    /* Is called when there is no match for the destination IP in the routing table. Sends back an ICMP DESTIANTION UNREACHABLE packet. */
    void send_desination_unreachable(packet original_message);

    /* Is called when the router does not know the mac for an IP in the routing table. 
    Sends an ARP REQUEST into the network (with the broadcast mac address). */
    void send_arp_request(RTableEntry *rtable_entry);
};

#endif
