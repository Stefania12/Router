# Router

The project contains the following C++ sources (along with their headers in
the folder include):
- ARPEntry.cpp -> used to hold the the IP and MAC pairs
- ARPTable.cpp -> implements the routing table functionality
- router.cpp -> the main function of the program
- Router.cpp -> implements the router behaviour
- RTable.cpp -> holds the routing table that is parsed from the text file upon
	     initialization
- RTableEntry.cpp -> holds the information for the entries in the routing table
- skel.cpp -> useful functions


## Functionality of the program

* Parsing the routing table:
The RTable class is a Singleton that parses the table upon initialization.
This is done by reading rtable.txt line by line and processing the information
in a function called get_entry that takes a string and returns a RTableEntry
pointer.

* Retrieval of the best matching routing table entry for a given IP:
The RTable class does this in get_best_entry. The routing table is implemented
using a map of maps. The inner maps hold RTableEntry objects that have the mask
as a key. The outer map holds these maps that have the prefix as a key. This
way, when looking for the closest IP to a given one, the function starts by
initializing the biggest mask possible (0xffffffff) and decreases it whith
left-shifting. For each mask, the prefix is computed and used for retrieving a
map with masks as keys and which have entries with that prefix. When finding
such a map, the mask is looked up. This method returns the longest prefix match
for a given IP. If nothing is found, then nullptr is returned.

Additionally, the insertion and retrieval from the routing table is done in a
time complexity of: 2*O(logn) = O(logn), where n is the number of entries.

## Flow of the program
The router.cpp file contains a while loop where a packet is received. Then,
based on its type (ARP or IP) a suitable management function from Router class
is called (manage_arp_message or manage_ip_message).

* manage_arp_message:
This function manages the packet based on the ARP type: REQUEST or REPLY.
	* REQUEST:
	   When a packet is of type ARP REQUEST, the function manage_arp_request
	is called. It checks if the destination IP is one of the router's and
	sends back an ARP REPLY packet.

	* REPLY:
	   When the router receives a packet of type ARP REPLY, the function
	manage_arp_reply updates the ARP table by inserting the new IP-MAC pair
	and then calls send_pending_messages. This function retrieves the range
	of pending messages for a given destination IP, updates their headers
	and send them, then removes them from their multimap container.



* manage_ip_message:
This function manages the IP packets in the following ways:
	* When the router receives a packet, it checks the correctness of the
	IP checksum and drops packets with a wrong checksum.

	* If the destination IP is one of the router's and is of type ICMP ECHO,
	then answer with an ICMP ECHOREPLY. This is done in manage_ip_msg_for_me.

	* If the router's IP is not the destination IP, then check the ttl. If
	it is <= 1 then drop the packet and send and ICMP TIME EXCEEDED packet.
	This is done by calling send_time_exceeded.

	* The router looks for the longest prefix match in the routing table.
	If none is found, then it calls the send_desination_unreachable function.
	The original packet is dropped.

	* The router looks for the MAC of the destination IP of the packet. If
	it is unknown, then call send_arp_request and insert the packet in the
	pending messages multimap. Their key in the multimap is the destination IP.

	* If the best next-hop is found and its MAC is known, then the router
	simply forwards the message. It updates the ether header and IP header
	and sends the packet to the next hop, on its interface.


