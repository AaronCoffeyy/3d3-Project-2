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
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>

#define routers_num 8

using namespace std;

//void *router_connection(void *threadID);
static int clientSockfd;
int numBytes;

int routersock = socket(AF_INET,  SOCK_DGRAM, 0);

 struct routerData {     //MY ROUTER'S data
    string src;
    //char src_ip[16];
    int port;
}router1;

std::string url;


string send_via_only(char buf[], int type, string destn) {
	ostringstream message;
	int send_size;
	//std::cout<<"hello";
	if (type == 1 || type == 2)// for dv
	{    //final destination of the dv
		message << "Destination: ";
		message << destn;
		message << "\r\n";

		//src of the message
		message << "Source: ";
		message << router1.src;
		message << "\r\n";

		//for port
		string port = to_string(router1.port);
		message << "Port: ";
		message << port;
		message << "\r\n";

		if (type == 1)
		{
			//type of funtion
			message << "Type: ";
			message << "DV\r\n";

			//for dv
			string dv = string(buf);
			message << "MSG: ";
			message << dv;
			message << "\r\n\r\n";
		}
		else {
			//type of funtion
			message << "Type: ";
			message << "MSG\r\n";

			//for path
			message << "Path: ";
			message << router1.src;
			message << "\r\n";

			//for dv
			string dv = string(buf);
			message << "MSG: ";
			message << dv;
			message << "\r\n\r\n";
		}
	}
	else if (type == 3)//for intermediate node
	{
		size_t end_of_message;
		string recieve_message = string(buf);
		end_of_message = recieve_message.find_first_of("\r\n\r\n");
		//just a edit to add path
		string new_message = recieve_message.substr(0, end_of_message);
		message << new_message;
		message << router1.src;
		message << "/\r\n\r\n";
	}
	else {
		std::cout << "funtion not supported";
	}

	string return_message = message.str();
	return return_message;
}

//Enter 1. router name , 2. router port, 3. URL
int main(int argc,const char* argv[]){

	pthread_t connection_thread;

	if(argc<1)
	{
		std::cout<<"Please enter all details\n";
		exit(0);
	}

	router1.src = "G";
	router1.port = 10020;							

	std::cout << "Traffic generator has been set up ";

	std::cout<<"\n\tConnection: Connection thread is running...";
	
	int routerSock;
	struct sockaddr_in routerAddr;
    struct sockaddr_storage senderAddr;
    char recvBuf[256];
	int recvLen;

    socklen_t addrLen = sizeof(senderAddr);

	// create a socket using UDP IP
	if ((routerSock = socket(AF_INET, SOCK_DGRAM, 0)) < 0);

	// bind address to socket
	memset((char *)&routerAddr, 0, sizeof(routerAddr));         
    routerAddr.sin_family = AF_INET;
    routerAddr.sin_port = htons(router1.port);
    routerAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	
    if(bind(routerSock, (struct sockaddr *) &routerAddr, sizeof(routerAddr)) < 0)
    {
        std::cerr<<"\n\tConnection: Router binding failed. Killing thread...";
        exit(1);
    }

	//Setting up delay for displaying the forwarding table
	using namespace std::this_thread;
	using namespace std::chrono;

	cout << "Preping to send" << endl;

	//message setup needs to be done 
	//Include starting port and ending port and message
	string final_dest_port,final_dest_name;
	string start_port_num, start_port_name;
	int start_port = 0;	

	//Enter the destination you want packet to start at and the end destination
	//cout << "Enter Destination port and name and starting port and name" << endl;
	//cin >> final_dest_port >> final_dest_name >> start_port_num >> start_port_name;
	//start_port = stoi(start_port_num);

	//cout << "Starting port : " << start_port_num << " Starting name: " << start_port_name << endl;
	//cout << "Dest port : " << start_port << " Dest name: " << final_dest_name << endl;

	char sendBuf[] = "hello";

	string string_to_char = send_via_only(sendBuf, 2, argv[1]);

	cout << string_to_char << endl;
	//Turning the DV into a char array to be sent to neightbours
	char *sending_array = new char[string_to_char.length() + 1];
	strcpy(sending_array, string_to_char.c_str());

	//Infinite loop that sends traffic/packets to router
	//for(;;) {		
		
		//Set up socket to send the forwading table on
		int sendSock = socket(AF_INET, SOCK_DGRAM, 0);

		//Creates a temporary socket to forwarding table to
		sockaddr_in addrSend = {};
		addrSend.sin_family = AF_INET;
		addrSend.sin_port = htons(10000);
		addrSend.sin_addr.s_addr = inet_addr("127.0.0.1");

		//Binding temp socket
		bind(sendSock, (struct sockaddr *) &addrSend, sizeof(addrSend));

		//Sending message across specidified port
		if ((numBytes = (sendto(sendSock, sending_array, 400, 0, (struct sockaddr *) &addrSend, 20))) == -1) {
			std::cout << "\n\t\tSender: Couldn't send table\n\n";
		}

		close(sendSock);

		//Adding 1 second delay to make outputs readable
		sleep_for(seconds(1));
    //}
	
    close(routerSock);

return 0;
}