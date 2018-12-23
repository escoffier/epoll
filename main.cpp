#include <stdio.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>
#include <fcntl.h>
#include <errno.h>

const int PORT = 9995;
const char * IP = "192.168.0.105";

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    int ret = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    if (ret < 0) {
        perror("set nonblocking fail.");
        return;
    }
}

const int MAX_EVENT = 100;


void process(int fd)
{
    char buffer[1024] = {0};
    int n = read(fd, buffer, 1024);
    if( n < 0)
    {
        if(errno != EWOULDBLOCK)
        {
            std::cout << "err: " << strerror(errno) << std::endl;
            return;
        }
    }
    else if( n == 0)
    {
        std::cout << "EOF on socket: " << fd << std::endl;
    }
    else
    {
        std::cout << "receive "<<n <<" bytes "<<"data: " << buffer << std::endl;
    }
}

int main()
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);

    if(inet_pton(AF_INET, IP, &addr.sin_addr) < 0)
    {
        std::cout << "inet_pton" <<std::endl;
        return 1;
    }

    int listernfd  = socket(AF_INET, SOCK_STREAM, 0);

    int resueAddr = 1;

    if (setsockopt(listernfd, SOL_SOCKET, SO_REUSEADDR, &resueAddr, sizeof(resueAddr))== -1 )
    {
        std::cout << "setsockopt error: " <<strerror( errno) <<std::endl;
        return 1;
    }

    if(bind(listernfd, (struct sockaddr*) &addr, sizeof(addr)) < 0)
    {
        std::cout << "bind error: " <<strerror( errno) <<std::endl;
        return 1;
    }

    if(listen(listernfd, 1) < 0)
    {
        std::cout << "bind error" <<std::endl;
        return 1;
    }

    struct epoll_event ev;
    struct epoll_event events[MAX_EVENT];
    int epoll_fd = epoll_create1(0);
    if(epoll_fd == -1)
    {
        std::cout << "epoll_create1 error" <<std::endl;
        return 1;
    }

    ev.events = EPOLLIN;
    ev.data.fd = listernfd;

    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listernfd, &ev) == -1)
    {
        std::cout << "epoll_ctl error" <<std::endl;
        return 1;
    }

    std::cout << "starting server" <<std::endl;

    for(;;)
    {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENT, -1);
        if(nfds == -1)
        {
            std::cout << "epoll_wait error" <<std::endl;
            return 1;
        }

        for(int i = 0 ; i < nfds; i++)
        {
            if(events[i].data.fd == listernfd)
            {
                struct sockaddr_in peeraddr;
                socklen_t slen;
                memset(&peeraddr, 0, sizeof(peeraddr));

                int connfd = accept(listernfd, (struct sockaddr*)&peeraddr, &slen);
                if(connfd == -1)
                {
                    std::cout << "accept error" <<std::endl;
                }
                set_nonblocking(connfd);

                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = connfd;

                if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connfd, &ev) == -1)
                {
                    std::cout << "connect epoll_ctl error" <<std::endl;
                    return 1;
                }
            }
            else
            {
                if(events[i].events == EPOLLIN)
                {
                    process(events[i].data.fd);
                }
                else
                {
                    std::cout << "event: " << events[i].events << std::endl;
                }

            }
        }
    }




    printf("Hello World!\n");
    return 0;
}
