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

void * getKey(void * arg){
    while(1){
        pthread_mutex_lock(&key_lock);
        pthread_cond_wait(&cond, &key_lock);
        pthread_mutex_unlock(&key_lock);
        key = wgetch(my_win);
        flag=1;
    }
    return NULL;
}

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
                pthread_mutex_lock(&draw_mutex);
                draw_paddle(my_win, &paddle, false);
                m.type=4;
                pthread_mutex_unlock(&draw_mutex);
                break;
            case 3:
                pthread_create(&message_thread, NULL, recvMessage, NULL);
                pthread_mutex_lock(&draw_mutex);
                draw_ball(my_win, &old_ball, false);
                draw_ball(my_win, &m.ball, true);
                draw_paddle(my_win, &paddle, true);
                pthread_mutex_unlock(&draw_mutex);
                key = -1;
                pthread_create(&ball_move_thread, NULL, moveBall, NULL);
                pthread_mutex_lock(&key_lock);
                pthread_cond_signal(&cond);
                pthread_mutex_unlock(&key_lock);
                m.type = 4;
                while(key != 113 && key != 114 && m.type==4){
                    pthread_mutex_lock(&draw_mutex);
                    wrefresh(my_win);
                    pthread_mutex_unlock(&draw_mutex);
                    
                    if(flag==1){
                        pthread_mutex_lock(&draw_mutex);
                        //make_play(key, my_win, &paddle, &m.ball); 
                        if (key == KEY_LEFT || key == KEY_RIGHT || key == KEY_UP || key == KEY_DOWN){
                            draw_paddle(my_win, &paddle, false);
                            draw_ball(my_win, &old_ball, false);
                            moove_paddle (&paddle, key, &m.ball);
                            draw_paddle(my_win, &paddle, true);
                            draw_ball(my_win, &m.ball, true);
                            old_ball=m.ball;
                        }
                        mvwprintw(message_win, 1,1,"%c key pressed", key);
                        mvwprintw(message_win, 2,1,"flag %d", flag);
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
                draw_paddle(my_win, &paddle, false);
                pthread_mutex_unlock(&draw_mutex);
                pthread_join(message_thread, NULL);
                pthread_join(ball_move_thread, NULL);
                break;
            
            case 4:
                /* Update the ball position on the screen (without paddle)*/
                //update_ball_on_screen(my_win, &m.ball, paddle);
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