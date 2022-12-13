/* Written by Nathan Hutchins (nahu7321@colorado.edu)

This is the client portion of the Client/Server CSPB 3753 assignment.

How-to: 
    After running make. You can send message with the client executable
    ./client -x <32-bit unsigned int data> -t <udp/tcp> -s <ip> -p <number>

What this does:
    This will establish a connection with the running server code on the port 
    and socket type provided. It will then send the provided data in a struct
    that has been packed to avoid sending a padded struct with extra memory.
    After ending the struct, the client waits for a response and will timeout
    if the server doesn't respond within 3 seconds. 

    The client will also return an error if the server struct version is not
    set to 1.l

What wasn't completed:
    Checking for a correct format ip and/or hostname. The tcp connection will fail 
    if the name is incorrect so there is error checking there. UDP will timeout if
    it cannot find a host or ip so that also has error checking. I would've liked
    to have added some regex checking or something to quickly check ip ranges on 
    input. Currently you can use ip or hostname and it will error out if the 
    client can't connect to a server.

Reference:
    Thank you to the beej guide for the very detailed walkthrough of 
    client server communication and Network Programming.
    https://beej.us/guide/bgnet/html/

*/
#include "client.h"

int main(int argc, char *argv[]){
    // Check that the correct number of arguments were given     
    if (argc != 9){
        // Set error to invalid arg
        errno = 22;
        fprintf(stderr, "Incorrect number of arguments.\n");
        return -1;
    }
    
    // Save command line arguments in these
    uint32_t data;
    char *socktype;
    char *ip;
    char *port;
    
    // Make sure the command line is correct and populate the needed data
    command_line_check(argc, argv, &data, &port, &socktype, &ip);

    // Prep socket address structures for holding needed socket flags/information
    struct addrinfo start_socket_addr, *res, *p;

    // A storage structure to hold our IPv4 strucutres
    struct sockaddr_storage their_addr;
    socklen_t addr_size;

    /* Parameters used to hold socket data, number of bytes returned when sending, and 
        status of send/recv functions. 
    */
    int sockfd;
    int numbytes;
    int status;

    /* 
        Custom structures packed to hold a 1 byte version number in both, and a 4 byte 
        data message in the client_message strucutre.
        See packing details in client.h and reference to packing logic.
    */
    struct client_message send_message;
    struct server_message server_message_struct;

    // Fill socket addr with 0s
    memset(&start_socket_addr, 0, sizeof(start_socket_addr));

    // Set to IPv4
    start_socket_addr.ai_family = AF_INET;

    /*
        Set to socket type: SOCK_DGRAM -> UDP or SOCK_STREAM -> TCP
    */

    // Set socktype depending on udp or tcp
    if (strcmp(socktype, "udp") == 0){
        start_socket_addr.ai_socktype = SOCK_DGRAM;
    }
    else if (strcmp(socktype, "tcp") == 0){
        start_socket_addr.ai_socktype = SOCK_STREAM;
    }

    // Setup struct - this includes the DNS and service name lookups
    // Also gives pointer to linked-list of results (pointer will be res parameter).
    if((status = getaddrinfo(ip, port, &start_socket_addr, &res)) != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return -1;
    }

    // loop through all the results and make a socket with the first possible result
    for(p = res; p != NULL; p = p->ai_next) {
        // Attempt to make a socket using the first result viable
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            continue;
        }
        break;
    }

    // Check socket was created correctly
    if (p == NULL) {
        fprintf(stderr, "client: failed to create socket\n");
        return -1;
    }

    /* Prep message by setting verion to 1 (1 byte) and encoding data
        This struct is already packed to avoid padding. See client.h 
    */
    send_message.version = 1;
    send_message.data = htonl(data);

    // connect if tcp
    if (strcmp(socktype, "tcp") == 0){
        // Attempt to connect 
        status = connect(sockfd, p->ai_addr, p->ai_addrlen);
        if (status != 0){
            fprintf(stderr, "client: failed to connect with socket. Server may be listening on UDP or different port.\n");
            return -1;
        }

        // Send message to server upon successful connection and read amount of bytes sent.
        numbytes = send(sockfd, &send_message, sizeof(send_message), 0);
        if (numbytes == -1){ // -1 is an error occuring when sending
            fprintf(stderr, "Error sending message.\n");
            return -1;
        }

        // check if we have left over bytes and send them until we are done or get error
        numbytes = sizeof(send_message) - numbytes;
        if (numbytes > 0){
            // If fail, log how many bytes were sent
            if (sendall(sockfd, &send_message, &numbytes) == -1) {
                printf("Only sent %d bytes because of an error!\n", numbytes);
            } 
        }

        /* 
            Get confirmation back from server.
            Wait for 3 seconds receving data. If the struct is empty by the end 
            we know we haven't receive data back from sever... Return Error at that point.
        */
        status = recvtimeout(sockfd, &server_message_struct, sizeof(server_message_struct), 3, 
                NULL, NULL); // 3 second timeout

    } 
    // udp connection socket
    else if(strcmp(socktype, "udp") == 0){ 
        // Get size of all structs in storage 
        addr_size = sizeof(their_addr);

        // Send message and check for error
        if ((numbytes = sendto(sockfd, &send_message, sizeof(send_message), 0,
            p->ai_addr, p->ai_addrlen)) == -1) {
            fprintf(stderr, "Failed to send message via udp.\n");
            exit(1);
        }

        /* 
            Get confirmation back from server.
            Wait for 3 seconds receving data. If the struct is empty by the end 
            we know we haven't receive data back from sever... Return Error at that point.
        */
        status = recvtimeout(sockfd, &server_message_struct, sizeof(server_message_struct), 3, 
        (struct sockaddr *)&their_addr, &addr_size); // 3 second timeout

    }
    else{
        /* This shouldn't get here with the commmand line check...
           But safety check just in case.
        */
        fprintf(stderr, "Incorrect Socket: Please use udp or tcp...\n");
        return -1;
    }

    // Check if we had an error sending the message above in tcp or udp
    if (status == -1) {
        // error occurred
        fprintf(stderr, "Error!\n");
        return -1;
    }
    // Check if we timed out
    else if (status == -2) { 
        // timeout occurred
        fprintf(stderr, "Timeout... Check to make sure server is running on correct socket and port.\n");
        return -1;

    } 
    // We received data back. Check to make sure the struct is the correct version (1)
    if (server_message_struct.version != 1){
        fprintf(stderr, "Error: Incorrect version from server: {%d}. Please set response to 1.\n", 
                server_message_struct.version);
        return -1;
    }

    /* Message was successful!!! */

    // Close connection to server
    close(sockfd);

    // Display sent message
    printf("sent %d to server %s:%s via %s\n", data, ip, port, socktype);

    return 0;
}

void command_line_check(int argc, char *argv[], uint32_t *data, char **port, char **socktype, char **ip){
    /* 
    Read in the command line arguments and check to make sure the correct tags were passed, they are
    in the correct format, and nothing is missing. 

    params:
        argc (int): Number of command line args passed.
        argv (char *): The command line text.
        data (uint32_t *): Pointer to where we will store the -x tag data.
        port (char **): Pointer to where we will store the -p port number.
        socktype (char **): Pointer to where we will store the -t socket type (udp or tcp).
        ip (char **): Pointer to where we will store the ip/host address.

    return:
        void
    */
    // Counters to make sure each arg is called once
    int opt;
    int temp_port;
    int x = 0;
    int t = 0;
    int s = 0;
    int p = 0;

    // Loop through all given arguments in command line
    while ((opt = getopt(argc, argv, "x:t:s:p:")) != -1){
        // Check if option matches a correct tag
        switch(opt) 
            { 
                // Data tag
                case 'x': 
                    *data = atoi(optarg);
                    x++;
                    break; 

                // Socket type tag
                case 't': 
                    *socktype = optarg;

                    // Make sure it's either udp or tcp
                    if ((strcmp(*socktype, "udp") != 0) && (strcmp(*socktype, "tcp") != 0)){
                        errno = 1;
                        fprintf(stderr, "Socket type not correct. Please use only udp or tcp.\n");
                        exit(-1);
                    }

                    // Increment how many times this is in the command line
                    t++;
                    break; 

                // IP or Hostname tag
                case 's':
                    *ip = optarg;
                    s++;
                    break; 

                // Port number tag
                case 'p':
                    // Convert string to int
                    temp_port = atoi(optarg);
                    
                    // Make sure our port is within range
                    if (temp_port < 1023 || temp_port > 65535){
                        errno = 1;
                        fprintf(stderr, "PORT number is out of range.\n");
                        exit(-1);
                    }

                    // Increment how many times this is in the command line
                    p++;

                    // Set commandline as por
                    *port = optarg;
                    break; 

                // Unkown tag
                case '?': 
                    // Check for incorrect tags and exit
                    printf("unknown option: %c\n", optopt);
                    errno = 22;
                    exit(-1);
            } 
    }

    // Make sure each command-line arg was called once
    if (x != 1 || t != 1 || s != 1 || p != 1){
        printf("Incorrect Number of Argument types\n");
        errno = 22;
        exit(-1);
    }
}

int sendall(int socket, struct client_message *message, int *len)
{
    /* Loop attempting to send unsent bytes to server.

    Params:
        socket (int): socket fd we are sending on.
        message (client_message *): pointer to struct containing message data. 
        len (int *): pointer to length of remaining bytes unsent.

    Return:
        -1 (int) on failure
        0 (int) on success
    */
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;

    // Keep sending data incrementing the bytes each time
    while(total < *len) {
        n = send(socket, &message->data+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
} 

int recvtimeout(int s, struct server_message *message, int len, int timeout, struct sockaddr *addr, socklen_t *addr_len)
{
    /* Timeout for the allocated time and attempt to receive the data. 

    Params:
        s (int): Socket fd we a receiving a message on.
        message (server_message *): Message we struct to store want we receive back from server.
        len (int): length of message to recv.
        timeout (int): How many seconds to timeout the program for. 
        addr (sockaddr *): Struct containing socket address information.
        addr_len (socklen_t *): Length of socket addr storage.

    Return:
        -2 if no message is received and we timedout.
        -1 if an error occured.
        n bytes received.
    */
    fd_set fds;
    int n;
    struct timeval tv;

    // set up the file descriptor set
    FD_ZERO(&fds);
    FD_SET(s, &fds);

    // set up the struct timeval for the timeout
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    // wait until timeout or data received
    n = select(s+1, &fds, NULL, NULL, &tv);
    if (n == 0) return -2; // timeout!
    if (n == -1) return -1; // error

    // UDP will have an address 
    if (addr != NULL){
        return recvfrom(s, message, 1024-1 , 0, addr, addr_len);
    }

    // TCP is just the socket
    return recv(s, message, len, 0);
}