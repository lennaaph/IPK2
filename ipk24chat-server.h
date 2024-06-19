// ipk24chat-server.h

// IPK - 2. project
// Phamova Thu Tra - xphamo00
// FIT VUT 2024

#ifndef IPK_H
#define IPK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <regex.h>
#include <signal.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>

// default values
#define IP_DEFAULT      "0.0.0.0"
#define PORT_DEFAULT    "4567"
#define TIME_DEFAULT    "250"
#define RETRANS_DEFAULT "3"

// message types
#define CONFIRM     0x00
#define REPLY       0x01
#define AUTH        0x02
#define JOIN        0x03
#define MSG         0x04
#define ERR         0xFE
#define BYE         0xFF

// states
#define S_START     100
#define S_AUTH      101
#define S_OPEN      102
#define S_ERROR     103
#define S_END       104

#define MSGHEADER   3
#define ERROR       -1

// size of buffer
#define BUFFSIZE    2048
#define PARAM_MAX   20
#define SECRET_MAX  128
#define CONTENT_MAX 1000
#define ARG_MAX     256

#define MAX_EVENTS   50

typedef struct {
    char *ip_addr;
    uint16_t port;
    uint16_t timeout;
    uint8_t retrans;
} Server;

struct msgprotocol {
    uint16_t id;        // server's id
    uint8_t type;       // message type
    int reply;          // decides false/true reply
    int len;            // message length
    bool left;          // for broadcast messages
    bool join;          // for broadcast messages
    bool new;           // detects the new client
    uint16_t clientid;  // client's id
};

struct info {
    uint8_t retrans;
    int state;
    char username[PARAM_MAX];
    char secret[SECRET_MAX];
    char display[PARAM_MAX];
    char channel[PARAM_MAX];
    char old_channel[PARAM_MAX];
    char content[CONTENT_MAX];
};

typedef struct {
    int fd;
    struct sockaddr_in addr;
} TCPClient;

typedef struct {
    struct sockaddr_in addr;
    struct info client;
} UDPClient;

// global variables
struct sockaddr_in server_addr, udp_addr;
int tcp_socket, udp_socket, bind_socket;
TCPClient tcp_clients[MAX_EVENTS];
UDPClient udp_clients[MAX_EVENTS];
extern int num_udp_clients;
extern int num_tcp_clients;

void args_parse(int argc, char *argv[], Server *server_info);

// prints output on server side
void print_ip(const char* action, struct sockaddr_in *clientaddr, char* type);
void print_recv_from(struct sockaddr_in *clientaddr, char* type);
void print_send_to(struct sockaddr_in *clientaddr, char* type);

bool add_udp_client(const struct sockaddr_in *clientaddr, struct info *client);
void remove_udp_client(const struct sockaddr_in *client_addr);

void udp_client(Server server_info, struct msgprotocol *msg);
void bye_all();

// signal handler
void ctrlc();

#endif
