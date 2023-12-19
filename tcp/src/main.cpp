#include <arpa/inet.h>
#include <ifaddrs.h>

#include <csignal>
#include <cstring>
#include <iostream>
#include <thread>

#include "csm.hpp"
#include "tcp.hpp"

void interrupt_handle(int) {
    std::cout << " -- Signal Caught -- " << std::endl;
    tcp_nfq::Handler::stop();
}

int setup_iptables_rules() {
    return system("sudo iptables -I OUTPUT -j NFQUEUE --queue-bypass && sudo iptables -I INPUT -j NFQUEUE --queue-bypass");
}

in_addr *get_interface_ip(char *interface) {
    ifaddrs *addrs = nullptr;
    int      code  = getifaddrs(&addrs);
    if (code != 0) {
        std::cerr << "[Main] Unable to fetch interfaces" << std::endl;
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
        std::cerr << "Usage: tcp_nfq-shank <interface> <next-hop ip (optional)>" << std::endl;
        return 1;
    }

    std::cout << "[Main] Interface: " << argv[1] << std::endl;

    in_addr *addr = get_interface_ip(argv[1]);
    if (addr == nullptr) {
        std::cerr << "[Main] Unable to get IP for interface " << argv[1] << std::endl;
        return 1;
    }
    std::cout << "[Main] IP: " << inet_ntoa(*addr) << std::endl;

    if (setup_iptables_rules() != 0) {
        std::cerr << "[Main] Unable to set iptables rule" << std::endl;
        return 1;
    }

    char *nh_ip = argc >= 3 ? argv[2] : nullptr;
    if (nh_ip != nullptr) {
        std::cout << "[Main] Next-Hop IP: " << nh_ip << std::endl;
    }

    signal(SIGINT, interrupt_handle);

    std::thread([&addr, &nh_ip]() -> void { tcp_nfq::Handler::start(addr, nh_ip); }).join();
    std::cout << "Terminated\n";

    return 0;
}
