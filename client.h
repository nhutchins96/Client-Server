#ifndef HEADER_FILE
#define HEADER_FILE

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <getopt.h>  // Input for the getopt variables
#include <ctype.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/time.h>

/* Ref to pragma: https://gcc.gnu.org/onlinedocs/gcc-4.4.4/gcc/Structure_002dPacking-Pragmas.html
This packing will help keep the message size as small as possible. This avoids any auto padding the 
compiler may try to do. Thus we are left with a 5 byte client message and 1 byte server message.
*/
#pragma pack(1)
struct client_message
{
    uint8_t version; // 1-byte version field 
    uint32_t data;  // 4-byte unsigned int of user data, this will be encoded and decoded with htonl and ntohl
};

#pragma pack(1)
struct server_message
{
    uint8_t version; // 1-byte version field 
};

void command_line_check(int argc, char *argv[], uint32_t *data, char **port, char **socktype, char **ip);
int sendall(int s, struct client_message *message, int *len);
int recvtimeout(int s, struct server_message *message, int len, int timeout, struct sockaddr *addr, socklen_t *addr_len);

#endif