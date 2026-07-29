#include <iostream>
#include <string>
#include <memory>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <unordered_map>
#include <mutex>
#include <algorithm>

constexpr int PORT = 8080;
constexpr int BUFFER_SIZE = 1024;
std::unordered_map<int, std::string> sockMap;
std::mutex threadSafety;

void msgThread(int new_socket)
{
    std::string name;
    

    char buffer[BUFFER_SIZE] = {0};
    while (true)
    {
        ssize_t valread = read(new_socket, buffer, BUFFER_SIZE - 1);
        //avoid stale data leftover
        buffer[valread] = '\0';
        //break upon client disconnect
        if (valread <= 0) { break; }

        //send to all connected clients 
        std::lock_guard<std::mutex> lock(threadSafety);
        for(const auto& pair : sockMap) 
        {
            //skip the client currently sending
            if (pair.first == new_socket) { continue; }
            send(pair.first, buffer, valread, 0);
        }  
    }

    std::lock_guard<std::mutex> lock(threadSafety);
    sockMap.erase(new_socket);
    close(new_socket);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    
    //creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    //forcefully attaching socket to the port 8080 [MAC]
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    //bind the socket to the network address and port
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    //start listening for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    std::cout << "Server listening on port " << PORT << std::endl;

    //accept incoming connection
    while (true)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen)) < 0) 
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        //lock mutex to protect concurrency
        std::lock_guard<std::mutex> lock(threadSafety);
        // //add socket to client list
        sockMap[new_socket] = "";
        // sockets.push_back(new_socket);
        //create thread to begin messaging
        std::thread messenger(msgThread, new_socket);
        //detach thread, continue looping for clients
        messenger.detach();
        //std::cout << "Clients connectd: " << threads.size() << std::endl;
    }

    close(server_fd);
    return 0;
}