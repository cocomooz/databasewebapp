#include <unistd.h>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <signal.h>
#include <sys/wait.h>

// Fireman function to handle SIGCHLD
void fireman(int)
{
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno, clilen;
    struct sockaddr_in serv_addr, cli_addr;

    // Check the commandline arguments
    if (argc != 2)
    {
        std::cerr << "Port not provided" << std::endl;
        exit(0);
    }

    // Create the socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        std::cerr << "Error opening socket" << std::endl;
        exit(0);
    }

    // Populate the sockaddr_in structure
    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    // Bind the socket with the sockaddr_in structure
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        std::cerr << "Error binding" << std::endl;
        exit(0);
    }

    // Set the max number of concurrent connections
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    // Call fireman to clean up zombie processes
    signal(SIGCHLD, fireman);

    // Keep accepting new connections
    while (true)
    {
        // Accept a new connection
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, (socklen_t *)&clilen);
        if (newsockfd < 0)
        {
            std::cerr << "Error accepting new connections" << std::endl;
            continue;
        }

        // Fork a child process to handle the client
        if (fork() == 0)
        {
            // Child process: Handle the client communication
            close(sockfd); // Child doesn't need the listening socket

            int n, msgSize = 0;
            n = read(newsockfd, &msgSize, sizeof(int));
            if (n < 0)
            {
                std::cerr << "Error reading from socket" << std::endl;
                exit(0);
            }

            char *tempBuffer = new char[msgSize + 1];
            bzero(tempBuffer, msgSize + 1);
            n = read(newsockfd, tempBuffer, msgSize + 1);
            if (n < 0)
            {
                std::cerr << "Error reading from socket" << std::endl;
                exit(0);
            }

            std::string buffer = tempBuffer;
            delete[] tempBuffer;

            // Handle the client's message
            std::cout << "Message from client: " << buffer << ", Message size: " << msgSize << std::endl;

            // Prepare and send a response
            buffer = "I got your message";
            msgSize = buffer.size();
            n = write(newsockfd, &msgSize, sizeof(int));
            if (n < 0)
            {
                std::cerr << "Error writing to socket" << std::endl;
                exit(0);
            }

            n = write(newsockfd, buffer.c_str(), msgSize);
            if (n < 0)
            {
                std::cerr << "Error writing to socket" << std::endl;
                exit(0);
            }

            close(newsockfd); // Child closes its socket
            exit(0);          // Child exits after handling client
        }
        else
        {
            // Parent process: Close the newsockfd and continue accepting connections
            close(newsockfd); // Parent doesn't need the client's connection socket
        }
    }

    close(sockfd); // Close the listening socket when the server shuts down
    return 0;
}
