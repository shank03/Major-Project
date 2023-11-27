#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

struct congestion_state {
    bool           send;
    unsigned short value;
    unsigned short delay;

    void convert(char buff[6]) {
        buff[0] = send;
        buff[1] = 1; /* non-zero padding */
        buff[2] = value & 0xFF;
        buff[3] = (value >> 8) & 0xFF;
        buff[4] = delay & 0xFF;
        buff[5] = (delay >> 8) & 0xFF;
    }
};

int main(int argc, char *argv[]) {
    if (argc <= 3) {
        std::cerr << "Usage: udp <send> <value> <delay>" << std::endl;
        return 1;
    }

    int                udpSocket;
    struct sockaddr_in serverAddr, sourceAddr;
    char               buffer[1024];
    int                ttlValue = 1;

    // Create UDP socket
    udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return 1;
    }

    // Set source address and port
    memset((char *) &sourceAddr, 0, sizeof(sourceAddr));
    sourceAddr.sin_family      = AF_INET;
    sourceAddr.sin_port        = htons(16);     // Source port 16
    sourceAddr.sin_addr.s_addr = INADDR_ANY;    // Use any available local IP address

    // Bind the socket to the source address
    if (bind(udpSocket, (struct sockaddr *) &sourceAddr, sizeof(sourceAddr)) < 0) {
        std::cerr << "Error binding socket to the source address" << std::endl;
        close(udpSocket);
        return 1;
    }

    // Set TTL value
    if (setsockopt(udpSocket, IPPROTO_IP, IP_TTL, &ttlValue, sizeof(ttlValue)) < 0) {
        std::cerr << "Error setting TTL" << std::endl;
        close(udpSocket);
        return 1;
    }

    // Server address and port
    memset((char *) &serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family      = AF_INET;
    serverAddr.sin_port        = htons(16);                     // Replace with the desired port number
    serverAddr.sin_addr.s_addr = inet_addr("192.168.240.1");    // Replace with the destination IP address

    // Message to send
    congestion_state state {
        strcmp(argv[1], "1") == 0,
        (unsigned short) (atoi(argv[2]) & 0xFFFF),
        (unsigned short) (atoi(argv[3]) & 0xFFFF)
    };
    state.convert(buffer);

    // Send message
    if (sendto(udpSocket, buffer, 6, 0, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error sending message" << std::endl;
        close(udpSocket);
        return 1;
    }

    std::cout << "Message sent successfully" << std::endl;

    // Close socket
    close(udpSocket);

    return 0;
}
