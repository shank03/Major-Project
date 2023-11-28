#include <unistd.h>

#define MAX_CS_REQ_DROP       3    // num of packet drops to wait until considering timeout
#define CONGESTION_STATE_PORT 16
#define CONGESTION_STATE_TTL  (10 * 1000)    // 10s

/* congestion state manager */
namespace csm {

    typedef struct congestion_state {
        bool           send;     // all [true] / limited [false]
        unsigned short value;    // 0/num of pkt
        unsigned short delay;    // (limited) duration in micro

        void update_state(congestion_state *state) {
            send  = state->send;
            value = state->value;
            delay = state->delay;
        };
    } *CS;

    static CS state     = new congestion_state { true, 0, 0 };
    uint64_t  state_ttl = 0;

    uint64_t get_current_millis() {
        auto time     = std::chrono::system_clock::now();
        auto duration = time.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    }

    bool state_ttl_expired() {
        return state_ttl < get_current_millis();
    }

    void renew_ttl() {
        state_ttl = get_current_millis() + CONGESTION_STATE_TTL;
    }

    int cs_req_udp(char *dest_ip) {
        char server_ip[] = "255.255.255.254";
        if (dest_ip != nullptr) {
            strcpy(server_ip, dest_ip);
        }

        int         udp_socket;
        sockaddr_in server_addr {}, source_addr {};
        char        buffer[1024];
        int         ttl = 2;

        // Create UDP socket
        udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
        if (udp_socket < 0) {
            std::cerr << "[CSM] Error creating socket" << std::endl;
            return 1;
        }

        // Set source address and port
        memset((char *) &source_addr, 0, sizeof(source_addr));
        source_addr.sin_family      = AF_INET;
        source_addr.sin_port        = htons(CONGESTION_STATE_PORT);
        source_addr.sin_addr.s_addr = INADDR_ANY;

        // Bind the socket to the source address
        if (bind(udp_socket, (sockaddr *) &source_addr, sizeof(source_addr)) < 0) {
            std::cerr << "[CSM] Error binding socket to the source address" << std::endl;
            close(udp_socket);
            return 1;
        }

        // Set TTL value
        if (setsockopt(udp_socket, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0) {
            std::cerr << "[CSM] Error setting TTL" << std::endl;
            close(udp_socket);
            return 1;
        }

        // Server address and port
        memset((char *) &server_addr, 0, sizeof(server_addr));
        server_addr.sin_family      = AF_INET;
        server_addr.sin_port        = htons(CONGESTION_STATE_PORT);
        server_addr.sin_addr.s_addr = inet_addr(server_ip);

        // Message to send
        const char *message = "Requested Congestion State";
        strcpy(buffer, message);

        // Send message
        if (sendto(udp_socket, buffer, strlen(buffer), 0, (sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "[CSM] Error sending message" << std::endl;
            close(udp_socket);
            return 1;
        }

        std::cout << "[CSM] [" << server_ip << "] Message sent" << std::endl;

        // Close socket
        close(udp_socket);

        return 0;
    }

}    // namespace csm
