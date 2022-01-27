//
//  main.cpp
//  OS3-Server
//
//  Created by Elior Schneider on 20/01/2022.
//

#include <iostream>
//#include <stdio.h>
#include <stdlib.h>
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

struct error{
    uint16_t opcode;    //2 bytes
    uint16_t ErrorCode; //2 bytes
    string errMsg;
    uint8_t zero;       //1 byte
    
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

void sendError(int err, string msgError, int serverSocket, struct sockaddr* echoClntAddr, int clntAddrLen){
    struct error errorPacket;
    errorPacket.opcode=htons(5);
    errorPacket.errMsg=msgError;
    errorPacket.ErrorCode=htons(err);
    errorPacket.zero=0;
    
    
    if ( (sendto(serverSocket, (void*)&errorPacket, sizeof(errorPacket), 0, (struct sockaddr*) &echoClntAddr, clntAddrLen)) < 0){
        perror("TTFTP_ERROR:");
        exit(1);
    }
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
    
    
    
    struct sockaddr_in echoServAddr={0};
    struct sockaddr_in echoClntAddr={0}; //name
    
    //struct hostent *server = gethostbyname(argv[1]);
    
    //memset(&echoServAddr, 0, sizeof(echoServAddr));
    echoServAddr.sin_family = AF_INET;
    echoServAddr.sin_addr.s_addr = INADDR_ANY; ///
    echoServAddr.sin_port = htons(echoServPort);
    

    if( (bind(serverSocket, (struct sockaddr *) &echoServAddr , sizeof(echoServAddr)) ) < 0){
    close(serverSocket);
    perror("TTFTP_ERROR:"); // ? should we specifr errors? or let perror do it
        exit(1);
    }
    
    ssize_t recvMsgSize;
    int fdFile;
    ssize_t writtenBytes;
    uint16_t expectedBlock=0;
    uint16_t receivedBlock = 0;
    
    //tmpAck = (struct ack*)malloc(sizeof(struct ack));
    //if (tmpAck!=NULL){
    //    cout << "NOT NULL" << endl;
    //    cout << tmpAck << endl;
    //}
    
    struct ack tmpAck;

    while(1){
        bool illegalWRQ=false;
        socklen_t clntAddrLen= sizeof(echoClntAddr);
        recvMsgSize= recvfrom(serverSocket, echoBuffer, ECHOMAX, 0, (struct sockaddr *)&echoClntAddr, &clntAddrLen);
        if (!(recvMsgSize > 0)){
            if (recvMsgSize == 0)
                continue;
            else{
                perror("TTFTP_ERROR:");
                exit(1);
            }
        }
        
        int opcode = ntohs(*(uint16_t*) &echoBuffer);
        if (opcode != WRQ_OPCODE){
            string msgError="Unknown user";
            sendError(7, msgError, serverSocket, (struct sockaddr *)&echoClntAddr, clntAddrLen);
            continue;
        }
        string fileName = &echoBuffer[2];
        if (fileName.size() == 0){ //?
            illegalWRQ=true;
        }
        
        if (fileName[0] != '/'){ /// @@@ illegal name
            illegalWRQ=true;
        }
        
        string mode = &echoBuffer[2 + fileName.length() + 1];
        if (mode != octet){
            illegalWRQ=true;
        }
        if (illegalWRQ){
            string msgError="Illegal WRQ";
            sendError(4, msgError, serverSocket, (struct sockaddr *)&echoClntAddr, clntAddrLen);
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
            string msgError="File already exists";
            sendError(6, msgError, serverSocket, (struct sockaddr *)&echoClntAddr, clntAddrLen);
            
            if (close(fdFile) == -1){
                //ERROR CANT CLOSE FILE
                perror("TTFTP_ERROR: 5B");
            }
            exit(1);
        }

    memset(&tmpAck, 0, sizeof(ack));
    tmpAck.opcode = htons(ACK_OPCODE);
    tmpAck.blockNum =htons(receivedBlock);
    cout << sizeof(tmpAck) << " is pointer size" << endl;
       if ( sendto(serverSocket, (void*)&tmpAck, sizeof(tmpAck), 0, (struct sockaddr*) &echoClntAddr, clntAddrLen) < 0){
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

             memset(&tmpAck, 0, sizeof(ack));
                tmpAck.opcode = htons(ACK_OPCODE);
                tmpAck.blockNum =htons(expectedBlock);
            cout << sizeof(tmpAck) << " is pointer size" << endl;
    
               if ( sendto(serverSocket, (void*)&tmpAck, sizeof(tmpAck), 0, (struct sockaddr*) &echoClntAddr, clntAddrLen) < 0){
                        // ERROR CANT SEND ACK
             cerr << "TTFTP_ERROR: 10" << endl;
                        if (close(fdFile) == -1){
                            //ERROR CANT CLOSE FILE
                 cerr << "TTFTP_ERROR: 11" << endl;
                        }
                        exit(1);
                    }
                    if (timeoutExpiredCount > allowedFailures){
                        string msgError="Abandoning file transmission";
                        sendError(4, msgError, serverSocket, (struct sockaddr *)&echoClntAddr, clntAddrLen);
                    }
                }
                
                
            }while (select_res == 0); //doInternal
            timeoutExpiredCount = 0;
            expectedBlock++;
        
            if( (recvMsgSize=recvfrom(serverSocket, echoBuffer, ECHOMAX, 0, (struct sockaddr*) &echoClntAddr, &clntAddrLen)) < 0 ){
                perror("");
                if (close(fdFile) == -1){
                    //ERROR CANT CLOSE FILE
             cerr << "TTFTP_ERROR: 12" << endl;
                }
                exit(1);
            }
            
            opcode=ntohs(*(uint16_t*)echoBuffer);
            if (opcode != DATA_OPCODE){
                string msgError;
                if (opcode== WRQ_OPCODE)
                    msgError="Unexpected packet";
                else
                    msgError="Illegal TFTP operation";
                sendError(4, msgError, serverSocket, (struct sockaddr *)&echoClntAddr, clntAddrLen);
            }
            
            //char opy[16]=echo[16] .. echo[31] ???
            memcpy((void*)&receivedBlock,(const void*)&echoBuffer[2], sizeof(receivedBlock) );
            receivedBlock=ntohs(receivedBlock);
            if (expectedBlock != receivedBlock){
                string msgError="Bad block number";
                sendError(0, msgError, serverSocket, (struct sockaddr *)&echoClntAddr, clntAddrLen);
                exit(1);
            }
        
            memset(&tmpAck, 0, sizeof(ack));
            tmpAck.opcode = htons(ACK_OPCODE);
            tmpAck.blockNum =htons(expectedBlock);
            cout << sizeof(tmpAck) << " is pointer size" << endl;
    
               if ( sendto(serverSocket, (void*)&tmpAck, sizeof(tmpAck), 0, (struct sockaddr*) &echoClntAddr, clntAddrLen) < 0){
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
//    free(tmpAck);
    return 0;
}
