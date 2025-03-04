#include <iostream>
#include <pthread.h>
#include <string>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>

struct threadData {
    std::string input;
    std::string serverIP;
    int portno;
    std::string encodedMessage;  
    std::string alphabet;        
};

void* threadInstructions(void* arg) {
    threadData* data = (threadData*)arg;

    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent* server;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "ERROR opening socket" << std::endl;
        return nullptr;
    }

    server = gethostbyname(data->serverIP.c_str());
    if (server == nullptr) {
        std::cerr << "ERROR, no such host" << std::endl;
        return nullptr;
    }

    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char*)server->h_addr, (char*)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(data->portno);

    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "ERROR connecting" << std::endl;
        return nullptr;
    }

    int msgSize = data->input.size();
    n = write(sockfd, &msgSize, sizeof(int));
    if (n < 0) {
        std::cerr << "ERROR writing to socket" << std::endl;
        close(sockfd);
        return nullptr;
    }

    n = write(sockfd, data->input.c_str(), msgSize);
    if (n < 0) {
        std::cerr << "ERROR writing to socket" << std::endl;
        close(sockfd);
        return nullptr;
    }

    n = read(sockfd, &msgSize, sizeof(int));
    if (n < 0) {
        std::cerr << "ERROR reading from socket" << std::endl;
        close(sockfd);
        return nullptr;
    }

    char* tempBuffer = new char[msgSize + 1];
    bzero(tempBuffer, msgSize + 1);
    n = read(sockfd, tempBuffer, msgSize); 
    if (n < 0) {
        std::cerr << "ERROR reading from socket" << std::endl;
        close(sockfd);
        return nullptr;
    }

    data->encodedMessage = tempBuffer;
    delete[] tempBuffer;

    n = read(sockfd, &msgSize, sizeof(int)); 
    if (n < 0) {
        std::cerr << "ERROR reading from socket" << std::endl;
        close(sockfd);
        return nullptr;
    }

    tempBuffer = new char[msgSize + 1];
    bzero(tempBuffer, msgSize + 1);
    n = read(sockfd, tempBuffer, msgSize); 
    if (n < 0) {
        std::cerr << "ERROR reading from socket" << std::endl;
        close(sockfd);
        return nullptr;
    }

    data->alphabet = tempBuffer;
    delete[] tempBuffer;

    close(sockfd);
    pthread_exit(nullptr);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "usage " << argv[0] << " hostname port" << std::endl;
        exit(0);
    }

    std::string serverIP = argv[1];
    int portno = atoi(argv[2]);

    std::vector<std::string> input;
    std::string temp;

    while (getline(std::cin, temp)) {
        if (temp.empty()) break;
        input.push_back(temp);
    }

    int n = input.size();

    threadData* data = new threadData[n];
    pthread_t* threads = new pthread_t[n];

    for (int i = 0; i < n; ++i) {
        data[i].input = input[i];
        data[i].serverIP = serverIP;
        data[i].portno = portno;
        pthread_create(&threads[i], nullptr, threadInstructions, (void*)&data[i]);
    }

    for (int i = 0; i < n; ++i) {
        pthread_join(threads[i], nullptr);
    }

    for (int i = 0; i < n; ++i) {
        std::cout << "Message: " << data[i].input << std::endl;
        std::cout << "Encoded Message: " << data[i].encodedMessage << std::endl;
        std::cout << "Alphabet: " << data[i].alphabet << std::endl;
    }

    delete[] threads;
    delete[] data;

    return 0;
}
