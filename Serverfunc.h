/*PSis Project 2 - 21/22
 * Filipe Santos - 90068
 * Alexandre Fonseca - 90210
 */
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "single-pong.h"

int Socket_creation();
void Socket_identification(int);
void Send_Reply(int, struct message *, struct sockaddr_in*);
void Receive_message(int, struct message *, struct sockaddr_in*);