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
    while (true)
    {
        ssize_t valread = read(sock, buffer, BUFFER_SIZE - 1);
        if (valread <= 0) { break; }
        //null terminate stale data
        buffer[valread] = '\0';
        std::cout << buffer << std::endl;
    }

    close(sock);
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

    //send name to server
    std::string name;
    std::cout << "Enter name (viewed by all clients): ";
    std::cin >> name;
    send(sock, name.c_str(), name.size(), 0);

    //messaging between client and server
    while (true)
    {
        std::string msg;
        std::getline(std::cin, msg);
        if (msg == "quit") { shutdown(sock, SHUT_RD); break; }
        send(sock, msg.c_str(), msg.size(), 0);
    }

    reader.join();
    return 0;
}