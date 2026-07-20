#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cerrno>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <vector>
#include <string>

#define MAX_LIMIT 1000

void make_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    int epoll_fd = epoll_create1(0);

    struct sockaddr_in server_add;
    server_add.sin_family = AF_INET;
    server_add.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server_add.sin_addr);

    for(int i = 0; i < MAX_LIMIT; i++) {
        int client_fd = socket(AF_INET, SOCK_STREAM, 0);
        make_non_blocking(client_fd);
        
        connect(client_fd, (sockaddr*)&server_add, sizeof(server_add));
        
        struct epoll_event event;
        event.events = EPOLLIN | EPOLLOUT | EPOLLET;
        event.data.fd = client_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event);
    }
    
    std::cout << "\n1000 connection requests sent\n";

    struct epoll_event active_events[10];
    int completed_req = 0;

    while(completed_req < MAX_LIMIT) {
        int read_count = epoll_wait(epoll_fd, active_events, 10, -1);
        
        for(int i = 0; i < read_count; i++) {
            int active_fd = active_events[i].data.fd;

            if(active_events[i].events & EPOLLOUT) {
                std::string cmd = "ADD 5 11";
                write(active_fd, cmd.c_str(), cmd.length());

                struct epoll_event modified;
                modified.events = EPOLLIN | EPOLLET;
                modified.data.fd = active_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, active_fd, &modified);
            }
            
            if(active_events[i].events & EPOLLIN) {
                std::vector<char> buffer(1024, 0);
                int bytes_read = read(active_fd, buffer.data(), buffer.size());
                
                if(bytes_read > 0) {
                    completed_req++;
                    close(active_fd);
                } else if (bytes_read == 0) {
                    close(active_fd);
                }
            }
        }
    }
    
    std::cout << "\n" << completed_req << " out of 1000 requests processed successfully.\n";
    return 0;
}