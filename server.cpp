#include <iostream>
#include <cstring>
#include <csignal>
#include <cerrno>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <vector>

using namespace std;

constexpr int PORT = 12345;
constexpr int BACKLOG = 5;
constexpr int BUF_SIZE = 1024;

volatile sig_atomic_t wasSigHup = 0;

void sigHupHandler(int signo) {
    wasSigHup = 1;
}

int main() {
    int server_fd = -1, client_fd = -1;
    sockaddr_in server_addr{};
    fd_set read_fds;
    sigset_t blocked_mask, orig_mask;
    struct sigaction sa;

    // Настройка обработчика сигнала SIGHUP
    sa.sa_handler = sigHupHandler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGHUP, &sa, nullptr) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    // Блокировка сигнала SIGHUP
    sigemptyset(&blocked_mask);
    sigaddset(&blocked_mask, SIGHUP);
    if (sigprocmask(SIG_BLOCK, &blocked_mask, &orig_mask) == -1) {
        perror("sigprocmask");
        return EXIT_FAILURE;
    }

    // Создание серверного сокета
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        return EXIT_FAILURE;
    }

    if (listen(server_fd, BACKLOG) == -1) {
        perror("listen");
        close(server_fd);
        return EXIT_FAILURE;
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    while (true) {
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        int max_fd = server_fd;

        if (client_fd != -1) {
            FD_SET(client_fd, &read_fds);
            if (client_fd > max_fd) {
                max_fd = client_fd;
            }
        }

        int ready = pselect(max_fd + 1, &read_fds, nullptr, nullptr, nullptr, &orig_mask);

        if (ready == -1) {
            if (errno == EINTR) {
                if (wasSigHup) {
                    cout << "Received SIGHUP" << endl;
                    wasSigHup = 0;
                }
                continue;
            } else {
                perror("pselect");
                break;
            }
        }

        if (FD_ISSET(server_fd, &read_fds)) {
            sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int new_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &addr_len);

            if (new_fd == -1) {
                perror("accept");
                continue;
            }

            std::cout << "New connection from " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << std::endl;

            if (client_fd == -1) {
                client_fd = new_fd;
            } else {
                close(new_fd);
                cout << "Connection closed" << endl;
            }
        }

        if (client_fd != -1 && FD_ISSET(client_fd, &read_fds)) {
            char buffer[BUF_SIZE];
            ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer));

            if (bytes_read > 0) {
                cout << "Received " << bytes_read << " bytes from client" << endl;
            } else {
                cout << "Client disconnected" << endl;
                close(client_fd);
                client_fd = -1;
            }
        }
    }

    close(server_fd);
    if (client_fd != -1) {
        close(client_fd);
    }

    return 0;
}
