#include <arpa/inet.h>
#include <ifaddrs.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <linux/netfilter.h>
#include <netinet/ether.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <pcap.h>

#include <cstring>
#include <iostream>
#include <thread>

#include "csm.h"
#include "netfilter.h"
#include "p_capture.h"

int setup_iptables_rules() {
    return system("sudo iptables -I OUTPUT -j NFQUEUE --queue-bypass");
}

in_addr *get_interface_ip(char *interface) {
    ifaddrs *addrs = nullptr;
    int      code  = getifaddrs(&addrs);
    if (code != 0) {
        std::cerr << "Unable to fetch IP of " << interface << std::endl;
        return nullptr;
    }

    for (ifaddrs *curr = addrs; curr != nullptr; curr = curr->ifa_next) {
        if (curr->ifa_addr->sa_family != AF_INET) {
            continue;
        }

        // IPv4

        // Be aware that the `ifa_addr`, `ifa_netmask` and `ifa_data` fields might contain nullptr.
        // De-referencing nullptr causes "Undefined behavior" problems.
        // So it is needed to check these fields before de-referencing.
        if (curr->ifa_addr != nullptr && strcmp(curr->ifa_name, interface) == 0) {
            return &((sockaddr_in *) (curr->ifa_addr))->sin_addr;
        }
    }
    return nullptr;
}

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        std::cerr << "Usage: tcp-shank <interface>" << std::endl;
        return 1;
    }

    std::cout << "Interface: " << argv[1] << std::endl;

    in_addr *addr = get_interface_ip(argv[1]);

    if (addr == nullptr) {
        std::cerr << "Unable to get IP for interface " << argv[1] << std::endl;
        return 1;
    }
    std::cout << "IP: " << inet_ntoa(*addr) << std::endl;

    if (setup_iptables_rules() != 0) {
        std::cerr << "Unable to set iptables rule" << std::endl;
        return 1;
    }

    auto *pcap_thread = new std::thread(
            [](char *interface) -> void {
                std::cout << "Starting pcap\n";
                pcp::start_handle(interface);
            },
            argv[1]);

    //    auto *nfq_thread = new std::thread([]() -> void {
    //        std::cout << "Starting nfq\n";
    //        nfq::start_handle();
    //    });

    pcap_thread->join();
    //    nfq_thread->join();

    return 0;
}
