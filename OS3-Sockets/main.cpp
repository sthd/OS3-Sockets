//
//  main.cpp
//  OS3-Server
//
//  Created by Elior Schneider on 20/01/2022.
//

#include <iostream>
#include <fstream>
#include <string.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <fcntl.h>      //to use creat(), open(), close()
#include <unistd.h>     // to use write()

#define DATA_SIZE 512
#define ECHOMAX 516
#define WRQ_OPCODE 2
#define ACK_OPCODE 4
#define DATA_OPCODE 3
#define HEADER 4
using namespace std;


struct wrq{
    uint16_t opcode;    //2 bytes
    string fileName;
    string transMode;
};

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
   // int legalSize = sizeof(unsigned short);
// || ((int)sizeof(argv[1]) > legalSize ) || ((int)sizeof(argv[2]) > legalSize ) || ((int)sizeof(argv[3]) > legalSize )
    if ( argc!=4){
    cout << argc << endl;
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

bool sendAck(uint16_t receivedBlock, int socket, struct sockaddr_in &ClntAddr, int ClntAddrLen){
    ack tmpAck;
    memset(&tmpAck, 0, sizeof(ack));
    tmpAck.opcode = htons(ACK_OPCODE);
    tmpAck.blockNum =htons(receivedBlock);
    if ( sendto(socket, &tmpAck, 4, 0, (struct sockaddr*) &ClntAddr, ClntAddrLen) < 0)
        return false;
    return true;
}

int main(int argc, const char * argv[]) {
    checkArguments(argc, argv);
    int select_res = 0;
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
    
    

    if( (bind(serverSocket, (struct sockaddr *) &echoServAddr , sizeof(echoServAddr)) ) < 0){
        perror("TTFTP_ERROR:"); // ? should we specifr errors? or let perror do it
        exit(1);
    }
    
    
    ssize_t recvMsgSize;
    int fdFile;
    ssize_t writtenBytes;
    uint16_t expectedBlock=0;
    uint16_t receivedBlock = 0;
    
    while(1){
        socklen_t clntSockSize= sizeof(echoClntAddr);
        recvMsgSize= recvfrom(serverSocket, echoBuffer, ECHOMAX, 0, (struct sockaddr *)&echoClntAddr, &clntSockSize);
        if (!(recvMsgSize > 0)){
            if (recvMsgSize == 0)
                continue;
            else{
                perror("TTFTP_ERROR: 1"); // ? should we specifr errors? or let perror do it
                exit(1);
            }
        }
        
        int opcode = ntohs(*(uint16_t*) &echoBuffer);
        if (opcode != WRQ_OPCODE){
            cerr << "TTFTP_ERROR: 2        " << endl; // ? should we specifr errors? or let perror do it
            continue;
        }
        string fileName = &echoBuffer[2];
        if (fileName.size() == 0){ //?
            // MARK - not given a fileName
            cerr << "TTFTP_ERROR: 3" << endl; // ? should we specifr errors? or let perror do it
            continue;
        }
        
        if (fileName[0] != '/'){
            //
            cerr << "TTFTP_ERROR: 4" << endl; // ? should we specifr errors? or let perror do it

        }
        
        string mode = &echoBuffer[2 + fileName.length() + 1];
        if (mode != octet){
            cerr << "TTFTP_ERROR: 5" << endl; // ? should we specifr errors? or let perror do it
            continue;
        }
        //char* filePath=NULL;
    //filePath = new char[fileName.length() + 1];
        //strcpy(filePath, fileName.c_str() );
    cout << "I just strcpy" << endl;
    //filePath[fileName.length()]='\0';
        //const char* filePath= "/Users/er/Mcode/OS3-Sockets/OS3-Sockets/a.txt"; //string.c_str()
        fdFile=open(fileName.c_str(), O_RDONLY);
  
        if (fdFile == -1){ // file doesn't exist :-)
            fdFile = open(fileName.c_str(), O_CREAT|O_WRONLY);
            if (fdFile == -1){
         cerr << "TTFTP_ERROR: 6" << endl;
                //cannot create file
                exit(1);
            }
        }
        else{
            // already exists!!
        cerr << "TTFTP_ERROR: 7" << endl;
        if (close(fdFile) == -1){
                    //ERROR CANT CLOSE FILE
             cerr << "TTFTP_ERROR: 5B" << endl;
                }
              exit(1);
        }

        if (sendAck(receivedBlock, serverSocket, echoClntAddr, clntSockSize) != true){
            // ERROR CANT SEND ACK
            if (close(fdFile) == -1){
                //ERROR CANT CLOSE FILE
         cerr << "TTFTP_ERROR: 8" << endl;
            }
            exit(1);
        }
        
        do{//doExternal
            do{//doInternal
                struct timeval timeVal;
                timeVal.tv_sec=timeout; // does it matter
                timeVal.tv_usec=0;
                
                fd_set writeFD;
                //int minFD = 0;
                //int maxFD = 5;
                
                FD_ZERO(&writeFD);
                FD_SET(serverSocket, &writeFD);
                
                select_res = select(serverSocket + 1, &writeFD, nullptr, nullptr, &timeVal);
                if (select_res < 0){
                    perror(" mmmmm "); //TFTP FAIL SELECT
                    if (close(fdFile) == -1){
                        //ERROR CANT CLOSE FILE
             cerr << "TTFTP_ERROR: 9" << endl;
                    }
                    exit(1);
                }
                
                if (select_res==0){
                    timeoutExpiredCount++;
                    if (sendAck(receivedBlock, serverSocket, echoClntAddr, clntSockSize) != true){
                        // ERROR CANT SEND ACK
             cerr << "TTFTP_ERROR: 10" << endl;
                        if (close(fdFile) == -1){
                            //ERROR CANT CLOSE FILE
                 cerr << "TTFTP_ERROR: 11" << endl;
                        }
                        exit(1);
                    }
                    if (timeoutExpiredCount > allowedFailures){
                        cerr << "Abandoning file transmission" << endl;
                        // @@@

                    }
                }
                
                
            }while (select_res == 0); //doInternal
            timeoutExpiredCount = 0;
            expectedBlock++;
        
            if( (recvMsgSize=recvfrom(serverSocket, echoBuffer, ECHOMAX, 0, (struct sockaddr*) &echoClntAddr, &clntSockSize)) < 0 ){
                perror("");
                if (close(fdFile) == -1){
                    //ERROR CANT CLOSE FILE
             cerr << "TTFTP_ERROR: 12" << endl;
                }
                exit(1);
            }
            
            opcode=ntohs(*(uint16_t*)echoBuffer);
            if (opcode != DATA_OPCODE){
                cerr << "Illegal TFTP operation" << endl;
            }
            
            //char opy[16]=echo[16] .. echo[31] ???
            memcpy((void*)&receivedBlock,(const void*)&echoBuffer[2], sizeof(receivedBlock) );
            if (expectedBlock != receivedBlock){
                // some error FATAL??
                cerr << " NOT THE RIGHT BLOCK" << endl;
            }
        
            if (sendAck(receivedBlock, serverSocket, echoClntAddr, clntSockSize) != true){
                // ERROR CANT SEND ACK
                if (close(fdFile) == -1){
                    //ERROR CANT CLOSE FILE
             cerr << "TTFTP_ERROR: 13" << endl;
                }
                exit(1);
            }
            
            long dataNeto = recvMsgSize - HEADER;
            if (dataNeto > 0){
                writtenBytes = write(fdFile, &echoBuffer[HEADER], dataNeto);
                if (writtenBytes != dataNeto){
                    // WRITE FALIED
             cerr << "TTFTP_ERROR: 14" << endl;
                    exit(1);
                }
            }
            
        }while (recvMsgSize==ECHOMAX); //doExternal
        // Have blocks left to be read from client (not end of transmission)
        
        if (close(fdFile) == -1){
            //ERROR CANT CLOSE FILE
         cerr << "TTFTP_ERROR: 15" << endl;
            exit(1);
        }
        //delete [] filePath;
    }//while(1)
    
    return 0;
}
