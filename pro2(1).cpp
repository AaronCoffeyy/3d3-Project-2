#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <memory.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <errno.h>
#include <fstream>
#include <stdlib.h>
#include <iostream>
#include <pthread.h>
#include <string.h>
#include <vector>

#define routers_num 8
//void *router_connection(void *threadID);
static int clientSockfd;
int numBytes;
int routersock = socket(AF_INET,  SOCK_DGRAM, 0);
 struct routerData {     //MY ROUTER'S data
    char src;
    //char src_ip[16];
    int port;
   
}router1;
std::string url;

int main(int argc,const char* argv[]){
url=argv[3];
int i=1;
pthread_t connection_thread;
 if(argc<3)
    {
        std::cout<<"Please enter all details\n";
        exit(0);
    }
std::cout<<"hello";
router1.src = argv[1][0];
router1.port = atoi(argv[2]);
std::cout<<"hello";
 //pthread_create(&connection_thread, NULL, router_connection, NULL);
 //pthread_cancel(connection_thread);
//std::cout<<"hello";
//pthread_join(connection_thread, NULL);
std::cout<<"\n\tConnection: Connection thread is running...";
 // int threadNum = (intptr_t) threadID;
int routerSock;
struct sockaddr_in routerAddr;
    struct sockaddr_storage senderAddr;
    char sendBuf[256], recvBuf[256];
int recvLen;

    socklen_t addrLen = sizeof(senderAddr);

if((routerSock = socket(AF_INET, SOCK_DGRAM, 0))<0)
        //std::cout<<"\n\tConnection: Thread number "<<threadNum<<" couldn't create socket. :(";
memset((char *)&routerAddr, 0, sizeof(routerAddr));         
    routerAddr.sin_family = AF_INET;
    routerAddr.sin_port = htons(router1.port);
    routerAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if(bind(routerSock, (struct sockaddr *) &routerAddr, sizeof(routerAddr)) < 0)
    {
        std::cerr<<"\n\tConnection: Router binding failed. Killing thread...";
        exit(1);
    }

for(;;) {
        recvLen = recvfrom(routerSock, recvBuf, 256, 0, (struct sockaddr *) &senderAddr, &addrLen);
        std::cout<<"\n\tConnection: Received "<<recvLen<<" bytes.";
        if(recvLen > 0) {
            recvBuf[recvLen] = 0;
            //packetParser(recvBuf);
 char sendBuf[]="hello";
//int sendlen=strlen(sendBuf);
 int sendSock = socket(AF_INET, SOCK_DGRAM, 0);

 sockaddr_in addrSend = {};    //zero-int, sin_port is 0 which picks a random port for bind
    addrSend.sin_family = AF_INET;
    addrSend.sin_port = htons(atoi(url.c_str()));
    addrSend.sin_addr.s_addr = inet_addr("127.0.0.1"); 
 bind(sendSock, (struct sockaddr *) &addrSend, sizeof(addrSend));     //Not checking failure for now..
    //    cout<<"\n\t\t BIND FAIL";    
    if((numBytes=(sendto(sendSock, sendBuf, 20, 0, (struct sockaddr *) &addrSend, 20))) == -1) {
        std::cout<<"\n\t\tSender: Couldn't send. :(";
      //  return;
    }
    //cout<<"\n\t\tSender: Message sent "<<numBytes<<" to "<<destnR->destn<<" on port "
      //  << destnR->port <<" using port: "<<sendSock<<"\n"<<"\t\tMESSAGE: "<< sendBuf; 
    close(sendSock); 
        }
    }
    close(routerSock);



return 0;
}





