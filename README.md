This is the client portion of the Client/Server CSPB 3753 assignment.

How-to: 
    After running make. You can start the server by running: 
    ./server -t <socktype> -p <number>

    You can send message with the client executable
    ./client -x <32-bit unsigned int data> -t <udp/tcp> -s <ip> -p <number>

What this does:
    Client:
        This will establish a connection with the running server code on the port 
        and socket type provided. It will then send the provided data in a struct
        that has been packed to avoid sending a padded struct with extra memory.
        After ending the struct, the client waits for a response and will timeout
        if the server doesn't respond within 3 seconds. 

        The client will also return an error if the server struct version is not
        set to 1.l
    
    Server: 
        This will start a server on the socktype and port specified. The server
        will stay on listening for messages. Once a message has been received in
        the correct format, it will be processed, displayed to the terminal, and
        a response will be sent to the client letting it know the message was 
        received. The server will then return to a listening state waiting for
        the next message. 


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