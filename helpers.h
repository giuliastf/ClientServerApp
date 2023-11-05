#ifndef __HELPERS_H__

#include <stddef.h>
#include <stdint.h>

#define MSG_MAXSIZE 1024

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

typedef struct {
	char ip[20];
	int port;
	char data[1551];
  uint8_t type;
} udp_msg;

typedef struct {
	char cmd;
	char topic[51];
	int sf;
	char id_cli[10];
} tcp_msg;

typedef struct subscriber {
    char id[11];
    int fd;
    int status; // 0-offline 1-online
    udp_msg send[10];
	int no_send;
} subscriber;


typedef struct topic {
  char name[51];
  int sf[10];
  subscriber subscribers[10];
  int no_sub;
} topic;







#endif