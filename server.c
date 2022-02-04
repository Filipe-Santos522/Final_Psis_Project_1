/*PSis Project 2 - 21/22
 * Filipe Santos - 90068
 * Alexandre Fonseca - 90210
 */
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include "Serverfunc.h"
#include "sock_dg_inet.h"
#define MAX_NUMBER_OF_PLAYERS 10

pthread_mutex_t send_message_mutex;
int Num_players=0;
int Curr_Player=0;
int sock_fd;
struct sockaddr_in client_addr;
message m;
int i;

//Thread function that changes the current player every 10 seconds
void * updateBallThread(void * arg){
    struct sockaddr_in *Players = (struct sockaddr_in *) arg;
    while(1){
        sleep(10);
        if (pthread_mutex_lock(&send_message_mutex) != 0){
            perror("lock");
            exit(-1);
        }
        m.type = 2;
        if(Num_players!=0)
            Send_Reply(sock_fd, &m, &Players[Curr_Player]); //Send release_ball to current player
        m.type = 3;
        if(Num_players==1){
            Send_Reply(sock_fd, &m, &Players[0]);
        }else if(Num_players>1){
            i = rand()%(Num_players-1); //Choose a new player different from the previous one
            if(i>=Curr_Player){
                    i++;
            }
            Curr_Player=i;
            Send_Reply(sock_fd, &m, &Players[Curr_Player]); //Send message to client to change to play state
            printf("changed current player to %d\n",Curr_Player);
            for (int p = 0; p < Num_players; p++){
                m.type = 4;
                if (p != i)
                    Send_Reply(sock_fd, &m, &Players[p]);
            }
        }
        if (pthread_mutex_unlock(&send_message_mutex) != 0){
            perror("unlock");
            exit(-1);
        }
    }
}

int main(int argc, char* argv[]){
    struct sockaddr_in *Players = malloc(MAX_NUMBER_OF_PLAYERS* sizeof(client_addr));
    sock_fd = Socket_creation();
    Socket_identification(sock_fd);
    if (pthread_mutex_init(&send_message_mutex, NULL) != 0){
        perror("mutex");
        exit(-1);
    }

    pthread_t tid;
    pthread_create(&tid, NULL, updateBallThread, Players); //Create thread
    while (1){
        Receive_message(sock_fd, &m, &client_addr);
        if (pthread_mutex_lock(&send_message_mutex) != 0){
            perror("lock");
            exit(-1);
        }

        switch (m.type)
        {
        case 1:
            /* add client to list */
            Players[Num_players]=client_addr;
            Num_players++;
            if(Num_players==1){
                m.type=3;
                place_ball_random(&m.ball);
                Send_Reply(sock_fd, &m, &Players[0]);
                Curr_Player=0;
            }else{
                m.type=6;
                Send_Reply(sock_fd, &m, &Players[Num_players-1]);
            }

            break;
        
        case 2:
            /* release ball - send ball to random client */

            m.type=3;
            if(Num_players==1){
                Send_Reply(sock_fd, &m, &Players[0]);
            }else if(Num_players>1){
                i= rand()%(Num_players-1);
                if(i>=Curr_Player){
                    i++;
                }
                Curr_Player=i;
                Send_Reply(sock_fd, &m, &Players[i]);
            }else{
                perror("error\n");
                exit(1);
            }
            break;
        case 4:
            /* move ball - update ball on screen */
            m.type=4;
            for(int j=0; j<Num_players; j++){
                if(j!=Curr_Player)
                    Send_Reply(sock_fd, &m, &Players[j]);
            }
            break;
        case 5:
            /* disconnect - remove user from list */
            Players[Curr_Player]=Players[Num_players-1];
            Num_players--;
            m.type=3;
            if(Num_players==1){
                Send_Reply(sock_fd, &m, &Players[0]);
            }else if(Num_players>1){
                i= rand()%(Num_players-1);
                Curr_Player=i;
                Send_Reply(sock_fd, &m, &Players[i]);
            }
            break;
        default:
            perror("invalid message type");
            exit(-1);
            break;
        }
        if (pthread_mutex_unlock(&send_message_mutex) != 0){
            perror("unlock");
            exit(-1);
        }
    }


    pthread_mutex_destroy(&send_message_mutex);
    free(Players);
}