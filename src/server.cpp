#include <unistd.h>
#include <fcntl.h>
#include <vector>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <sys/epoll.h>

#include "ThreadPool.hpp"

void make_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags|O_NONBLOCK);
}

int main() {
    ThreadPool workers;
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_add;
    server_add.sin_family = AF_INET;
    server_add.sin_port = htons(8080);
    server_add.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (sockaddr*)&server_add, sizeof(server_add));
    listen(server_fd, 1024);
    make_non_blocking(server_fd);
    std::cout << "\nServer is listning on port 8080\n";

    int epoll_fd = epoll_create1(0);
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = server_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event);

    struct epoll_event active_events[10];

    while(1) {
        int read_count = epoll_wait(epoll_fd, active_events, 10, -1);
        for(int i = 0; i<read_count; i++) {
            int active_fd = active_events[i].data.fd;

            if(active_fd == server_fd) {
                struct sockaddr_in client_add;
                socklen_t size = sizeof(client_add);
                int client_fd = accept(server_fd, (sockaddr*)&client_add, &size);
                make_non_blocking(client_fd);

                struct epoll_event client_event;
                client_event.events = EPOLLIN;
                client_event.data.fd = client_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_event);
            }

            else {
                std::vector<char> buffer(1024);
                int bytes_read = read(active_fd, buffer.data(), buffer.size());
                if(bytes_read == 0) {
                    close(active_fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, active_fd, nullptr);
                }
                else {
                    std::stringstream ss(buffer.data());
                    std::vector<std::string> tokens;
                    std::string token;
                    while(ss >> token) {
                        if(!token.empty()) tokens.push_back(token);
                    }

                    if(tokens.size()>=3) {
                        int a = stoi(tokens[1]);
                        int b = stoi(tokens[2]);
                        if(tokens[0] == "ADD") {
                            workers.add_task([=] {
                                std::string ans = to_string(a+b);
                                write(active_fd, ans.c_str(), ans.length());
                            });
                        }
                        else if(tokens[0] == "SUB") {
                            workers.add_task([=] {
                                std::string ans = to_string(a-b);
                                write(active_fd, ans.c_str(), ans.length());
                            });
                        }
                        else {
                            std::cerr << "\ninvalid operation\n";
                            return 1;
                        }
                    }
                    else {
                        std::cerr << "\nInvalid operation\n";
                        continue;
                    }
                }
            }
        }
    }
}