// ipk24chat-util.c

// IPK - 2. project
// IPK24-CHAT protocol
// Phamova Thu Tra - xphamo00
// FIT VUT 2024

#include "ipk24chat-util.h"

void help() {
    printf("Usage: ./ipk24chat-server -l <ip> -p <port> -d <timeout> -r <retransmissions>\n"
           "Options:\n"
           "  -l <ip>               IP address (default: 0.0.0.0)\n"
           "  -p <port>             Port (default: 4567)\n"
           "  -d <timeout>          UDP confirmation timeout (default: 250)\n"
           "  -r <retransmissions>  Maximum number of UDP retransmissions (default: 3)\n"
           "  -h                    Print this help message\n");
}

int is_user(const struct sockaddr_in *clientaddr) {
    for (int i = 0; i < num_udp_clients; i++) {
        if (memcmp(&udp_clients[i].addr, clientaddr, sizeof(struct sockaddr_in)) == 0) {
            return i;
        }
    }
    return ERROR;
}

bool is_username(struct info *client) {
    for (int i = 0; i < num_udp_clients; i++) {
        if (strcmp(udp_clients[i].client.username, client->username) == 0) {
            return true;
        }
    }
    return false;
}

bool reg_check(const char * string, int option) {
    regex_t regex;
    int regvalue;
    char *reg1 = "^[A-Za-z0-9-]+$"; // option 1
    char *reg2 = "^[\x21-\x7E]+$";  // option 2
    char *reg3 = "^[\x20-\x7E]+$";  // option 3

    // compile regex based on option
    switch (option) {
        case 1:
            regvalue = regcomp(&regex, reg1, REG_EXTENDED);
            break;
        case 2:
            regvalue = regcomp(&regex, reg2, REG_EXTENDED);
            break;
        case 3:
            regvalue = regcomp(&regex, reg3, REG_EXTENDED);
            break;
    }
    // error when compiling
    if (regvalue) return false;
    // comparing regex
    regvalue = regexec(&regex, string, 0, NULL, 0);
    // no match
    if (regvalue) return false;

    // frees the compiled regular expression
    regfree(&regex);
    return true;
}

bool get_user_info(char *clientbuffer, struct info *client) {
    memset(client->username, 0, PARAM_MAX);
    memset(client->secret, 0, SECRET_MAX);
    memset(client->display, 0, PARAM_MAX);
    int i;
    // username
    for (i = 0; clientbuffer[i + MSGHEADER] != '\0'; ++i) {
        if (i == PARAM_MAX) {
            return false;
        }
        client->username[i] = clientbuffer[i + MSGHEADER];
    }
    
    // display name
    for (i = 0; clientbuffer[i + MSGHEADER+strlen(client->username)+1] != '\0'; ++i) {
        if (i == PARAM_MAX) {
            return false;
        }
        client->display[i] = clientbuffer[i +MSGHEADER+strlen(client->username)+1];
    }
    client->display[i] = '\0';

    // secret
    for (i = 0; clientbuffer[i + MSGHEADER+strlen(client->username)+strlen(client->display)+2] != '\0'; ++i) {
        if (i == SECRET_MAX) {
            return false;
        }
        client->secret[i] = clientbuffer[i + MSGHEADER+strlen(client->username)+strlen(client->display)+2];
    }
    client->secret[i] = '\0';

    if (!reg_check(client->username, 1) || !reg_check(client->display, 2) || !reg_check(client->secret, 1)) {
        return false;
    }
    
    return true;
}

bool get_msg_info(char *clientbuffer, struct info *client) {
    memset(client->display, 0, PARAM_MAX);
    memset(client->content, 0, CONTENT_MAX);
    int i;
    // display name
    for (i = 0; clientbuffer[i + MSGHEADER] != '\0'; ++i) {
        if (i == PARAM_MAX) {
            return false;
        }
        client->display[i] = clientbuffer[i + MSGHEADER];
    }
    client->display[i] = '\0';

    // message content
    for (i = 0; clientbuffer[i + MSGHEADER + strlen(client->display) + 1] != '\0'; ++i) {
        if (i == CONTENT_MAX) {
            return false;
        }
        client->content[i] = clientbuffer[i + MSGHEADER + strlen(client->display) + 1];
    }
    client->content[i] = '\0';

    if (!reg_check(client->display, 2) || !reg_check(client->content, 3)) {
        return false;
    }
    return true;
}

bool get_join_info(char *clientbuffer, struct info *client) {
    memset(client->display, 0, PARAM_MAX);
    memset(client->channel, 0, PARAM_MAX);
    int i;
    // channel name
    for (i = 0; clientbuffer[i + MSGHEADER] != '\0'; ++i) {
        if (i == PARAM_MAX) {
            return false;
        }
        client->channel[i] = clientbuffer[i + MSGHEADER];
    }
    client->channel[i] = '\0';

    // display name
    for (i = 0; clientbuffer[i + MSGHEADER + strlen(client->channel) + 1] != '\0'; ++i) {
        if (i == CONTENT_MAX) {
            return false;
        }
        client->display[i] = clientbuffer[i + MSGHEADER + strlen(client->channel) + 1];
    }
    client->display[i] = '\0';

    if (!reg_check(client->channel, 1) || !reg_check(client->display, 2)) {
        return false;
    }
    return true;
}
