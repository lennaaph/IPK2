// ipk24chat-msg.h

// IPK - 2. project
// Phamova Thu Tra - xphamo00
// FIT VUT 2024

#ifndef IPK_MSG_H
#define IPK_MSG_H

#include "ipk24chat-server.h"

// builds message format to be send
int udp_msg_view(char *msgbuffer, struct info *client, struct msgprotocol *msg);

char *type2string(char clientbuffer[]);

// receiving and sending message
bool recv_confirm(char *clientbuffer, char *serverbuffer, struct sockaddr_in *clientaddr, struct msgprotocol *msg, struct info *client);
void send_message(char *serverbuffer, struct sockaddr_in *clientaddr, socklen_t clientaddr_size, struct msgprotocol *msg, struct info *client, char *clientbuffer);
void send_confirm(char *serverbuffer, struct sockaddr_in *clientaddr, socklen_t clientaddr_size, struct msgprotocol *msg, struct info *client, char *clientbuffer);
void send_bye(char *serverbuffer, char *clientbuffer, struct sockaddr_in *clientaddr, socklen_t clientaddr_size, struct msgprotocol *msg, struct info *client);
void send_reply(char *serverbuffer, char *clientbuffer, struct sockaddr_in *clientaddr, socklen_t clientaddr_size, struct msgprotocol *msg, struct info *client);
void send_err(char *serverbuffer, char *clientbuffer, struct sockaddr_in *clientaddr, socklen_t clientaddr_size, struct msgprotocol *msg, struct info *client);
void error_bye_msg(char *clientbuffer, char *serverbuffer, struct sockaddr_in *clientaddr, socklen_t clientaddr_size, struct msgprotocol *msg, struct info *client, char *text);
// broadcast handling
void msg_in_channel(char *serverbuffer, const char *channel_name, struct msgprotocol *msg, struct info *client);

#endif
