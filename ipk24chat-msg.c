// ipk24chat-msg.c

// IPK - 2. project
// IPK24-CHAT protocol
// Phamova Thu Tra - xphamo00
// FIT VUT 2024

#include "ipk24chat-msg.h"

int udp_msg_view(char *msgbuffer, struct info *client, struct msgprotocol *msg) {
    memset(msgbuffer, 0, BUFFSIZE);         // clear message buffer
    uint16_t byteorder = htons(msg->id);    // network byte order

    // message header
    msgbuffer[0] = msg->type;
    // saves value in msgbuffer that takes 2 bytes ([1] - MSB, [2] - LSB)
    memcpy(&msgbuffer[1], &byteorder, sizeof(uint16_t));

    // copy needed values in a msgbuffer after the header
        // +1, +2 or +3 to leave a '\0' in buffer in between values
    if ((msg->type == MSG && !msg->join && !msg->left) || msg->type == ERR) {
        memcpy(msgbuffer + MSGHEADER, client->display, strlen(client->display));
        memcpy(msgbuffer + MSGHEADER+strlen(client->display)+1, client->content, strlen(client->content));
        return (MSGHEADER+strlen(client->display)+strlen(client->content)+2);

    } else if (msg->type == MSG) {
        memcpy(msgbuffer + MSGHEADER, "Server", strlen("Server"));
        memcpy(msgbuffer + MSGHEADER+strlen("Server")+1, client->display, strlen(client->display));

        // broadcast MSG when left server
        if (msg->join && msg->left) {
            msg->join = false;
            msg->left = false;
            memcpy(msgbuffer + MSGHEADER+strlen("Server")+1+strlen(client->display), " has left ", strlen(" has left "));
            memcpy(msgbuffer + MSGHEADER+strlen("Server")+1+strlen(client->display)+strlen(" has left "), client->channel, strlen(client->channel));
            return (MSGHEADER+strlen("Server")+strlen(client->display)+strlen(" has left ")+strlen(client->channel)+2);

        // broadcast MSG when left from channel
        } else if (msg->left) {
            msg->left = false;
            memcpy(msgbuffer + MSGHEADER+strlen("Server")+1+strlen(client->display), " has left ", strlen(" has left "));
            memcpy(msgbuffer + MSGHEADER+strlen("Server")+1+strlen(client->display)+strlen(" has left "), client->old_channel, strlen(client->old_channel));
            return (MSGHEADER+strlen("Server")+strlen(client->display)+strlen(" has left ")+strlen(client->old_channel)+2);

        // broadcast MSG when joined in channel
        } else {
            memcpy(msgbuffer + MSGHEADER+strlen("Server")+1+strlen(client->display), " has joined ", strlen(" has joined "));
            memcpy(msgbuffer + MSGHEADER+strlen("Server")+1+strlen(client->display)+strlen(" has joined "), client->channel, strlen(client->channel));
            return (MSGHEADER+strlen("Server")+strlen(client->display)+strlen(" has joined ")+strlen(client->channel)+2);
        }

    } else if (msg->type == REPLY && msg->join) {
        msg->join = false;
        memcpy(msgbuffer + MSGHEADER, &msg->reply, sizeof(uint8_t));
        byteorder = htons(msg->clientid);
        memcpy(&msgbuffer[4], &byteorder, sizeof(uint16_t));
        memcpy(msgbuffer + MSGHEADER+sizeof(uint8_t)+sizeof(uint16_t), "Joining.", strlen("Joining."));
        return (MSGHEADER+sizeof(uint8_t)+sizeof(uint16_t)+strlen("Joining.")+1);

    } else if (msg->type == REPLY) {
        memcpy(msgbuffer + MSGHEADER, &msg->reply, sizeof(uint8_t));
        byteorder = htons(msg->clientid);
        memcpy(&msgbuffer[4], &byteorder, sizeof(uint16_t));
        memcpy(msgbuffer + MSGHEADER+sizeof(uint8_t)+sizeof(uint16_t), "Authorization.", strlen("Authorization."));
        return (MSGHEADER+sizeof(uint8_t)+sizeof(uint16_t)+strlen("Authorization.")+1);

    } else if (msg->type == CONFIRM) {
        byteorder = htons(msg->clientid);
        memcpy(&msgbuffer[1], &byteorder, sizeof(uint16_t));
        return (MSGHEADER);

    } else {
        // BYE
        return (MSGHEADER);
    }
}

char *type2string(char clientbuffer[]) {
    unsigned char clienttype = clientbuffer[0];
    switch(clienttype) {
        case CONFIRM:
            return "CONFIRM";
        case REPLY:
            return "REPLY";
        case AUTH:
            return "AUTH";
        case JOIN:
            return "JOIN";
        case MSG:
            return "MSG";
        case ERR:
            return "ERR";
        case BYE:
            return "BYE";
        default:
            return "ERR";
    }
}

bool recv_confirm(char *clientbuffer, char *serverbuffer, struct sockaddr_in *clientaddr, struct msgprotocol *msg, struct info *client) {
    socklen_t clientaddr_size = sizeof(clientaddr);
    ssize_t bytes_rx = recvfrom(bind_socket, clientbuffer, sizeof(clientbuffer), 0, (struct sockaddr *)clientaddr, &clientaddr_size);
    if (bytes_rx < 0) {
        error_bye_msg(clientbuffer, serverbuffer, clientaddr, clientaddr_size, msg, client, "Receive message error.");
    }

    if (clientbuffer[0] == CONFIRM &&
        serverbuffer[1] == clientbuffer[1] && 
        serverbuffer[2] == clientbuffer[2]) {
        
        print_recv_from(clientaddr, type2string(clientbuffer));
        return true;
    }
    return false;
}

void send_message(char *serverbuffer, struct sockaddr_in *clientaddr, socklen_t clientaddr_size, struct msgprotocol *msg, struct info *client, char *clientbuffer) {
    ssize_t bytes_tx = sendto(bind_socket, serverbuffer, msg->len, 0, (struct sockaddr *)clientaddr, clientaddr_size);
    if (bytes_tx < 0) {
        error_bye_msg(clientbuffer, serverbuffer, clientaddr, clientaddr_size, msg, client, "Send to message error.");
    }
}

void send_confirm(char *serverbuffer, struct sockaddr_in *clientaddr, socklen_t clientaddr_size, struct msgprotocol *msg, struct info *client, char *clientbuffer) {
    msg->type = CONFIRM;
    msg->len = udp_msg_view(serverbuffer, client, msg);
    send_message(serverbuffer, clientaddr, clientaddr_size, msg, client, clientbuffer);
    print_send_to(clientaddr, type2string(serverbuffer));
}

void send_bye(char *serverbuffer, char *clientbuffer, struct sockaddr_in *clientaddr, socklen_t clientaddr_size, struct msgprotocol *msg, struct info *client) {
    msg->type = BYE;
    msg->len = udp_msg_view(serverbuffer, client, msg);
    send_message(serverbuffer, clientaddr, clientaddr_size, msg, client, clientbuffer);
    print_send_to(clientaddr, type2string(serverbuffer));
    if (!recv_confirm(clientbuffer, serverbuffer, clientaddr, msg, client)) {
        error_bye_msg(clientbuffer, serverbuffer, clientaddr, clientaddr_size, msg, client, "Not received confirm.");
    }
}

void send_reply(char *serverbuffer, char *clientbuffer, struct sockaddr_in *clientaddr, socklen_t clientaddr_size, struct msgprotocol *msg, struct info *client) {
    msg->len = udp_msg_view(serverbuffer, client, msg);
    send_message(serverbuffer, clientaddr, clientaddr_size, msg, client, clientbuffer);
    print_send_to(clientaddr, type2string(serverbuffer));
    if (!recv_confirm(clientbuffer, serverbuffer, clientaddr, msg, client)) {
        error_bye_msg(clientbuffer, serverbuffer, clientaddr, clientaddr_size, msg, client, "Not received confirm.");
    }
}

void send_err(char *serverbuffer, char *clientbuffer, struct sockaddr_in *clientaddr, socklen_t clientaddr_size, struct msgprotocol *msg, struct info *client) {
    msg->type = ERR;
    msg->len = udp_msg_view(serverbuffer, client, msg);
    send_message(serverbuffer, clientaddr, clientaddr_size, msg, client, clientbuffer);
    print_send_to(clientaddr,type2string(serverbuffer));
    if (!recv_confirm(clientbuffer, serverbuffer, clientaddr, msg, client)) {
        error_bye_msg(clientbuffer, serverbuffer, clientaddr, clientaddr_size, msg, client, "Not received confirm.");
    }
}

void msg_in_channel(char *serverbuffer, const char *channel_name, struct msgprotocol *msg, struct info *client) {
    struct sockaddr_in clientaddr;
    socklen_t clientaddr_size = sizeof(clientaddr);
    char clientbuffer[BUFFSIZE];

    for (int i = 0; i < num_udp_clients; i++) {
        // looks for clients in the same channel
        if (strcmp(udp_clients[i].client.channel, channel_name) == 0) {
            memcpy(&clientaddr, &udp_clients[i].addr, sizeof(struct sockaddr_in));

            if (strcmp(client->username, udp_clients[i].client.username) != 0) {
                send_message(serverbuffer, &clientaddr, clientaddr_size, msg, client, clientbuffer);
                print_send_to(&clientaddr, type2string(serverbuffer));

                if (!recv_confirm(clientbuffer, serverbuffer, &clientaddr, msg, client)) {
                    error_bye_msg(clientbuffer, serverbuffer, &clientaddr, clientaddr_size, msg, client, "Not received confirm.");
                }
            }
        }
    }
}

void error_bye_msg(char *clientbuffer, char *serverbuffer, struct sockaddr_in *clientaddr, socklen_t clientaddr_size, struct msgprotocol *msg, struct info *client, char *text) {
    print_recv_from(clientaddr, type2string(clientbuffer));
    send_confirm(serverbuffer, clientaddr, clientaddr_size, msg, client, clientbuffer);
    
    msg->id++;
    memset(client->content, 0, sizeof(client->content));
    memcpy(client->content, text, strlen(text));
    send_err(serverbuffer, clientbuffer, clientaddr, clientaddr_size, msg, client);
    
    msg->id++;
    send_bye(serverbuffer, clientbuffer, clientaddr, clientaddr_size, msg, client);
    remove_udp_client(clientaddr);
}
