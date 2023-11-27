#include <netinet/ether.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <pcap.h>

#include <iostream>

std::string eth_toa(unsigned char mac[6]) {
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0],
             mac[1],
             mac[2],
             mac[3],
             mac[4],
             mac[5]);
    return std::string(mac_str);
}

void packet_handler(u_char *user_data, const struct pcap_pkthdr *pkthdr, const u_char *packet) {
    struct ethhdr *ethernet_header = (struct ethhdr *) (packet);
    std::string    src_eth         = eth_toa(ethernet_header->h_source);
    std::string    dest_eth        = eth_toa(ethernet_header->h_dest);
    std::cout << "Source Mac: " << src_eth << std::endl;
    std::cout << "Dest Mac: " << dest_eth << std::endl;

    // Process only TCP packets
    struct ip *ip_header = (struct ip *) (packet + 14);    // Assuming Ethernet headers
    if (ip_header->ip_p != IPPROTO_TCP) {
        return;
    }

    std::string src_ip_addr  = std::string(inet_ntoa(ip_header->ip_src));
    std::string dest_ip_addr = std::string(inet_ntoa(ip_header->ip_dst));
    std::cout << "Source IP: " << src_ip_addr << std::endl;
    std::cout << "Dest IP: " << dest_ip_addr << std::endl;

    if (dest_ip_addr == "142.250.66.4" || dest_ip_addr == "142.250.183.100") {
        packet = NULL;
        std::cout << "Google dropped\n";
        return;
    }

    if (src_ip_addr == "142.250.66.4" || src_ip_addr == "142.250.183.100") {
        packet = NULL;
        std::cout << "Google response dropped\n";
        return;
    }

    struct tcphdr *tcp_header = (struct tcphdr *) (packet + 14 + (ip_header->ip_hl * 4));

    // Modify packet content (example: change the TCP port number)
    // Change destination port to 8888
    tcp_header->th_dport = htons(8888);

    struct tcphdr *tcp_header_edited = (struct tcphdr *) (packet + 14 + (ip_header->ip_hl * 4));

    // Send the modified packet or do further processing here
    // For illustration purposes, printing modified packet information
    std::cout << "Modified Packet: Source Port: " << ntohs(tcp_header->th_sport)
              << " Destination Port: " << ntohs(tcp_header_edited->th_dport) << std::endl
              << "--" << std::endl;
}

int main() {
    pcap_t            *handle;
    char               errbuf[PCAP_ERRBUF_SIZE];
    struct bpf_program fp;
    char               filter_exp[] = "tcp";    // Filter expression to capture only TCP packets

    // Open the network interface for packet capture
    handle = pcap_open_live("eth0", BUFSIZ, 1, 1000, errbuf);    // Change "eth0" to your interface

    if (handle == NULL) {
        std::cerr << "Couldn't open device: " << errbuf << std::endl;
        return -1;
    }

    // Compile the filter expression
    if (pcap_compile(handle, &fp, filter_exp, 0, PCAP_NETMASK_UNKNOWN) == -1) {
        std::cerr << "Couldn't parse filter " << pcap_geterr(handle) << std::endl;
        return -1;
    }

    pcap_set_immediate_mode(handle, 1);

    // Set the filter
    if (pcap_setfilter(handle, &fp) == -1) {
        std::cerr << "Couldn't install filter " << pcap_geterr(handle) << std::endl;
        return -1;
    }

    // Start capturing packets
    pcap_loop(handle, -1, packet_handler, NULL);

    pcap_close(handle);
    return 0;
}
