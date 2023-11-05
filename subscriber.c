
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <poll.h>
#include <netinet/tcp.h>

#include "helpers.h"

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    DIE(argc != 4, "eroare argumente\n");
    
    // obtin nr portului
    int port = atoi(argv[3]);
    DIE(port == 0, "eroare port\n");

    char id_cli[11];
    memcpy(id_cli, argv[1], strlen(argv[1]) + 1);

    struct sockaddr_in server_addr;
    memset((char *)&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    int rez;

    rez = inet_aton(argv[2], &server_addr.sin_addr); // converts the Internet host address cp from the IPv4 numbers-and-dots notation into binary form
    DIE(rez == 0, "eroare inet_aton\n");

    
    int cli_sock = socket(AF_INET, SOCK_STREAM, 0);
    DIE(cli_sock < 0, "eroare socket client\n");

    rez = connect(cli_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    DIE(rez < 0, "eroare connect client\n");
    
    int yes = 1;
    setsockopt(cli_sock, IPPROTO_TCP, TCP_NODELAY, (char *) &yes, sizeof(int));

    rez = send(cli_sock, argv[1], 11, 0); // trimit id la server
    DIE(rez < 0, "eroare send\n");

    struct pollfd pfds[2];

    pfds[0].fd = STDIN_FILENO;
    pfds[0].events = POLLIN;

    pfds[1].fd = cli_sock;
    pfds[1].events = POLLIN;

    while (1)
    {
        poll(pfds, 2, -1); //
        if ((pfds[0].revents & POLLIN) != 0)
        {
            // tastatura
            char buf[100];
            memset(buf, 0, 100);
            if (fgets(buf, sizeof(buf), stdin))
            {
                if (strcmp(buf, "exit\n") == 0)
                {
                    close(cli_sock);
                    return 0;
                }
                else
                {
                    // creez mesaj de subscribe/unsubscirbe
                    tcp_msg tcp_msg;
                    memset(&tcp_msg, 0, sizeof(tcp_msg));

                    // setare id client (argv[1])
                    memcpy(&tcp_msg.id_cli, argv[1], strlen(argv[1]));
                    char *temp = strtok(buf, " \n");

                    // setare tip subscribe-1/unsubscribe-0
                    tcp_msg.cmd = temp[0]; // s/u

                    // setare topic
                    temp = strtok(NULL, " \n");
                    memcpy(&tcp_msg.topic, temp, strlen(temp));

                    // obtin SF
                    temp = strtok(NULL, " \n");
                    tcp_msg.sf = atoi(temp); // 0 sau 1

                    rez = send(cli_sock, (char *)&tcp_msg, sizeof(tcp_msg), 0); // trimit id la server
                    DIE (rez < 0, "eroare la send\n");

                    if (tcp_msg.cmd == 's')
                    {
                        printf("Subscribed to topic.\n");
                    }
                    else if (tcp_msg.cmd == 'u')
                    {
                        printf("Unsubscribed to topic.\n");
                    }
                }
            }
        }
        else if ((pfds[1].revents & POLLIN) != 0)
        {
            // mesaj de la server, primesc tcp trimis de mine
            udp_msg udp_msg;
            memset(&udp_msg, 0, sizeof(udp_msg));
            rez = recv(cli_sock, (char *)&udp_msg, sizeof(udp_msg), 0);
            DIE(rez < 0, "eroare la recv\n");
            if (rez == 0)
            {
                close(cli_sock);
                return 0;
            }
            
            // afisare mesaj primit de la server
            char topic_msg[51];
            memcpy(topic_msg, udp_msg.data, 50);
            // printf("%s:%d - %s - ", udp_msg.ip, udp_msg.port, topic_msg);

            if (udp_msg.type == 0)
            {
                uint8_t sign = 0;
                memcpy(&sign, udp_msg.data + 51, sizeof(uint8_t));

                uint32_t integer = 0;
                memcpy(&integer, udp_msg.data + 52, sizeof(uint32_t));
                integer = ntohl(integer);
                if(sign == 1) {
                    integer *= -1;
                }

                printf("%s:%d - %s - INT - %d\n", udp_msg.ip, udp_msg.port, topic_msg, integer);
            }

            else if (udp_msg.type == 1)
            {
                // SHORT REAL
                uint16_t sh = 0;
                memcpy(&sh, udp_msg.data + 51, sizeof(uint16_t));
                sh = ntohs(sh);

                printf("%s:%d - %s - SHORT_REAL - %.2f\n", udp_msg.ip, udp_msg.port, topic_msg, sh/100.0);

            }
            else if (udp_msg.type == 2)
            {
                // FLOAT
                uint8_t sign = 0;
                memcpy(&sign, udp_msg.data + 51, sizeof(uint8_t));
                uint32_t mod = 0;
                memcpy(&mod, udp_msg.data + 52, sizeof(uint32_t));
                mod = ntohl(mod);
                uint8_t a = 0;
                memcpy(&a, udp_msg.data + 56, sizeof(uint8_t));
                float rez = (float) mod;
                
                for(uint8_t i = 0; i < a; i++)
                    rez /= 10.0;
                if(sign == 1) {
                    rez *= -1;
                }
                printf("%s:%d - %s - FLOAT - %.*f\n", udp_msg.ip, udp_msg.port, topic_msg, a, rez);

            }
            else if (udp_msg.type == 3)
            {
                // STRING
                printf("%s:%d - %s - STRING - %s\n", udp_msg.ip, udp_msg.port, topic_msg, udp_msg.data + 51);
            }
        }
    }
    return 0;
}