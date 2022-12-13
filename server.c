
/* Written by Nathan Hutchins (nahu7321@colorado.edu)

This is the server portion of the Client/Server CSPB 3753 assignment.

How-to: 
    After running make. You can receive messages with the server executable
    ./server -t <socktype> -p <number>

What this does:
    This will start a server on the socktype and port specified. The server
    will stay on listening for messages. Once a message has been received in
    the correct format, it will be processed, displayed to the terminal, and
    a response will be sent to the client letting it know the message was 
    received. The server will then return to a listening state waiting for
    the next message. 

What wasn't completed:
    n/a - I believe everything required for the sever portion was completed.

Reference:
    Thank you to the beej guide for the very detailed walkthrough of 
    client server communication and Network Programming.
    https://beej.us/guide/bgnet/html/

*/
#include "server.h"

int main(int argc, char *argv[]){
    // Check that the correct number of arguments were given 
    if (argc != 5){
        // Set error to invalid arg
        errno = 22;
        fprintf(stderr,"Incorrect number of arguments.\n");
        return -1;
    }

    // Filter each command line arg and make sure they are correct
    char *socktype;
    char *port;

    // Make sure the command line is correct and populate the needed data
    command_line_check(argc, argv, &port, &socktype);

    /* Start server and listen */
    // A storage structure to hold our IPv4 strucutres
    struct sockaddr_storage their_addr;
    socklen_t addr_size;

    // Prep socket address structures for holding needed socket flags/information   
    struct addrinfo start_socket_addr, *res, *p;

    // Prep variables to hold socket information, bytes returned, and status checking
    int sockfd, new_fd;
    int numbytes;
    int status;

    /* 
        Custom structures packed to hold a 1 byte version number in both, and a 4 byte 
        data message in the client_message strucutre.
        See packing details in server.h and reference to packing logic.
    */
    struct client_message client_response_struct;
    struct server_message server_send_struct;
    uint32_t data;

    // Fill socket addr with 0s
    memset(&start_socket_addr, 0, sizeof(start_socket_addr));

    // Set to IPv4
    start_socket_addr.ai_family = AF_INET;  

    // Set socktype depending on udp or tcp
    if (strcmp(socktype, "udp") == 0){
        start_socket_addr.ai_socktype = SOCK_DGRAM;
    }
    else if (strcmp(socktype, "tcp") == 0){
        start_socket_addr.ai_socktype = SOCK_STREAM;
    }

    // Set to auto IP (current IP on machine)
    start_socket_addr.ai_flags = AI_PASSIVE;    


    // Setup struct - this includes the DNS and service name lookups
    // Also gives pointer to linked-list of results (pointer will be res parameter).
    if ((status = getaddrinfo(NULL, port, &start_socket_addr, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }

    // make a socket, bind it, and listen on it

    // loop through all the results and bind to the first we can
    for(p = res; p != NULL; p = p->ai_next) {
        // Attempt to make a socket using the first result viable
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            fprintf(stderr,"listener: socket");
            continue;
        }

        // Attempt to bind to this socket
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            fprintf(stderr,"Failed to bind to socket.. trying next result.\n");
            continue;
        }
        break;
    }

    // Free linked list and accept an incoming connection:
    freeaddrinfo(res);

    printf("Starting %s server on port: %s...\n", socktype, port);
    while (1){
        // Get size of all structs in storage 
        addr_size = sizeof(their_addr);
        // Check what socket we are listening on
        if (strcmp(socktype, "tcp") == 0){
            // Prep connection on socket with only 1 request in the queue at a time
            status = listen(sockfd, 1);
            if (status == -1){
                fprintf(stderr, "Error seting up accept connection on socket.\n");
                return -1;
            }

            // Accept a message by awaiting a connection on a socket (this is was "waits" for a message)
            new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
            if (new_fd == -1){
                fprintf(stderr, "Error seting up connection on socket.\n");
                return -1;
            }

            // Read the bytes from the message and store into client response message
            status = recv(new_fd, &client_response_struct, sizeof(client_response_struct), 0);
            if (status == -1){
                fprintf(stderr, "Error receiving message from client.\n");
                return -1;
            }

            // Check that the version of the message is correct
            if (client_response_struct.version != 1){
                fprintf(stderr, "Incorrect message version.. please set it to 1.\n");
                return -1;
            }

            // Decode and Display message to terminal
            data = ntohl(client_response_struct.data);
            printf("the sent number is: %d\n", data);

            // Send message back and check for error
            server_send_struct.version = 1;
            if ((numbytes = send(new_fd, &server_send_struct, sizeof(server_send_struct), 0)) == -1){
                fprintf(stderr,"Failed to send back to client.\n");
                exit(1);
            } 

            // check if the one byte was sent 
            numbytes = sizeof(server_send_struct) - numbytes;

            // If it does fail, try send again since it's only one byte
            if (numbytes > 0){
                numbytes = send(new_fd, &server_send_struct, sizeof(server_send_struct), 0);
                // If fails a second time, return error.
                if (numbytes <= 0) {
                    fprintf(stderr,"Error sending confirmation struct back to client\n");
                    return -1;
                } 
            }
            
        }
        else if (strcmp(socktype, "udp") == 0){
            // Accept any message coming in on socket (UDP doesn't need to prep listen on socket)
            // Receives faster but possible of data loss
            new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);

            // Receive message coming in and write it to the client message struct
            if ((numbytes = recvfrom(sockfd, &client_response_struct, sizeof(client_response_struct), 0,
                (struct sockaddr *)&their_addr, &addr_size)) == -1) {
                fprintf(stderr,"recvfrom");
                exit(1);
            }

            // Make sure the version is correct
            if (client_response_struct.version != 1){
                fprintf(stderr, "Error: Incorrect client version number. Please set to 1.\n");
                return -1;
            }

            // Display message to terminal
            data = ntohl(client_response_struct.data);
            printf("the sent number is: %d\n", data);

            // Send received message back to server 
            server_send_struct.version = 1;
            status = sendto(sockfd, &server_send_struct, sizeof(server_send_struct), 0,
            (struct sockaddr *)&their_addr, addr_size);

            // Check if failed to send the one byte
            if (status <= 0){
                // Try sending again
                status = sendto(sockfd, &server_send_struct, sizeof(server_send_struct), 0,
                    (struct sockaddr *)&their_addr, addr_size);

                // If still failed, return error
                if (status <= 0){
                    fprintf(stderr, "Error sending confirmation message back to client.\n");
                    return -1;
                }
            }
        }
        else{
            // This should never happen, should be caught by command line function
            fprintf(stderr, "Not correct socket type. Please user udp or tcp...\n");
        }
    }
    // Close connection and end server
    close(new_fd);
    printf("Closed client connection\n");

    return 0;
}

void command_line_check(int argc, char *argv[], char **port, char **socktype){
    /* 
    Read in the command line arguments and check to make sure the correct tags were passed, they are
    in the correct format, and nothing is missing. 

    params:
        argc (int): Number of command line args passed.
        argv (char *): The command line text.
        port (char **): Pointer to where we will store the -p port number.
        socktype (char **): Pointer to where we will store the -t socket type (udp or tcp).

    return:
        void
    */
    // Make sure each arg is called once
    int t = 0;
    int p = 0;
    int opt;

    // Loop through all given arguments in command line
    while ((opt = getopt(argc, argv, "t:p:")) != -1){
        switch(opt) 
            { 
                case 't': // udp or tcp
                    *socktype = optarg;

                    // Make sure it's either udp or tcp
                    if ((strcmp(*socktype, "udp") != 0) && (strcmp(*socktype, "tcp") != 0)){
                        errno = 1;
                        fprintf(stderr,"Socket type not correct. Please use only udp or tcp.");
                        exit(-1);
                    }

                    // Increment how many times this is in the command line
                    t++;
                    break; 

                case 'p': // port number
                    // Save string to port
                    *port = optarg;

                    // Convert string to int
                    int port_check = atoi(optarg);
                    // Make sure our port is within range
                    if (port_check < 1023 || port_check > 65535){
                        errno = 1;
                        fprintf(stderr,"PORT number is out of range");
                        exit(-1);
                    }

                    // Increment how many times this is in the command line
                    p++;
                    break; 

                case '?': // unknown
                    // Check for incorrect tags and exit
                    errno = 22;
                    exit(-1);
            } 
    }

    // Make sure each command-line arg was called once
    if (t != 1 || p != 1){
        printf("Incorrect Number of Argument types\n");
        errno = 22;
        exit(-1);
    }
}