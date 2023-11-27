#define MAX_PAYLOAD_SIZE 4096

namespace nfq {

    static int packet_callback(nfq_q_handle *qh, nfgenmsg *nfmsg,
                               nfq_data *nfad, void *data) {
        nfqnl_msg_packet_hdr *ph;
        unsigned char        *payload;
        int                   payload_len;

        ph          = nfq_get_msg_packet_hdr(nfad);
        payload_len = nfq_get_payload(nfad, &payload);

        // Manipulate the packet payload here if needed
        // For example, modify TCP destination port
        auto *ip_header = (ip *) payload;
        if (ip_header->ip_p == IPPROTO_TCP && ip_header->ip_src.s_addr != 16777343) {
            std::cout << "Source IP: " << inet_ntoa(ip_header->ip_src) << std::endl;
            std::cout << "Dest IP: " << inet_ntoa(ip_header->ip_dst) << std::endl
                      << "--" << std::endl;

            // auto *tcp_header = (tcphdr *) (payload + (ip_header->ihl << 2));

            // Modify the destination port (example: change to 8888)
            // tcp_header->dest          = htons(8888);
        }

        // Accept the modified packet
        return nfq_set_verdict(qh, ntohl(ph->packet_id), NF_ACCEPT, payload_len, payload);
    }

    int start_handle() {
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
