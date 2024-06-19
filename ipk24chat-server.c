// ipk24chat-server.c

// IPK - 2. project
// IPK24-CHAT protocol
// Phamova Thu Tra - xphamo00
// FIT VUT 2024

#include "ipk24chat-server.h"
#include "ipk24chat-msg.h"
#include "ipk24chat-util.h"

int num_tcp_clients = 0;
int num_udp_clients = 0;

// parses arguments and save correct values
void args_parse(int argc, char *argv[], Server *server_info) {
    // sets default values
    server_info->ip_addr = IP_DEFAULT;
    server_info->port = atoi(PORT_DEFAULT);
    server_info->timeout = atoi(TIME_DEFAULT);
    server_info->retrans = atoi(RETRANS_DEFAULT);

    int option;
    while ((option = getopt(argc, argv, "l:p:d:r:h")) != -1) {
        switch (option) {
            case 'l':
                server_info->ip_addr = optarg;
                break;
            case 'p':
                server_info->port = atoi(optarg);
                break;
            case 'd':
                server_info->timeout = atoi(optarg);
                break;
            case 'r':
                server_info->retrans = atoi(optarg);
                break;
            case 'h':
                help();
                exit(0);
            default:
                fprintf(stderr, "Usage: ./ipk24chat-server -l <ip> -p <port> -d <timeout> -r <retransmissions>\n");
                exit(ERROR);
        }
    }
}


void print_ip(const char* action, struct sockaddr_in *clientaddr, char* type) {
    // gets IP address and port
    char clientip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clientaddr->sin_addr), clientip, INET_ADDRSTRLEN);
    uint16_t clientport = ntohs(clientaddr->sin_port);

    // prints the message
    fprintf(stdout, "%s %s:%d | %s\n", action, clientip, clientport, type);
}

void print_recv_from(struct sockaddr_in *clientaddr, char* type) {
    print_ip("RECV", clientaddr, type);
}

void print_send_to(struct sockaddr_in *clientaddr, char* type) {
    print_ip("SENT", clientaddr, type);
}

// adds a new UDP client to the global array
bool add_udp_client(const struct sockaddr_in *clientaddr, struct info *client) {
    for (int i = 0; i < num_udp_clients; i++) {
        if (memcmp(&udp_clients[i].addr, clientaddr, sizeof(struct sockaddr_in)) == 0) {
            return false;
        }
    }

    if (num_udp_clients < MAX_EVENTS) {
        memcpy(&udp_clients[num_udp_clients].addr, clientaddr, sizeof(struct sockaddr_in));
        udp_clients[num_udp_clients].client = *client;
        num_udp_clients++;
        return true;
    }
    return false;
}

void remove_udp_client(const struct sockaddr_in *client_addr) {
    for (int i = 0; i < num_udp_clients; i++) {
        if (memcmp(&udp_clients[i].addr, client_addr, sizeof(struct sockaddr_in)) == 0) {
            // removes client by shifting
            for (int j = i; j < num_udp_clients - 1; j++) {
                udp_clients[j] = udp_clients[j + 1];
            }
            num_udp_clients--;
            return;
        }
    }
}

void udp_client(Server server_info, struct msgprotocol *msg) {
    struct sockaddr_in clientaddr;
    socklen_t clientaddr_size = sizeof(clientaddr);
    unsigned long bytes_rx;

    char clientbuffer[BUFFSIZE];    // recv buffer
    char serverbuffer[BUFFSIZE];    // sent buffer
    memset(clientbuffer, 0, sizeof(clientbuffer));
    memset(serverbuffer, 0, sizeof(serverbuffer));

    struct info client;
    msg->join = false;
    msg->left = false;
    msg->id++;

    // checks if it is a new client
    if (msg->new) {
        // new client receives on old socket
        bytes_rx = recvfrom(udp_socket, clientbuffer, sizeof(clientbuffer), 0, (struct sockaddr *)&clientaddr, &clientaddr_size);
        if (bytes_rx < 0) {
            error_bye_msg(clientbuffer, serverbuffer, &clientaddr, clientaddr_size, msg, &client, "Receive message error.");
        }   
    } else {
        // client exists already and receives on new bind socket
        bytes_rx = recvfrom(bind_socket, clientbuffer, sizeof(clientbuffer), 0, (struct sockaddr *)&clientaddr, &clientaddr_size);
        if (bytes_rx < 0) {
            error_bye_msg(clientbuffer, serverbuffer, &clientaddr, clientaddr_size, msg, &client, "Receive message error.");
        }
    }
    msg->clientid = (clientbuffer[1] << 8) | clientbuffer[2];

    int client_index = is_user(&clientaddr);
    if (client_index != ERROR) {
        // already registered client
        client = udp_clients[client_index].client;
    } else {
        // adds new client to the global array
        client.retrans = 0;
        client.state = S_START;
        add_udp_client(&clientaddr, &client);
    }

    unsigned char clienttype = clientbuffer[0];
    switch (client.state) {
        case S_START:
            // AUTH
            if (clientbuffer[0] == AUTH) {
                print_recv_from(&clientaddr, type2string(clientbuffer));
                send_confirm(serverbuffer, &clientaddr, clientaddr_size, msg, &client, clientbuffer);
                // checks username, secret and display name
                if (!get_user_info(clientbuffer, &client)) {
                    error_bye_msg(clientbuffer, serverbuffer, &clientaddr, clientaddr_size, msg, &client, "AUTH content values error.");
                }

                if (bytes_rx != MSGHEADER + strlen(client.username) + strlen(client.display) + strlen(client.secret) + 3) {
                    error_bye_msg(clientbuffer, serverbuffer, &clientaddr, clientaddr_size, msg, &client, "Sent wrong format error.");
                }

                msg->type = REPLY;
                if (is_username(&client)) {
                    // sends REPLY!
                    msg->reply = 0;
                    msg->id++;
                    send_reply(serverbuffer, clientbuffer, &clientaddr, clientaddr_size, msg, &client);
                    if (client.retrans == server_info.retrans) {
                        msg->id++;
                        send_bye(serverbuffer, clientbuffer, &clientaddr, clientaddr_size, msg, &client);
                        remove_udp_client(&clientaddr);
                    }
                    client.retrans++;

                    if (is_user(&clientaddr) != ERROR) {
                        // saves new information in existing client
                        udp_clients[is_user(&clientaddr)].client = client;
                    }
                    break;
                } else {
                    // sends REPLY+
                    msg->reply = 1;
                    msg->id++;
                    send_reply(serverbuffer, clientbuffer, &clientaddr, clientaddr_size, msg, &client);
                    client.state = S_OPEN;
                }
            } else {
                break;
            }

            if (is_user(&clientaddr) != ERROR) {
                // add default channel
                memset(client.channel, 0, PARAM_MAX);
                strcpy(client.channel, "default");

                // saves new information in existing client
                udp_clients[is_user(&clientaddr)].client = client;

                // sends MSG broadcast
                msg->type = MSG;
                msg->join = true;
                msg->id++;
                msg->len = udp_msg_view(serverbuffer, &client, msg);
                msg_in_channel(serverbuffer, client.channel, msg, &client);
            }
            break;
        case S_OPEN:
            // MSG
            if (clienttype == MSG) {
                print_recv_from(&clientaddr, type2string(clientbuffer));
                send_confirm(serverbuffer, &clientaddr, clientaddr_size, msg, &client, clientbuffer);
                // gets message content
                if (!get_msg_info(clientbuffer, &client)) {
                    error_bye_msg(clientbuffer, serverbuffer, &clientaddr, clientaddr_size, msg, &client, "MSG content values error.");
                }
                if (bytes_rx != MSGHEADER + strlen(client.display) + strlen(client.content) + 2) {
                    error_bye_msg(clientbuffer, serverbuffer, &clientaddr, clientaddr_size, msg, &client, "Sent wrong format error.");
                }
                
                // sends MSG broadcast
                msg->id++;
                msg->type = MSG;
                msg->len = udp_msg_view(serverbuffer, &client, msg);
                msg_in_channel(serverbuffer, client.channel, msg, &client);

            // JOIN
            } else if (clienttype == JOIN) {
                print_recv_from(&clientaddr, type2string(clientbuffer));
                send_confirm(serverbuffer, &clientaddr, clientaddr_size, msg, &client, clientbuffer);

                strcpy(client.old_channel, client.channel);
                // gets channelID and display name
                if (!get_join_info(clientbuffer, &client)) {
                    error_bye_msg(clientbuffer, serverbuffer, &clientaddr, clientaddr_size, msg, &client, "JOIN content values error.");
                }
                if (bytes_rx != MSGHEADER + strlen(client.channel) + strlen(client.display) + 2) {
                    error_bye_msg(clientbuffer, serverbuffer, &clientaddr, clientaddr_size, msg, &client, "Sent wrong format error.");
                }

                // updates client
                udp_clients[is_user(&clientaddr)].client = client;

                // sends left MSG broadcast
                msg->id++;
                msg->type = MSG;
                msg->left = true;
                msg->len = udp_msg_view(serverbuffer, &client, msg);
                msg_in_channel(serverbuffer, client.old_channel, msg, &client);

                // sends REPLY+
                msg->id++;
                msg->type = REPLY;
                msg->reply = 1;
                msg->join = true;
                send_reply(serverbuffer, clientbuffer, &clientaddr, clientaddr_size, msg, &client);

                // sends joined MSG broadcast
                msg->id++;
                msg->type = MSG;
                msg->join = true;
                msg->len = udp_msg_view(serverbuffer, &client, msg);
                msg_in_channel(serverbuffer, client.channel, msg, &client);

            // ERR
            } else if (clienttype == ERR) {
                print_recv_from(&clientaddr, type2string(clientbuffer));
                send_confirm(serverbuffer, &clientaddr, clientaddr_size, msg, &client, clientbuffer);
                msg->id++;
                send_bye(serverbuffer, clientbuffer, &clientaddr, clientaddr_size, msg, &client);
                remove_udp_client(&clientaddr);

            // BYE
            } else if (clienttype == BYE) {
                print_recv_from(&clientaddr, type2string(clientbuffer));
                send_confirm(serverbuffer, &clientaddr, clientaddr_size, msg, &client, clientbuffer);
                
                memset(udp_clients[is_user(&clientaddr)].client.channel, 0, PARAM_MAX);
                // sends left MSG broadcast
                msg->id++;
                msg->type = MSG;
                msg->join = true;
                msg->left = true;
                msg->len = udp_msg_view(serverbuffer, &client, msg);
                msg_in_channel(serverbuffer, client.channel, msg, &client);

                remove_udp_client(&clientaddr);

            } else {
            // UNKNOWN
                error_bye_msg(clientbuffer, serverbuffer, &clientaddr, clientaddr_size, msg, &client, "Unknown message type sent.");
            }
            break;
    }
}

// sends bye to everyone
void bye_all() {
    for (int i = 0; i < num_udp_clients; i++) {
        char serverbuffer[BUFFSIZE];
        char clientbuffer[BUFFSIZE];
        memset(clientbuffer, 0, sizeof(clientbuffer));
        memset(serverbuffer, 0, sizeof(serverbuffer));
        struct sockaddr_in clientaddr = udp_clients[i].addr;
        socklen_t clientaddr_size = sizeof(clientaddr);

        uint16_t byteorder = htons(1000);
        serverbuffer[0] = (unsigned char)BYE;
        memcpy(&serverbuffer[1], &byteorder, sizeof(uint16_t));
        
        // new bind port
        ssize_t bytes_tx = sendto(bind_socket, serverbuffer, MSGHEADER, 0, (struct sockaddr *)&clientaddr, clientaddr_size);
        if (bytes_tx < 0) {
            perror("Sending bye every client failed.");
            close(udp_socket);
            close(tcp_socket);
            close(bind_socket);
            exit(EXIT_FAILURE);
        }
        print_send_to(&clientaddr, type2string(serverbuffer));
    }
}

// checks for C-c
void ctrlc(){
    bye_all();
    close(udp_socket);
    close(tcp_socket);
    close(bind_socket);
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    if (signal(SIGINT, ctrlc) == SIG_ERR) {
        perror("SIGINT failed.");
        exit(EXIT_FAILURE);
    }
    Server server_info;
    args_parse(argc, argv, &server_info);

    struct pollfd fds[MAX_EVENTS];

    // creates UDP and TCP socket
    tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_socket < 0) {
        perror("TCP socket creation failed.");
        exit(EXIT_FAILURE);
    }
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
        perror("UDP socket creation failed.");
        exit(EXIT_FAILURE);
    }
    // creates new port to bind
    bind_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (bind_socket < 0) {
        perror("New socket creation failed.");
    }

    // sets up server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(server_info.port);

    // sets up server address with new port
    memset(&udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    udp_addr.sin_port = htons(0); // new port
    
    // binds for UDP and TCP socket
    if (bind(tcp_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("TCP bind failed.");
        exit(EXIT_FAILURE);
    }
    if (bind(udp_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("UDP bind failed.");
        exit(EXIT_FAILURE);
    }
    if (bind(bind_socket, (struct sockaddr*)&udp_addr, sizeof(udp_addr)) < 0) {
        perror("Binding new socket failed.");
        exit(EXIT_FAILURE);
    }
    
    // listens for TCP socket
    if (listen(tcp_socket, SOMAXCONN) < 0) {
        perror("TCP listen failed.");
        exit(EXIT_FAILURE);
    }
  
    // for TCP socket
    fds[0].fd = tcp_socket;
    fds[0].events = POLLIN;
    // for UDP socket
    fds[1].fd = udp_socket;
    fds[1].events = POLLIN;
    // for new port UDP socket
    fds[2].fd = bind_socket;
    fds[2].events = POLLIN;

    struct msgprotocol msg; // useful information about message format
    msg.id = 0;
    
    while (1) {
        msg.new = false;
        int activity = poll(fds, 3, -1);
        if (activity < 0) {
            perror("Function poll() error.");
            break;
        }
        
        // TCP
        if (fds[0].revents & POLLIN) {
        }
        
        // UDP
        if (fds[1].revents & POLLIN) {
            msg.new = true;
            udp_client(server_info, &msg);
        }

        // new port UDP
        if (fds[2].revents & POLLIN) {
            udp_client(server_info, &msg);
        }
    }
    
    close(tcp_socket);
    close(udp_socket);
    close(bind_socket);
    return 0;
}
