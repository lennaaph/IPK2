// ipk24chat-util.h

// IPK - 2. project
// Phamova Thu Tra - xphamo00
// FIT VUT 2024

#ifndef IPK_UTIL_H
#define IPK_UTIL_H

#include "ipk24chat-server.h"

// usage-help
void help();

int is_user(const struct sockaddr_in *clientaddr);
bool is_username(struct info *client);

// handling information from client messages
bool reg_check(const char * string, int option);
bool get_user_info(char *clientbuffer, struct info *client);
bool get_msg_info(char *clientbuffer, struct info *client);
bool get_join_info(char *clientbuffer, struct info *client);

#endif
