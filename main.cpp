#include "include/skel.h"
#include "include/Router.h"


int main(int argc, char *argv[])
{
	packet m;
	int rc;

	init();

	// Creates and initializes a Router instance.
	Router* router = Router::get_instance();

	// Receives packets and manages them depending on the protocol type.
	while (1) {
		rc = get_packet(&m);
		DIE(rc < 0, "get_message");

		struct ether_header *eth_hdr = (struct ether_header *)m.payload;
		switch (ntohs(eth_hdr->ether_type))
		{
			case ETHERTYPE_ARP:	router->manage_arp_message(m);
								break;
			case ETHERTYPE_IP:	router->manage_ip_message(m);
								break;
			default:
					std::cout << "Unknown protocol\n";
		}
	}

	// Free allocated memory.
	delete router;

	return 0;
}
