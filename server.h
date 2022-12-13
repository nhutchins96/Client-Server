#ifndef HEADER_FILE
#define HEADER_FILE

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <getopt.h>  // Input for the getopt variables
#include <ctype.h>

void command_line_check(int argc, char *argv[], char **port, char **socktype);

/* Pragma packs the struct to avoid padding which saves space and the size
    ref: https://gcc.gnu.org/onlinedocs/gcc-4.4.4/gcc/Structure_002dPacking-Pragmas.html
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

#endif