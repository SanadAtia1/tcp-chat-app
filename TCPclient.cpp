#include <iostream>
#include <string>
#include <memory>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

constexpr int PORT = 8080;
constexpr int BUFFER_SIZE = 1024;
int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    //creating socket file descriptor
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return -1;
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    //convert IPv4 adn IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        return -1;
    }
    //connect to the server
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed" << std::endl;
        return -1;
    }
    //std::string hello = "Hello from client";
    char choice = 'y';
    while (choice != 'q')
    {
        std::string hello;
        std::getline(std::cin, hello);
        send(sock, hello.c_str(), hello.size(), 0);
        std::cout << "Hello message sent" << std::endl;
        ssize_t valread = read(sock, buffer, BUFFER_SIZE);
        std::cout << "Received: " << buffer << std::endl;
        std::cin >> choice;
    }
    //close socket
    close(sock);
    return 0;
}
//g++ TCPclient.cpp -o client