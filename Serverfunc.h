#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "single-pong.h"

int Socket_creation();
void Socket_identification(int);
void Send_Reply(int, struct message *, struct sockaddr_in*);
void Receive_message(int, struct message *, struct sockaddr_in*);