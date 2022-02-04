/*PSis Project 2 - 21/22
 * Filipe Santos - 90068
 * Alexandre Fonseca - 90210
 */
#include <ncurses.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "Serverfunc.h"
#include "sock_dg_inet.h"

paddle_position_t paddle;
message m;
int key, count;
WINDOW * my_win;
ball_position_t old_ball;
int flag=0, sock_fd;
pthread_mutex_t draw_mutex;
pthread_mutex_t key_lock;
pthread_cond_t cond;
struct sockaddr_in server_addr;


// Thread function that moves the ball every second
void * moveBall(void * arg){
    while(key != 113 && key != 114 && m.type==4){
        pthread_mutex_lock(&draw_mutex);
        if(m.type==4){
        draw_ball(my_win, &m.ball, false);
        moove_ball(&m.ball, paddle);
        draw_ball(my_win, &m.ball, true);
        old_ball=m.ball;
        Send_Reply(sock_fd, &m, &server_addr);
        }
        pthread_mutex_unlock(&draw_mutex);
        sleep(1);
    }
    return NULL;
}

// Thread function that reads a key from the keyboard
void * getKey(void * arg){
    while(1){
        pthread_mutex_lock(&key_lock); 
        pthread_cond_wait(&cond, &key_lock); //Condition variable that waits for the previous key press to be processed
        pthread_mutex_unlock(&key_lock);
        key = wgetch(my_win);
        flag=1;
    }
    return NULL;
}

// Thread function to receive a message from the server
void * recvMessage(void * arg){
    Receive_message(sock_fd, &m, &server_addr);
    return NULL;
}

int main(int argc, char** argv){
    if(argc!=2){
        printf("Error in arguments\n");
        exit(1);
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SOCK_PORT);
    inet_pton(AF_INET, argv[1], &server_addr.sin_addr);

    sock_fd=Socket_creation();
    m.type = 1; /* Set message type to "connect"*/
    Send_Reply(sock_fd, &m, &server_addr); /* Send connect message */

    initscr();		    	/* Start curses mode 		*/
	cbreak();				/* Line buffering disabled	*/
    keypad(stdscr, TRUE);   /* We get F1, F2 etc..		*/
	noecho();			    /* Don't echo() while we do getch */

    /* creates a window and draws a border */
    my_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 0, 0);
    box(my_win, 0 , 0);	
	wrefresh(my_win);
    keypad(my_win, true);
    /* creates a window and draws a border */
    message_win = newwin(5, WINDOW_SIZE+10, WINDOW_SIZE, 0);
    box(message_win, 0 , 0);	
	wrefresh(message_win);

    new_paddle(&paddle, PADLE_SIZE);

    pthread_t ball_move_thread, get_key_thread, message_thread;
    
    pthread_create(&get_key_thread, NULL, getKey, NULL);
    pthread_mutex_init(&draw_mutex, NULL);

    while (1){
        Receive_message(sock_fd, &m, &server_addr);
        switch (m.type){
            case 2:
                //Release ball - erase paddle form the screen safely
                pthread_mutex_lock(&draw_mutex);
                draw_paddle(my_win, &paddle, false);
                m.type=4;
                pthread_mutex_unlock(&draw_mutex);
                break;
            case 3:
                //Playing state
                pthread_create(&message_thread, NULL, recvMessage, NULL); //Create thread to receive release ball message
                pthread_mutex_lock(&draw_mutex);
                draw_ball(my_win, &old_ball, false);
                draw_ball(my_win, &m.ball, true); //Update ball
                draw_paddle(my_win, &paddle, true); //Draw paddle
                pthread_mutex_unlock(&draw_mutex);
                key = -1;
                pthread_create(&ball_move_thread, NULL, moveBall, NULL); //Create thread to move the ball
                pthread_mutex_lock(&key_lock);
                pthread_cond_signal(&cond); //Signal the getKey thread
                pthread_mutex_unlock(&key_lock);
                m.type = 4;
                while(key != 113 && key != 114 && m.type==4){
                    pthread_mutex_lock(&draw_mutex);
                    wrefresh(my_win);
                    pthread_mutex_unlock(&draw_mutex);
                    
                    if(flag==1){ //If a key was pressed, update the board and send a message to the server
                        pthread_mutex_lock(&draw_mutex);
                        if (key == KEY_LEFT || key == KEY_RIGHT || key == KEY_UP || key == KEY_DOWN){
                            draw_paddle(my_win, &paddle, false);
                            draw_ball(my_win, &old_ball, false);
                            moove_paddle (&paddle, key, &m.ball);
                            draw_paddle(my_win, &paddle, true);
                            draw_ball(my_win, &m.ball, true);
                            old_ball=m.ball;
                        }
                        mvwprintw(message_win, 1,1,"%c key pressed", key);
                        wrefresh(message_win); 
                        Send_Reply(sock_fd, &m, &server_addr);
                        flag=0;
                        pthread_mutex_lock(&key_lock);
                        pthread_cond_signal(&cond);
                        pthread_mutex_unlock(&key_lock);
                        pthread_mutex_unlock(&draw_mutex);
                    }
                }
                /* Check which key was pressed to stop playing*/
                if (key == 113){

                    m.type = 5; /* Change message type to "disconnect"*/
                    Send_Reply(sock_fd, &m, &server_addr); /* Send "disconnect" message*/
                    endwin();
                    exit(1);
                    break;
                }
                else if (key == 114){
                    m.type = 2; /* Change message type to "release ball"*/
                    draw_paddle(my_win, &paddle, false);

                    Send_Reply(sock_fd, &m, &server_addr); /* Send "release ball" message*/
                    mvwprintw(message_win, 1,1,"%c key pressed", key);
                }
                pthread_mutex_lock(&draw_mutex);
                draw_paddle(my_win, &paddle, false); //Erase paddle when quitting play state
                pthread_mutex_unlock(&draw_mutex);
                pthread_join(message_thread, NULL);
                pthread_join(ball_move_thread, NULL);
                break;
            
            case 4:
                /* Update the ball position on the screen (without paddle)*/
                pthread_mutex_lock(&draw_mutex);
                draw_ball(my_win, &old_ball, false);
                draw_ball(my_win, &m.ball, true);
                old_ball=m.ball;
                pthread_mutex_unlock(&draw_mutex);
                break;
            case 6:
                pthread_mutex_lock(&draw_mutex);
                draw_ball(my_win, &m.ball, true);
                old_ball=m.ball;
                pthread_mutex_unlock(&draw_mutex);
                break;
            default:
                perror("invalid message type");
                exit(-1);
                break;
        }
    }
    pthread_mutex_destroy(&draw_mutex);
    close(sock_fd);
    return 1;
}