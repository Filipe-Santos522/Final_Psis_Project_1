# C compiler
HOST_CC = gcc  -O2

FLAGS = -Wall -g

HOST_CC_FLAGS =  -lncurses -lpthread

all: server client

server: server.o single-pong.o Serverfunc.o
	$(HOST_CC) $(FLAGS) server.o single-pong.o Serverfunc.o -o server $(HOST_CC_FLAGS)

client: client.o single-pong.o Serverfunc.o
	$(HOST_CC) $(FLAGS) client.o  single-pong.o Serverfunc.o -o client $(HOST_CC_FLAGS)



server.o: server.c 
	$(HOST_CC) $(FLAGS) -c server.c -o server.o  $(HOST_CC_FLAGS)

client.o: client.c
	$(HOST_CC) $(FLAGS) -c client.c -o client.o  $(HOST_CC_FLAGS)

single-pong.o: single-pong.c single-pong.h pong.h
	$(HOST_CC) $(FLAGS) -c single-pong.c -o single-pong.o  $(HOST_CC_FLAGS)

Serverfunc.o: Serverfunc.c single-pong.h Serverfunc.h sock_dg_inet.h
	$(HOST_CC) $(FLAGS) -c Serverfunc.c -o Serverfunc.o  $(HOST_CC_FLAGS)

clean:
	rm -f *.o *~ server client
