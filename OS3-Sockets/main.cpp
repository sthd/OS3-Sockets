//
//  main.cpp
//  OS3-Server
//
//  Created by Elior Schneider on 20/01/2022.
//

#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define DATA_SIZE 512
#define ECHOMAX 516
#define WRQ_OPCODE 2


using namespace std;


struct wrq{
    uint16_t opcode;    //2 bytes
    string fileName;
    string transMode;
} __attribute__((packed));

struct ack{
    uint16_t opcode;    //2 bytes
    uint16_t blockNum;  //2 bytes

} __attribute__((packed));

struct data{
    uint16_t opcode;    //2 bytes
    uint16_t blockNum;  //2 bytes
    char data[DATA_SIZE];
} __attribute__((packed));

/*
 Global variables
 */
int echoServPort;
int timeout;
int allowedFailures;
char echoBuffer[ECHOMAX];
string octet = "octet";

void checkArguments(int argc, const char * argv[]){
    bool illegal = false;
    int legalSize = sizeof(unsigned short);
    if ( argc!=3 || (sizeof(argv[1]) > legalSize ) || (sizeof(argv[2]) > legalSize ) || (sizeof(argv[3]) > legalSize ) ){
        cout << "TTFTP_ERROR: illegal argument" << endl;
        exit(1);
    }
    try {
        echoServPort =  stoi(argv[1]);
    }
    catch (...) {
        illegal=true;
     }
    
    try {
        timeout =  stoi(argv[2]);
    }
    catch (...) {
        illegal=true;
    }
    try {
        allowedFailures =  stoi(argv[3]);
    }
    catch (...) {
        illegal=true;
    }
    if ((echoServPort < 1) || (timeout < 1) || (allowedFailures < 1) )
        illegal=true;
    
    if (illegal==true){
        cout << "TTFTP_ERROR: illegal argument" << endl;
        exit(1);
    }
}


//@input: port, timeout, maxfail       >0
//
//size < unsignedshort
//
//Port > 10000
//Is port free?
//
//maxSize=1500bytes


/*
 *@
 *
 */


int main(int argc, const char * argv[]) {
    checkArguments(argc, argv);
    struct timeval timeVal;
    timeVal.tv_sec=timeout; // does it matter
    timeVal.tv_usec=0;
    int timeoutExpiredCount = 0;
    int serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverSocket <0){
        perror("TTFTP_ERROR:"); // ? should we specifr errors? or let perror do it
        exit(1);
    }
    
    
    struct sockaddr_in echoServAddr;
    struct sockaddr_in echoClntAddr; //name
    
    //struct hostent *server = gethostbyname(argv[1]);
    
    memset(&echoServAddr, 0, sizeof(echoServAddr));
    echoServAddr.sin_family = AF_INET;
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    echoServAddr.sin_port = htons(echoServPort);
    
    
    
    if( bind(serverSocket, (struct sockaddr *)&echoServAddr , sizeof(echoServAddr)) < 0){
        perror("TTFTP_ERROR:"); // ? should we specifr errors? or let perror do it
        exit(1);
    }
    
    
    ssize_t recvMsgSize;
    
    do{
        socklen_t clntSockSize= sizeof(echoClntAddr);
        recvMsgSize= recvfrom(serverSocket, echoBuffer, ECHOMAX, 0, (struct sockaddr *)&echoClntAddr, &clntSockSize);
        if (!(recvMsgSize > 0)){
            if (recvMsgSize == 0)
                continue;
            else{
                perror("TTFTP_ERROR:"); // ? should we specifr errors? or let perror do it
                exit(1);
            }
        }
        
        int opcode = ntohs(*(uint16_t*) &echoBuffer);
        if (opcode != WRQ_OPCODE){
            cerr << "TTFTP_ERROR:" << endl; // ? should we specifr errors? or let perror do it
            continue;
        }
        string fileName = &echoBuffer[2];
        if (fileName.size() == 0){
            cerr << "TTFTP_ERROR:" << endl; // ? should we specifr errors? or let perror do it
            continue;
        }
        string mode = &echoBuffer[2 + fileName.length() + 1];
        if (mode != octet){
            cerr << "TTFTP_ERROR:" << endl; // ? should we specifr errors? or let perror do it
            continue;
        }
        
        
        do{
            do{
                
                
                
                fd_set writeFD;
                int minFD = 0;
                int maxFD = 5;
                
                FD_ZERO(&writeFD);
                FD_SET(sock, &writeFD);
                int select_res = select(serverSocket + 1, &writeFD, nullptr, nullptr, &timeVal);
                // we will have for?? to run on a few c
                for (int fd= minFD; fd<maxFD; fd++){
                    if (FD_ISSET(fd, &writeFD)){
                        print("handle request");
                    }
                }
                
                
                // TODO: Wait WAIT_FOR_PACKET_TIMEOUT to see if something appears
                // for us at the socket (we are waiting for DATA)

                

                if (1){
                    // TODO: if there was something at the socket and
                    // we are here not because of a timeout
                    
                    // TODO: Read the DATA packet from the socket (at
                    //       least we hope this is a DATA packet)
                    
                    
                }

                if (1){
                    // TODO: Time out expired while waiting for data
                    //       to appear at the socket

                        timeoutExpiredCount++;
                    if (timeoutExpiredCount> allowedFailures){
                        // FATAL ERROR BAIL OUT
                            
                    }
                    else{
                        //TODO: Send another ACK for the last packet
                    }
                }
            }while (1); // TODO: Continue while some socket was ready
                        // but recvfrom failed to read the data (ret 0)
            
            if (1){
                // TODO: We got something else but DATA
                // FATAL ERROR BAIL OUT
            }
            if (1){
                // TODO: The incoming block number is not what we have
                // expected, i.e. this is a DATA pkt but the block number
                // in DATA was wrong (not last ACK’s block number + 1)
                
                // FATAL ERROR BAIL OUT
            }

        }while (0);
        timeoutExpiredCount = 0;
        //lastWriteSize = fwrite(...); // write next bulk of data
        // TODO: send ACK packet to the client
    }while (1); // Have blocks left to be read from client (not end of transmission)
    
    return 0;
}
