#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>

int Socket_creation();
void Socket_identification(int);
void Send_Reply(int, message*, struct sockaddr_in*);
void Receive_message(int, message*, struct sockaddr_in*);