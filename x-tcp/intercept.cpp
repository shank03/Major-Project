#include <arpa/inet.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <linux/netfilter.h>
#include <netinet/ether.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#include <bitset>
#include <iostream>

#define MAX_PAYLOAD_SIZE 4096

std::string ip_toa(uint32_t addr) {
    struct in_addr p;
    p.s_addr = addr;
    return inet_ntoa(p);
}

static int packet_callback(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
                           struct nfq_data *nfa, void *data) {
    struct nfqnl_msg_packet_hdr *ph;
    unsigned char               *payload;
    int                          payload_len;

    ph = nfq_get_msg_packet_hdr(nfa);
    // if (ph) {
    // int id = ntohl(ph->packet_id);
    // std::cout << "Packet received with ID: " << id << std::endl;
    // }

    payload_len = nfq_get_payload(nfa, &payload);

    // Manipulate the packet payload here if needed
    // For example, modify TCP destination port
    struct iphdr *ip_header = (struct iphdr *) payload;
    if (ip_header->protocol == IPPROTO_TCP && ip_header->saddr != 16777343) {
        std::cout << "Source IP: " << ip_toa(ip_header->saddr) << std::endl;
        std::cout << "Dest IP: " << ip_toa(ip_header->daddr) << std::endl
                  << "--" << std::endl;
        struct tcphdr *tcp_header = (struct tcphdr *) (payload + (ip_header->ihl << 2));

        // Modify the destination port (example: change to 8888)
        tcp_header->dest          = htons(8888);
    }

    // Accept the modified packet
    return nfq_set_verdict(qh, ntohl(ph->packet_id), NF_ACCEPT, payload_len, payload);
}

int main() {
    struct nfq_handle   *h;
    struct nfq_q_handle *qh;
    struct nfnl_handle  *nh;
    int                  fd, rv;
    char                 buf[MAX_PAYLOAD_SIZE];

    h = nfq_open();
    if (!h) {
        std::cerr << "Error during nfq_open()" << std::endl;
        exit(1);
    }

    if (nfq_unbind_pf(h, AF_INET) < 0) {
        std::cerr << "Error during nfq_unbind_pf()" << std::endl;
        exit(1);
    }

    if (nfq_bind_pf(h, AF_INET) < 0) {
        std::cerr << "Error during nfq_bind_pf()" << std::endl;
        exit(1);
    }

    qh = nfq_create_queue(h, 0, &packet_callback, NULL);
    if (!qh) {
        std::cerr << "Error during nfq_create_queue()" << std::endl;
        exit(1);
    }

    if (nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff) < 0) {
        std::cerr << "Can't set packet_copy mode" << std::endl;
        exit(1);
    }

    fd = nfq_fd(h);

    while ((rv = recv(fd, buf, sizeof(buf), 0)) && rv >= 0) {
        nfq_handle_packet(h, buf, rv);
    }

    nfq_destroy_queue(qh);
    nfq_close(h);

    return 0;
}
