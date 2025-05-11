#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

// Helper function to connect to the server
int connect_to_server(const std::string& host, int port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }

    struct addrinfo hints, *result = nullptr;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;     // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP socket

    // Get address info
    std::string port_str = std::to_string(port);
    if (getaddrinfo(host.c_str(), port_str.c_str(), &hints, &result) != 0) {
        WSACleanup();
        throw std::runtime_error("getaddrinfo failed");
    }

    // Try each address until we connect
    SOCKET sockfd = INVALID_SOCKET;
    for (struct addrinfo* rp = result; rp != nullptr; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd == INVALID_SOCKET) {
            continue;
        }

        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != SOCKET_ERROR) {
            break; // Success
        }

        closesocket(sockfd);
        sockfd = INVALID_SOCKET;
    }

    freeaddrinfo(result);

    if (sockfd == INVALID_SOCKET) {
        WSACleanup();
        throw std::runtime_error("Could not connect to server");
    }

    return static_cast<int>(sockfd);
}

// Send a request with call type and sequence number
void send_request(int sock, uint8_t callType, uint8_t seq) {
    uint8_t buffer[2] = {callType, seq};
    ssize_t total_sent = 0;
    
    while (total_sent < 2) {
        ssize_t sent = send(sock, reinterpret_cast<const char*>(buffer + total_sent), 2 - total_sent, 0);
        if (sent == SOCKET_ERROR) {
            throw std::runtime_error("Failed to send request");
        }
        total_sent += sent;
    }
}

// Read exactly n bytes from the socket
std::vector<uint8_t> read_n(int sock, size_t n) {
    std::vector<uint8_t> buffer(n);
    size_t total_read = 0;
    
    while (total_read < n) {
        ssize_t bytes_read = recv(sock, reinterpret_cast<char*>(buffer.data() + total_read), n - total_read, 0);
        if (bytes_read <= 0) {
            if (bytes_read == 0) {
                throw std::runtime_error("Connection closed by server");
            }
            throw std::runtime_error("Failed to read from socket");
        }
        total_read += bytes_read;
    }
    
    return buffer;
}

// Packet structure for parsing responses
struct Packet {
    std::string symbol;
    char buysell;
    int32_t quantity;
    int32_t price;
    int32_t sequence;
};

// Parse a 17-byte big-endian response into a Packet
Packet parse_packet(const uint8_t* p) {
    Packet packet;
    
    // Symbol (4 bytes)
    packet.symbol = std::string(reinterpret_cast<const char*>(p), 4);
    
    // Buy/Sell (1 byte)
    packet.buysell = static_cast<char>(p[4]);
    
    // Quantity (4 bytes, big-endian)
    packet.quantity = (static_cast<int32_t>(p[5]) << 24) |
                     (static_cast<int32_t>(p[6]) << 16) |
                     (static_cast<int32_t>(p[7]) << 8) |
                     static_cast<int32_t>(p[8]);
    
    // Price (4 bytes, big-endian)
    packet.price = (static_cast<int32_t>(p[9]) << 24) |
                  (static_cast<int32_t>(p[10]) << 16) |
                  (static_cast<int32_t>(p[11]) << 8) |
                  static_cast<int32_t>(p[12]);
    
    // Sequence (4 bytes, big-endian)
    packet.sequence = (static_cast<int32_t>(p[13]) << 24) |
                     (static_cast<int32_t>(p[14]) << 16) |
                     (static_cast<int32_t>(p[15]) << 8) |
                     static_cast<int32_t>(p[16]);
    
    return packet;
}

// Stream all packets from the server
std::vector<Packet> stream_all(const std::string& host, int port) {
    std::vector<Packet> packets;
    int sock = connect_to_server(host, port);
    
    try {
        // Send initial request
        send_request(sock, 1, 0);
        
        // Read packets until server closes
        while (true) {
            try {
                auto response = read_n(sock, 17);
                packets.push_back(parse_packet(response.data()));
            } catch (const std::runtime_error& e) {
                if (std::string(e.what()) == "Connection closed by server") {
                    break;
                }
                throw;
            }
        }
    } catch (...) {
        closesocket(sock);
        WSACleanup();
        throw;
    }
    
    closesocket(sock);
    WSACleanup();
    return packets;
}

// Resend a specific sequence number
Packet resend_one(const std::string& host, int port, uint8_t seq) {
    int sock = connect_to_server(host, port);
    
    try {
        // Send resend request
        send_request(sock, 2, seq);
        
        // Read and parse the response
        auto response = read_n(sock, 17);
        Packet packet = parse_packet(response.data());
        
        closesocket(sock);
        WSACleanup();
        return packet;
    } catch (...) {
        closesocket(sock);
        WSACleanup();
        throw;
    }
}

// Write packets to JSON file
void write_packets_to_json(const std::vector<Packet>& packets, const std::string& filename) {
    std::ofstream out(filename);
    if (!out) {
        throw std::runtime_error("Could not open " + filename + " for writing");
    }

    out << "[\n";
    for (size_t i = 0; i < packets.size(); ++i) {
        const auto& p = packets[i];
        out << "  {\n"
            << "    \"symbol\": \"" << p.symbol << "\",\n"
            << "    \"buysellindicator\": \"" << p.buysell << "\",\n"
            << "    \"quantity\": " << p.quantity << ",\n"
            << "    \"price\": " << p.price << ",\n"
            << "    \"packetSequence\": " << p.sequence << "\n"
            << "  }" << (i < packets.size() - 1 ? "," : "") << "\n";
    }
    out << "]\n";
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <host> <port>" << std::endl;
        return 1;
    }

    try {
        const std::string host = argv[1];
        int port;
        try {
            port = std::stoi(argv[2]);
            if (port <= 0 || port > 65535) {
                throw std::out_of_range("Port number out of range");
            }
        } catch (const std::exception& e) {
            std::cerr << "Invalid port number: " << e.what() << std::endl;
            return 1;
        }
        
        // Stream all packets
        std::vector<Packet> packets = stream_all(host, port);
        
        // Find max sequence number
        int32_t maxSeq = 0;
        for (const auto& packet : packets) {
            maxSeq = std::max(maxSeq, packet.sequence);
        }
        
        // Track seen sequences
        std::vector<bool> seen(maxSeq + 1, false);
        for (const auto& packet : packets) {
            seen[packet.sequence] = true;
        }
        
        // Resend missing packets
        for (int32_t seq = 1; seq <= maxSeq; ++seq) {
            if (!seen[seq]) {
                try {
                    Packet missing = resend_one(host, port, seq);
                    packets.push_back(missing);
                    seen[seq] = true;
                } catch (const std::exception& e) {
                    std::cerr << "Failed to resend sequence " << seq << ": " << e.what() << std::endl;
                }
            }
        }
        
        // Sort packets by sequence number
        std::sort(packets.begin(), packets.end(), 
                 [](const Packet& a, const Packet& b) { return a.sequence < b.sequence; });
        
        // Write to JSON file
        write_packets_to_json(packets, "packets.json");
        
        // Verify no gaps in sequences
        bool has_gaps = false;
        for (int32_t seq = 1; seq <= maxSeq; ++seq) {
            if (!seen[seq]) {
                has_gaps = true;
                std::cerr << "Warning: Missing sequence " << seq << std::endl;
            }
        }
        
        std::cout << "Wrote " << packets.size() << " packets to packets.json" << std::endl;
        if (has_gaps) {
            std::cerr << "Warning: Some sequences are missing!" << std::endl;
            return 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 