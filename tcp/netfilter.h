#include <libnetfilter_queue/libnetfilter_queue.h>
#include <linux/netfilter.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#include <atomic>

#define MAX_PAYLOAD_SIZE 4096

namespace nfq {

    static std::atomic<bool>  udp_req_called      = false;
    static std::atomic<short> packets_transferred = 0;
    static std::atomic<short> packet_drops        = 0;

    static in_addr *source_ip   = nullptr;
    static char    *next_hop_ip = nullptr;

    static int packet_callback(nfq_q_handle *qh, nfgenmsg *nfmsg, nfq_data *nfad, void *data) {
        nfqnl_msg_packet_hdr *ph;
        uint32_t              packet_id;
        unsigned char        *packet;
        int                   packet_len;

        ph         = nfq_get_msg_packet_hdr(nfad);
        packet_id  = ntohl(ph->packet_id);
        packet_len = nfq_get_payload(nfad, &packet);

        // Manipulate the packet here if needed
        // For example, modify TCP destination port
        auto *ip_header = (ip *) packet;
        if (ip_header->ip_src.s_addr == 16777343 || ip_header->ip_dst.s_addr == 16777343) {
            // Skip localhost
            return nfq_set_verdict(qh, packet_id, NF_ACCEPT, 0, nullptr);
        }

        if (ip_header->ip_p == IPPROTO_UDP) {
            if (ip_header->ip_src.s_addr == source_ip->s_addr) {
                return nfq_set_verdict(qh, packet_id, NF_ACCEPT, 0, nullptr);
            }

            // Capture only those with immediate hop
            if (ip_header->ip_ttl - 1 != 0) {
                return nfq_set_verdict(qh, packet_id, NF_ACCEPT, 0, nullptr);
            }

            auto *udp_header = (udphdr *) (packet + (ip_header->ip_hl << 2));
            if (ntohs(udp_header->dest) != CONGESTION_STATE_PORT) {
                return nfq_set_verdict(qh, packet_id, NF_ACCEPT, 0, nullptr);
            }

            std::cout << "[NFQ] [UDP] CS response\n";

            std::cout << "--\n";
            std::cout << "[NFQ] [UDP] source IP: " << inet_ntoa(ip_header->ip_src) << std::endl;
            std::cout << "source port: " << ntohs(udp_header->source) << std::endl;
            std::cout << "dest port: " << ntohs(udp_header->dest) << std::endl;

            const u_char *payload = packet + (ip_header->ip_hl << 2) + sizeof(udphdr);
            //            int           payload_len = htons(udp_header->len) - sizeof(udphdr);

            csm::state->update_state((csm::CS) payload);
            csm::renew_ttl();

            std::cout << "[NFQ] [CS] state: s: " << (csm::state->send ? "all" : "limited") << "; v: " << csm::state->value << "; d: " << csm::state->delay << std::endl;
        }

        if (ip_header->ip_p == IPPROTO_TCP) {
            if (ip_header->ip_src.s_addr == source_ip->s_addr) {
                return nfq_set_verdict(qh, packet_id, NF_ACCEPT, 0, nullptr);
            }

            if (csm::state_ttl_expired()) {
                if (packet_drops == MAX_CS_REQ_DROP) {
                    packet_drops   = 0;
                    udp_req_called = false;
                }

                if (udp_req_called) {
                    packet_drops++;
                    // Do not let any packet pass if we don't have congestion state
                    return nfq_set_verdict(qh, packet_id, NF_DROP, 0, nullptr);
                }

                std::cout << "[NFQ] [CS] TTL expired. Renewing" << std::endl;
                udp_req_called = csm::cs_req_udp(next_hop_ip) == 0;
                return nfq_set_verdict(qh, packet_id, NF_DROP, 0, nullptr);
            }
            udp_req_called = false;
            packet_drops   = 0;

            if (!csm::state->send) {
                if (packets_transferred == csm::state->value) {
                    if (csm::state->delay > 0) {
                        std::cout << "[NFQ] [CS] Delaying for " << csm::state->delay << "us" << std::endl;
                        std::this_thread::sleep_for(std::chrono::microseconds(csm::state->delay));
                    }
                    packets_transferred = 0;
                }
                packets_transferred++;
            }
        }

        // Accept the modified packet
        // return nfq_set_verdict(qh, ntohl(ph->packet_id), NF_ACCEPT, payload_len, payload);
        return nfq_set_verdict(qh, packet_id, NF_ACCEPT, 0, nullptr);
    }

    int start_handle(in_addr *src, char *nh_ip) {
        source_ip   = src;
        next_hop_ip = nh_ip;

        nfq_handle   *handle;
        nfq_q_handle *queue_handle;

        int  fd, rv;
        char buf[MAX_PAYLOAD_SIZE];

        handle = nfq_open();
        if (!handle) {
            std::cerr << "Error during nfq_open()" << std::endl;
            return 1;
        }

        if (nfq_unbind_pf(handle, AF_INET) < 0) {
            std::cerr << "Error during nfq_bind_pf()" << std::endl;
            return 1;
        }

        queue_handle = nfq_create_queue(handle, 0, &packet_callback, nullptr);
        if (!queue_handle) {
            std::cerr << "Error during nfq_create_queue()" << std::endl;
            return 1;
        }

        if (nfq_set_mode(queue_handle, NFQNL_COPY_PACKET, 0xFFFF) < 0) {
            std::cerr << "Can't set packet_copy mode" << std::endl;
            return 1;
        }

        fd = nfq_fd(handle);
        while ((rv = recv(fd, buf, sizeof(buf), 0)) && rv >= 0) {
            nfq_handle_packet(handle, buf, rv);
        }

        nfq_destroy_queue(queue_handle);
        nfq_close(handle);

        return 0;
    }
}    // namespace nfq
