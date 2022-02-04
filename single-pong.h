/*PSis Project 2 - 21/22
 * Filipe Santos - 90068
 * Alexandre Fonseca - 90210
 */
#include <stdlib.h>
#include <ncurses.h>

#include "pong.h"

WINDOW * message_win;

typedef struct ball_position_t{
    int x, y;
    int up_hor_down; //  -1 up, 0 horizontal, 1 down
    int left_ver_right; //  -1 left, 0 vertical,1 right
    char c;
} ball_position_t;

typedef struct paddle_position_t{
    int x, y;
    int length;
} paddle_position_t;

typedef struct message{
    int type; //1 - connect, 2 - release_ball, 3 - send_ball, 4 - move_ball, 5 - disconnect
    ball_position_t ball;
} message;


void new_paddle (paddle_position_t * paddle, int legth);
void draw_paddle(WINDOW *win, paddle_position_t * paddle, int delete);
void moove_paddle (paddle_position_t * paddle, int direction, ball_position_t  *ball);
void place_ball_random(ball_position_t * ball);
void moove_ball(ball_position_t * ball, paddle_position_t paddle);
void draw_ball(WINDOW *win, ball_position_t * ball, int draw);