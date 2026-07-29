#include <iostream>
#include <string>
#include <memory>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>

constexpr int PORT = 8080;
constexpr int BUFFER_SIZE = 1024;

void readThread (int sock)
{
    char buffer[BUFFER_SIZE] = {0};

    while (sock != -1)
    {
        //read incoming messages from server
        ssize_t valread = read(sock, buffer, BUFFER_SIZE - 1);
        std::cout << "read msg..." << std::endl;
        //null terminate unless data
        buffer[valread] = '\0';
        std::cout << "from server: " << buffer << std::endl;
    }
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;

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

    std::thread reader(readThread, sock);

    //messaging between client and server
    while (true)
    {
        std::string msg;
        std::getline(std::cin, msg);
        if (msg == "quit") { std::cout << "quitting..." << std::endl; sock = -1;; break; }
        send(sock, msg.c_str(), msg.size(), 0);
    }

std::cout << "about to detach socket..." << std::endl;
    reader.detach();
    std::cout << "detached socket..." << std::endl;

    //close socket
    //close(sock);
    close(sock);
    std::cout << "closed socket..." << std::endl;
    std::cout << "goodbye..." << std::endl;
    return 0;
}
//g++ TCPclient.cpp -o client