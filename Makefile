#  Makefile 

#  IPK - 2. project
#  IPK24-CHAT protocol
#  Phamova Thu Tra - xphamo00
#  FIT VUT 2024

C_FLAGS = -std=gnu99 -Wall -pedantic -pthread
CC=gcc
 
all: ipk24chat-server

ipk24chat-server: ipk24chat-server.c ipk24chat-util.c ipk24chat-msg.c ipk24chat-server.h ipk24chat-util.h ipk24chat-msg.h
	$(CC) $(C_FLAGS) -o $@ ipk24chat-server.c ipk24chat-util.c ipk24chat-msg.c

clean:
	rm -f ipk24chat-server
