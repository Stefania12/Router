#include "include/Router.h"

Router* Router::instance = nullptr;

Router::Router()
{
    rtable = RTable::get_instance();
    arp_table = ARPTable::get_instance();
}

Router::~Router()
{
    delete rtable;
    delete arp_table;
}

Router* Router::get_instance()
{
    if (!instance)
        instance = new Router();
    return instance;
}

// Checksum function from the lab
uint16_t Router::get_checksum(void *vdata, size_t length) {
	// Cast the data pointer to one that can be indexed.
	char* data=(char*)vdata;

	// Initialise the accumulator.
	uint64_t acc=0xffff;

	// Handle any partial block at the start of the data.
	unsigned int offset=((uintptr_t)data)&3;
	if (offset) {
		size_t count=4-offset;
		if (count>length) count=length;
		uint32_t word=0;
		memcpy(offset+(char*)&word,data,count);
		acc+=ntohl(word);
		data+=count;
		length-=count;
	}

	// Handle any complete 32-bit blocks.
	char* data_end=data+(length&~3);
	while (data!=data_end) {
		uint32_t word;
		memcpy(&word,data,4);
		acc+=ntohl(word);
		data+=4;
	}
	length&=3;

	// Handle any partial block at the end of the data.
	if (length) {
		uint32_t word=0;
		memcpy(&word,data,length);
		acc+=ntohl(word);
	}

	// Handle deferred carries.
	acc=(acc&0xffffffff)+(acc>>32);
	while (acc>>16) {
		acc=(acc&0xffff)+(acc>>16);
	}

	// If the data began at an odd byte address
	// then reverse the byte order to compensate.
	if (offset&1) {
		acc=((acc&0xff00)>>8)|((acc&0x00ff)<<8);
	}

	// Return the checksum in network byte order.
	return htons(~acc);
}

void Router::manage_arp_message(packet m)
{
    struct ether_arp *arp_hdr = (struct ether_arp*)(m.payload + sizeof(struct ether_header));
    
    // Check the type of message and manage it
    switch (ntohs(arp_hdr->ea_hdr.ar_op))
    {
        case ARPOP_REQUEST: manage_arp_request(m);
                            break;
        case ARPOP_REPLY:   manage_arp_reply(m);
                            break;
        default: std::cout << "Unknown ARP message\n";
    }
}


void Router::manage_arp_request(packet m)
{
    struct ether_arp *arp_hdr = (struct ether_arp*)(m.payload + sizeof(struct ether_header));
    __u32 dest_ip_big_end = *(__u32*)arp_hdr->arp_tpa;
    __u32 my_ip_big_end = inet_addr(get_interface_ip(m.interface));

    // Check if the destination IP is one of the router IPs and answer with ARP REPLY
    if (my_ip_big_end == dest_ip_big_end)
    {
        // Update ether header (destination = old source)
        struct	ether_header *eth_hdr = (struct ether_header*)m.payload;
        memcpy(eth_hdr->ether_dhost, eth_hdr->ether_shost, sizeof(eth_hdr->ether_dhost));
        get_interface_mac(m.interface, eth_hdr->ether_shost);

        // Update arp table
        arp_hdr->ea_hdr.ar_op = htons(ARPOP_REPLY);
        memcpy(arp_hdr->arp_tpa, arp_hdr->arp_spa, sizeof(arp_hdr->arp_spa));
        memcpy(arp_hdr->arp_tha, arp_hdr->arp_sha, sizeof(arp_hdr->arp_sha));
        memcpy(arp_hdr->arp_spa, &my_ip_big_end, sizeof(arp_hdr->arp_spa));
        get_interface_mac(m.interface, arp_hdr->arp_sha);

        send_packet(m.interface, &m);
    }
}

void Router::manage_arp_reply(packet m)
{
    struct ether_arp *arp_hdr = (struct ether_arp*)(m.payload + sizeof(struct ether_header));
    __u32 big_end_ip = *(__u32*)arp_hdr->arp_spa;
    __u32 lil_end_ip = ntohl(big_end_ip);
    arp_table->insert(lil_end_ip, arp_hdr->arp_sha);
    send_pending_messages(lil_end_ip, arp_hdr->arp_sha);
}

void Router::send_pending_messages(__u32 ip, uint8_t mac[])
{
    // Find the range of packets with the destination IP being the ip parameter.
    std::pair <std::multimap<__u32, packet>::iterator, std::multimap<__u32, packet>::iterator> range;
    range = pending_messages.equal_range(ip);

    // Find next hop interface
    int interface = rtable->get_best_entry(ip)->interface;

    // Update the packets in the range and send them
    for (std::multimap<__u32, packet>::iterator it = range.first; it != range.second; it++)
    {
        packet m = it->second;
        m.interface = interface;

        // Update ether header
        struct	ether_header *eth_hdr = (struct ether_header*)m.payload;
        memcpy(eth_hdr->ether_dhost, mac, sizeof(eth_hdr->ether_dhost));
        get_interface_mac(m.interface, eth_hdr->ether_shost);

        // Decrease ttl and recompute checksum of IP packets
        if ((ntohs(eth_hdr->ether_type)) == ETHERTYPE_IP)
        {
            struct iphdr* ip_hdr = (struct iphdr*)(m.payload+sizeof(struct ether_header));
            ip_hdr->ttl--;
            ip_hdr->check = 0;
            ip_hdr->check = get_checksum(ip_hdr, sizeof(struct iphdr));
        }
        
        send_packet(m.interface, &m);
    }
    // Packets are sent so delete them from the pending mesages
    pending_messages.erase(range.first, range.second);
}

void Router::manage_ip_message(packet m)
{
    struct iphdr *ip_hdr = (struct iphdr*)(m.payload + sizeof (struct ether_header));
    __u32 my_ip_big_end = inet_addr(get_interface_ip(m.interface));
    __u32 dest_ip_big_end = ip_hdr->daddr;

    __u16 old_checksum = ip_hdr->check;
    ip_hdr->check = 0;
    __u16 new_checksum = get_checksum(ip_hdr, sizeof(struct iphdr));
    ip_hdr->check = new_checksum;

    // Drop packets with wrong checksum
    if (old_checksum != new_checksum)
        return;

    // Manage the packet if it is for the router IP or otherwise
    if (my_ip_big_end == dest_ip_big_end)
        manage_ip_msg_for_me(m);
    else
    {
        // Send time exceeded packet if ttl <= 1
        if (ip_hdr->ttl <= 1)
        {
            send_time_exceeded(m);
            return;
        }

        // Find the match for the destination IP in the routing table
        RTableEntry *best_rtable_entry = rtable->get_best_entry(ntohl(dest_ip_big_end));
        // Send destination unreachable when no match is found
        if (best_rtable_entry == nullptr)
        {
            send_desination_unreachable(m);
        }
        else
        {
            // Look for the mac of the next hop in the arp table
            ARPEntry *arp_entry = arp_table->get_entry(best_rtable_entry->next_hop);
            // Send arp request when the mac is unknown and add packet to the pending messages
            if (arp_entry == nullptr)
            {
                send_arp_request(best_rtable_entry);
                pending_messages.insert(std::make_pair(best_rtable_entry->next_hop, m));
            }
            else
            {
                // Update IP header
                ip_hdr->ttl--;
                ip_hdr->check = 0;
                ip_hdr->check = get_checksum(ip_hdr, sizeof(struct iphdr));
                m.interface = best_rtable_entry->interface;

                // Update ether header
                struct ether_header *eth_hdr = (struct ether_header*)m.payload;
                memcpy(eth_hdr->ether_dhost, arp_entry->mac, sizeof(eth_hdr->ether_dhost));
                get_interface_mac(m.interface, eth_hdr->ether_shost);
                
                send_packet(m.interface, &m);
            }
        }
    }
}

void Router::manage_ip_msg_for_me(packet m)
{
    struct iphdr *ip_hdr = (struct iphdr*)(m.payload + sizeof (struct ether_header));
    // Answer only for icmp echo messages
    if (ip_hdr->protocol == IPPROTO_ICMP)
    {
        struct icmphdr *icmp_hdr = (struct icmphdr*)(m.payload + sizeof(struct ether_header) + sizeof (struct iphdr));
        if (icmp_hdr->type == ICMP_ECHO)
        {
            // Update ether header
            struct ether_header *eth_hdr = (struct ether_header*)m.payload;
            memcpy(eth_hdr->ether_dhost, eth_hdr->ether_shost, sizeof(eth_hdr->ether_dhost));
            get_interface_mac(m.interface, eth_hdr->ether_shost);

            // Update IP header
            ip_hdr->daddr = ip_hdr->saddr;
            ip_hdr->saddr = inet_addr(get_interface_ip(m.interface));
            ip_hdr->check = 0;
            ip_hdr->check = get_checksum(ip_hdr, sizeof(struct iphdr));

            // Update icmp header
            icmp_hdr->type = ICMP_ECHOREPLY;
            icmp_hdr->checksum = 0;
            icmp_hdr->checksum = get_checksum(icmp_hdr, m.len - sizeof(struct ether_header) - sizeof(struct iphdr));
            
            send_packet(m.interface, &m);
        }
    }
}

void Router::send_time_exceeded(packet initial_msg)
{
    packet m;
    m.interface = initial_msg.interface;
    m.len = sizeof(struct ether_header) + sizeof(struct iphdr) + sizeof(struct icmphdr);
    struct ether_header *eth_hdr = (struct ether_header*)m.payload;
    struct iphdr *ip_hdr = (struct iphdr*)(m.payload + sizeof(struct ether_header));
    struct icmphdr *icmp_hdr = (struct icmphdr*)(m.payload + sizeof(struct ether_header) + sizeof (struct iphdr));

    // Init ether header
    memcpy(eth_hdr->ether_dhost, ((struct ether_header*)initial_msg.payload)->ether_shost, sizeof(eth_hdr->ether_dhost));
    get_interface_mac(m.interface, eth_hdr->ether_shost);
    eth_hdr->ether_type = htons(ETHERTYPE_IP);

    // Init IP header
    ip_hdr->daddr = ((struct iphdr*)initial_msg.payload+sizeof(struct ether_header))->saddr;
    ip_hdr->saddr = inet_addr(get_interface_ip(m.interface));
    ip_hdr->ttl = 0xff;
    ip_hdr->protocol  = IPPROTO_ICMP;
    ip_hdr->tot_len = htons((short)(sizeof(struct iphdr)) + sizeof(struct icmphdr));
    ip_hdr->frag_off = 0;
    ip_hdr->id = ((struct iphdr*)(initial_msg.payload + sizeof(struct ether_header)))->id;
    ip_hdr->ihl = 5;
    ip_hdr->tos = 0;
    ip_hdr->version = 4;
    ip_hdr->check = 0;
    ip_hdr->check = get_checksum(ip_hdr, sizeof(struct iphdr));

    // Update icmp header
    icmp_hdr->type = ICMP_TIME_EXCEEDED;
    icmp_hdr->code = ICMP_EXC_TTL;
    icmp_hdr->checksum = 0;
    icmp_hdr->checksum = get_checksum(icmp_hdr, sizeof(struct icmphdr));
    
    send_packet(m.interface, &m);
}

void Router::send_desination_unreachable(packet original_message)
{
    struct ether_header *old_eth_hdr = (struct ether_header*)original_message.payload;
    struct iphdr *old_ip_hdr = (struct iphdr*)(original_message.payload + sizeof(struct ether_header));

    packet m;
    m.interface = original_message.interface;
    m.len = sizeof(struct ether_header) + sizeof(struct iphdr) + sizeof(struct icmphdr);

    struct ether_header *eth_hdr = (struct ether_header*)m.payload;
    struct iphdr *ip_hdr = (struct iphdr*)(m.payload + sizeof(struct ether_header));
    struct icmphdr *icmp_hdr = (struct icmphdr*)(m.payload + sizeof(struct ether_header) + sizeof(struct iphdr));

    // Init ether header
    eth_hdr->ether_type = htons(ETHERTYPE_IP);
    memcpy(eth_hdr->ether_dhost, old_eth_hdr->ether_shost, sizeof(eth_hdr->ether_dhost));
    get_interface_mac(m.interface, eth_hdr->ether_shost);

    // Init IP header
    ip_hdr->daddr = old_ip_hdr->saddr;
    ip_hdr->saddr = inet_addr(get_interface_ip(m.interface));
    ip_hdr->ttl = 0xff;
    ip_hdr->protocol  = IPPROTO_ICMP;
    ip_hdr->tot_len = htons((short)(sizeof(struct iphdr) + sizeof(struct icmphdr)));
    ip_hdr->frag_off = 0;
    ip_hdr->id = ((struct iphdr*)(original_message.payload + sizeof(struct ether_header)))->id;
    ip_hdr->ihl = 5;
    ip_hdr->tos = 0;
    ip_hdr->version = 4;
    ip_hdr->check = 0;
    ip_hdr->check = get_checksum(ip_hdr, sizeof(struct iphdr));    

    // init icmp header
    icmp_hdr->type = ICMP_DEST_UNREACH;
    icmp_hdr->code = ICMP_NET_UNREACH;
    icmp_hdr->checksum = 0;
    icmp_hdr->checksum = get_checksum(icmp_hdr, sizeof(struct icmphdr));

    send_packet(m.interface, &m);
}


void Router::send_arp_request(RTableEntry *rtable_entry)
{
    __u32 dest_ip_big_end = htonl(rtable_entry->next_hop);
    __u32 my_ip_big_end = inet_addr(get_interface_ip(rtable_entry->interface));

    packet m;
    m.interface = rtable_entry->interface;
    m.len = sizeof(struct ether_header) + sizeof(struct ether_arp);

    struct ether_header *eth_hdr = (struct ether_header*)m.payload;
    struct ether_arp *arp_hdr = (struct ether_arp*)(m.payload + sizeof(struct ether_header));

    // Init ether header
    memset(eth_hdr->ether_dhost, 0xff, sizeof(eth_hdr->ether_dhost));
    get_interface_mac(m.interface, eth_hdr->ether_shost);
    eth_hdr->ether_type = htons(ETHERTYPE_ARP);

    // Init arp header
    arp_hdr->ea_hdr.ar_op = htons(ARPOP_REQUEST);
    arp_hdr->ea_hdr.ar_pro = htons(0x0800);
    arp_hdr->ea_hdr.ar_hln = 6;
    arp_hdr->ea_hdr.ar_hrd = htons(ARPHRD_ETHER);
    arp_hdr->ea_hdr.ar_pln = 4;
    memcpy(arp_hdr->arp_spa, &my_ip_big_end, sizeof(arp_hdr->arp_spa));
    memcpy(arp_hdr->arp_tpa, &dest_ip_big_end, sizeof(dest_ip_big_end));
    memcpy(arp_hdr->arp_sha, eth_hdr->ether_shost, sizeof(arp_hdr->arp_sha));
    memcpy(arp_hdr->arp_tha, eth_hdr->ether_dhost, sizeof(arp_hdr->arp_tha));    

    send_packet(m.interface, &m);
}