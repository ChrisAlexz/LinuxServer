#include <iostream>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <unistd.h>
#include "ThreadPool.h"
#include "EpollServer.h"
#include "http_handler.h"

int main() {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigprocmask(SIG_BLOCK, &mask, nullptr);

    int signal_fd = signalfd(-1, &mask, 0);

    int server_fd = create_server_socket(8080);
    int epoll_fd  = create_epoll(server_fd);

    epoll_event sig_ev{};
    sig_ev.events  = EPOLLIN;
    sig_ev.data.fd = signal_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, signal_fd, &sig_ev);

    ThreadPool pool(8);

    epoll_event events[MAX_EVENTS];

    std::cout << "Listening on port 8080 with epoll + 8 worker threads...\n";
    std::cout << "Press Ctrl+C to shut down gracefully.\n";

    bool running = true;
    while (running) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

        for (int i = 0; i < n; i++) {
            if (events[i].data.fd == signal_fd) {
                signalfd_siginfo info;
                read(signal_fd, &info, sizeof(info));
                std::cout << "\nShutting down gracefully...\n";
                running = false;
                break;
            }

            if (events[i].data.fd == server_fd) {
                int client_fd = accept(server_fd, nullptr, nullptr);
                if (client_fd < 0) continue;
                pool.enqueue([client_fd] {
                    handle_client(client_fd);
                });
            }
        }
    }

    close(signal_fd);
    close(server_fd);
    close(epoll_fd);
    return 0;
}