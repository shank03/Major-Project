namespace pcp {

    void packet_handler(u_char *user_data, const pcap_pkthdr *pkthdr, const u_char *packet) {
        // Process only UDP packets
        ip *ip_header = (ip *) (packet + 14);    // Assuming Ethernet headers
        if (ip_header->ip_p != IPPROTO_UDP) {
            return;
        }

        // Only accept from immediate hop
        if (ip_header->ip_ttl - 1 != 0) {
            return;
        }

        auto *udp_header = (udphdr *) (packet + 14 + (ip_header->ip_hl * 4));
        if (udp_header->dest != udp_header->source || ntohs(udp_header->dest) != 16) {
            return;
        }

        std::cout << "--\n";
        std::cout << "[PCAP] source IP: " << inet_ntoa(ip_header->ip_src) << std::endl;
        std::cout << "[PCAP] dest IP: " << inet_ntoa(ip_header->ip_dst) << std::endl;
        std::cout << "[PCAP] source port: " << ntohs(udp_header->source) << std::endl;
        std::cout << "[PCAP] dest port: " << ntohs(udp_header->dest) << std::endl;

        const u_char *payload     = packet + 14 + (ip_header->ip_hl * 4) + sizeof(udphdr);
        int           payload_len = ntohs(udp_header->len) - sizeof(udphdr);

        csm::state->update_state((csm::CS) payload);
        std::cout << "[PCAP] state: s: " << csm::state->send << "; v: " << csm::state->value << "; d: " << csm::state->delay << std::endl;

        // Modify packet content (example: change the TCP port number)
        // Change destination port to 8888
        //        udp_header->th_dport = htons(8888);

        //        auto *tcp_header_edited = (tcphdr *) (packet + 14 + (ip_header->ip_hl * 4));

        // Send the modified packet or do further processing here
        // For illustration purposes, printing modified packet information
        //        std::cout << "Modified Packet: Source Port: " << ntohs(udp_header->th_sport)
        //                  << " Destination Port: " << ntohs(tcp_header_edited->th_dport) << std::endl
        //                  << "--" << std::endl;
    }

    int start_handle(char *interface) {
        pcap_t     *handle;
        char        err_buf[PCAP_ERRBUF_SIZE];
        bpf_program fp {};
        char        filter_exp[] = "udp";    // Filter expression to capture only TCP packets

        // Open the network interface for packet capture
        handle = pcap_open_live(interface, BUFSIZ, 1, 1000, err_buf);

        if (handle == nullptr) {
            std::cerr << "Couldn't open device: " << err_buf << std::endl;
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
        pcap_loop(handle, -1, packet_handler, nullptr);

        pcap_close(handle);
        return 0;
    }

}    // namespace pcp
