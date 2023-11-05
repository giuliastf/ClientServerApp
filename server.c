#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <limits.h>
#include <sys/poll.h>

#include "helpers.h"
int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    DIE(argc < 2, "prea putine argumente, lipseste port\n");

    // obtin nr portului
    int port = atoi(argv[1]);
    DIE(port == 0, "nu s-a putut obtine nr port");

    // setare serv_addr pentru a fi receptiv pe portul port
    struct sockaddr_in server_addr;
    memset((char *)&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // TCP
    int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    DIE(tcp_socket < 0, "eroare socket tcp\n");

    int tcp_bind = bind(tcp_socket, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));
    DIE(tcp_bind < 0, "eroare bind tcp\n");

    int tcp_listen = listen(tcp_socket, 100);
    DIE(tcp_listen < 0, "eroare listen tcp\n");

    // udp_addr
    struct sockaddr_in udp_addr;
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = INADDR_ANY;
    udp_addr.sin_port = htons(port);

    // UDP
    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(udp_socket < 0, "eroare socket udp\n");

    int udp_bind = bind(udp_socket, (struct sockaddr *)&udp_addr, sizeof(struct sockaddr));
    DIE(udp_bind < 0, "eroare bind udp\n");

    // multiplexare cu poll (lab 7/43)
    struct pollfd pfds[10];
    memset(pfds, 0, 10 * sizeof(struct pollfd));
    int nfds = 0;

    pfds[nfds].fd = STDIN_FILENO;
    pfds[nfds].events = POLLIN;
    nfds++; // = 1

    pfds[nfds].fd = tcp_socket;
    pfds[nfds].events = POLLIN;
    nfds++; // = 2

    pfds[nfds].fd = udp_socket;
    pfds[nfds].events = POLLIN;
    nfds++; // = 3

    int num_clients = 0; // = 0
    subscriber subscribers[10];
    int num_topics = 0;
    topic topics[50];

    // server loop
    while (1)
    {
        poll(pfds, nfds, -1);
        for (int index = 0; index < nfds; index++)
        {
            if (!(pfds[index].revents & POLLIN))
                continue;
            if (index == 0)
            {
                // tastatura => doar EXIT
                char text[100];
                scanf("%s", text);
                if (strncmp(text, "exit", 4) == 0)
                {
                    for (int i = 0; i < nfds; i++)
                        close(pfds[i].fd);
                    return 0;
                }
            }
            else if (index == 1)
            {
                // TCP
                struct sockaddr_in cli_addr;
                socklen_t cli_len = sizeof(cli_addr);
                memset(&cli_addr, 0, sizeof(cli_addr));

                int newsockfd = accept(tcp_socket, (struct sockaddr *)&cli_addr, &cli_len);
                DIE(newsockfd < 0, "eroare la accept\n");

                int yes = 1;
                setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, (char *) &yes, sizeof(int));

                pfds[nfds].fd = newsockfd;
                pfds[nfds].events = POLLIN;
                nfds++;

                char id_cli[11];
                recv(newsockfd, id_cli, 11, 0);

                int exists = 0;
                for (int i = 0; i < num_clients && exists != 1; i++)
                {
                    if (strcmp(subscribers[i].id, id_cli) == 0)
                    {
                        // exista client => trebuie reactivat
                        if (subscribers[i].fd == -1)
                        {
                            subscribers[i].fd = newsockfd;
                            exists = 2; // reconectat
                            printf("New client %s connected from %s:%d\n", id_cli, inet_ntoa(cli_addr.sin_addr), port);
                        }
                        if (subscribers[i].status == 0)
                        {
                            subscribers[i].status = 1;
                            // trimit mesajele stocate
                            for (int j = 0; j < subscribers[i].no_send; j++) {
                                send(newsockfd, &subscribers[i].send[j], sizeof(udp_msg), 0);
                            }
                            subscribers[i].no_send = 0;
                        }
                        else
                            exists = 1;
                    }
                }
                if (exists == 0)
                {
                    // client nou
                    subscriber s;
                    memset(&s, 0, sizeof(s));
                    memcpy(s.id, id_cli, strlen(id_cli));
                    s.fd = newsockfd;
                    s.status = 1;
                    s.no_send = 0;
                    memcpy(subscribers + num_clients, &s, sizeof(subscriber));
                    num_clients++;
                    printf("New client %s connected from %s:%d\n", id_cli, inet_ntoa(cli_addr.sin_addr), port);
                }
                else if (exists == 1)
                {
                    printf("Client %s already connected.\n", id_cli);
                    close(newsockfd);
                }
                else if (exists == 2)
                {
                    //???
                }
            }
            else if (index == 2)
            {
                // UDP
                char buf[1551];
                memset(buf, 0, 1551);
                socklen_t len = sizeof(udp_addr);
                int rez = recvfrom(udp_socket, buf, sizeof(buf), MSG_WAITALL, (struct sockaddr *)&udp_addr, &len);
                DIE(rez <= 0, "eroare la recvfrom udp\n");

                // construiesc mesajul udp
                udp_msg udp_msg;
                memset(&udp_msg, 0, sizeof(udp_msg));
                memcpy(udp_msg.data, buf, sizeof(buf));
                memcpy(udp_msg.ip, inet_ntoa(udp_addr.sin_addr), 20);
                udp_msg.port = ntohs(udp_addr.sin_port);

                // iau topic
                char topic[51];
                memcpy(topic, udp_msg.data, 50);
                udp_msg.type = buf[50];

                for (int i = 0; i < num_topics; i++)
                {
                    if (strcmp(topics[i].name, topic) == 0)
                    {
                        for (int j = 0; j < topics[i].no_sub; j++)
                        {
                            if (subscribers[j].status) {
                                send(subscribers[j].fd, (char *)&udp_msg, sizeof(udp_msg), 0);
                            } else {
                                if (topics[i].sf[j]) {
                                    subscribers[j].send[subscribers[j].no_send] = udp_msg;
                                    subscribers[j].no_send++;
                                }
                            }
                        }
                        break;
                    }
                    else
                    {
                         // sf = 1;
                    }
                }
            }
            else
            {

                tcp_msg tcp_msg;
                memset(&tcp_msg, 0, sizeof(tcp_msg));
                int rc = recv(pfds[index].fd, (char *)&tcp_msg, sizeof(tcp_msg), 0);
                if (rc == 0)
                {
                    for (int j = 0; j < num_clients; j++)
                        if (subscribers[j].fd == pfds[index].fd)
                        {
                            subscribers[j].status = 0;
                            printf("Client %s disconnected.\n", subscribers[j].id);
                            subscribers[j].fd = -1;
                        }
                    close(pfds[index].fd);
                    for (int j = index; j < nfds; j++)
                    {
                        pfds[j] = pfds[j + 1];
                    }
                    nfds--;
                    continue;
                }

                if (tcp_msg.cmd == 's')
                {
                    int topic_exists = 0;
                    for (int j = 0; j < num_topics; j++)
                    {
                        if (strcmp(tcp_msg.topic, topics[j].name) == 0)
                        {
                            topic_exists = 1;
                            memcpy(&topics[j].subscribers[topics[j].no_sub], &subscribers[index - 2], sizeof(subscriber));
                            topics[j].sf[topics[j].no_sub] = tcp_msg.sf;
                            topics[j].no_sub++;
                            break;
                        }
                    }
                    if (topic_exists == 0)
                    {
                        // adaug topic
                        topic new_top;
                        memset(&new_top, 0, sizeof(new_top));
                        new_top.no_sub = 1;

                        memcpy(&new_top.name, tcp_msg.topic, 51);
                        memcpy(&new_top.subscribers[0], &subscribers[index - 2], sizeof(subscriber));
                        new_top.sf[0] = tcp_msg.sf;
                        topics[num_topics] = new_top;
                        num_topics++;
                    }
                }
                else if (tcp_msg.cmd == 'u')
                {
                    topic new_top;
                    memcpy(new_top.name, tcp_msg.topic, strlen(tcp_msg.topic));

                    for (int i = 0; i < num_topics; i++)
                    {
                        if (strcmp(tcp_msg.topic, topics[i].name) == 0)
                        {
                            for (int j = i; j < topics[i].no_sub; j++)
                                topics[i].subscribers[j] = topics[i].subscribers[j + 1];
                            topics[i].no_sub--;
                            break;
                        }
                    }
                }
            }
        }
    }
    return 0;
}
