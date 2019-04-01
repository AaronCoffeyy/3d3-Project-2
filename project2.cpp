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

//Neighbour struct with holds the port number, hop distance and if they are connected
struct Neighbour {
	int port;
	int hop_distance;
	bool connected;
	string port_name;
	int via_port;
	string via_port_name;    
	Neighbour *next_node;
	int total_dist;
};

int routersock = socket(AF_INET,  SOCK_DGRAM, 0);

 struct routerData {     //MY ROUTER'S data
    string src;
    //char src_ip[16];
    int port;
	Neighbour *head;
}router1;

 struct edge {
	 string start,end;
	 int weight;
};

 struct graph {
 int total_number_of_vertices,total_edges;
	 vector<struct edge*> edges;
};

 struct graph* graph_of_edges;

std::string url;

void Neighbour_setup(routerData *host, string source, string dest, string port_num, string weight) {

	Neighbour *temp;

	//Setting up new neighbour
	Neighbour *New_Neighbour = new Neighbour;
	New_Neighbour->port_name = dest;
	New_Neighbour->next_node = NULL;
	New_Neighbour->port = stoi(port_num);
	New_Neighbour->hop_distance = stoi(weight);
	New_Neighbour->total_dist = stoi(weight);
	New_Neighbour->via_port = stoi(port_num);
	New_Neighbour->via_port_name = dest;

	//Checks to see if this is first neighbour
	//If so sets it as start of the list
	if (host->head == NULL) {
		host->head = New_Neighbour;
	}
	else {
		//If it isnt the first neighbour it goes through the list until it finds the 
		//end then adds it to the end

		temp = host->head;

		while (temp->next_node != NULL) {
			temp = temp->next_node;
		}

		temp->next_node = New_Neighbour;
	}
}

void initial_graph() {
	graph_of_edges = new graph;
	graph_of_edges->total_number_of_vertices=0;
	graph_of_edges->total_edges=0;
	graph_of_edges->edges.resize(0);
}

void add_Link(string start,string end, string weight) { 

	edge *new_edge = new edge;
	struct graph *g;
	g=graph_of_edges;
int w=stoi(weight);

	new_edge->start = start;
	new_edge->end = end;
	new_edge->weight = stoi(weight);

	char x=start.at(0);
	char y=end.at(0);
int flag=0;                          //coded by lakshi

	int size = g->edges.size();
		 if(g->total_number_of_vertices == 0){
        g->total_edges++;
        g->total_number_of_vertices+=2;
        g->edges.push_back(new_edge);
        cout<<"\n\t\t\tGRAPHER: Added First Edge: \t"<<g->edges[0]->start<<"->"<<g->edges[0]->end<<" W: "<<g->edges[0]->weight;
    }
    else{
        for(int i = 0; i<g->edges.size(); i++){     //Check if given edge already exists in graph
            if(g->edges[i]->start.at(0) == x) {
                if(g->edges[i]->end.at(0) == y) {
                    cout<<"\n\t\t\tGRAPHER: Edge already exists."<<endl;
                    flag = 1;
                    if(g->edges[i]->weight != w) {
                        cout<<"\n\t\t\tGRAPHER: Updating weight..."<<endl;
                        g->edges[i]->weight = w;
                        flag = 1;
                    }
                }             
            }
        }
        if(flag == 0) {     //Adding edge if it doesn't already exist in the graph
            int index, flag1 = 1, flag2= 1;            
            g->total_edges++;
            //g->V+=2;
           
            for(int i = 0; i<g->edges.size(); i++)
            {
                if(g->edges[i]->start.at(0) == x || g->edges[i]->end.at(0) == x)
                    flag1 = 0;
                if(g->edges[i]->start.at(0) == y || g->edges[i]->end.at(0) == y)
                    flag2 = 0;
            }                
            g->total_number_of_vertices = g->total_number_of_vertices + flag1 + flag2;
		g->edges.push_back(new_edge);
            index = g->edges.size()-1;
            cout<<"\n\t\t\tGRAPHER: Inserted new edge***: \t"<<g->edges[index]->start
            <<"->"<<g->edges[index]->end<<" W: "<<g->edges[index]->weight;
        }
    }
   

	//Need to add in check to see if edge already exists 
	/*
	for (int x = 0; x < size; x++) {
		cout << "edge " << x << " weight " << graph_of_edges->edges[x]->ending_node << endl;
	}*/
}//lakshi's code ends here

//Setting up connection from file
void Initial_forwarding_table(routerData *init_router) {

	ifstream nodefile("ff.txt");

	char temp;
	int pos = 0;
	ostringstream char_to_string;
	bool next = false;
	string source,neighbour, port_num, weight;

	//Reads in from file until EOF
	while (nodefile >> temp) {

		string temp1;

		//When a coma is found the stringstream will be outputted to the corrseponding string and the 
		//reset
		if (temp == ',') {
			next = true;
		}

		if (next == true) {

			next = false;
			temp1 = char_to_string.str();

			//Switch function to swap between what the stringstream will be let equal to
			switch (pos) {
			case 0:
				source = temp1;
				pos = 1;
				goto switch_end;
			case 1:
				neighbour = temp1;
				pos = 2;
				goto switch_end;
			case 2:
				port_num = temp1;
				pos = 3;
				goto switch_end;
			case 3:
				weight = temp1;

				//Add all edges to the grpah for the DV and bellmond ford calculations - shortets path
				add_Link(source, neighbour, weight);


				//If the source read in from file matche our router we set up the neighbour 
				if (init_router->src == source) {
					Neighbour_setup(init_router, source, neighbour, port_num, weight);
					
				}

				pos = 0;
				goto switch_end;
			}
			break;
		switch_end:
			char_to_string.str("");
		}
		else {
			char_to_string << temp;
		}
	}
}

//Prints out neighbour of the router
void print_neighbours(routerData print_router) {

	cout << endl;
	cout << "Router " << print_router.src << endl;

	cout << setw(15) << left << "Destination";
	cout << setw(5) << left << "Cost";
	cout << setw(20) << left << "Outgoing UDP Port";
	cout << setw(20) << left << "Destination UDP port" << endl;
	for (Neighbour *pointer = print_router.head; pointer != NULL; pointer = pointer->next_node) {
		cout << setw(15) << left << pointer->port_name;
		cout << setw(5) << left << pointer->total_dist;
		cout << left << pointer->via_port << " (Node " << pointer->via_port_name << ")" << setw(6) << " ";
		cout << left << pointer->port << left<< " (Node " << pointer->port_name << ")" << endl;
	}
}

//Edit so it reads in DV not full table - Aaron
void Reading_in_forwardingtable(char table[]) {

	int pos = 0;
	ostringstream char_to_string;
	string temp1;

	string port_name,source,source_port;
	string port_num,hop_distance,total_dist;

	int i = 0;
	
	while (table[i] != '\0' ) {
		
		if (table[i] == ',') {
			temp1 = char_to_string.str();
			switch (pos) {
			case 0:
				source = temp1;
				pos = 1;
				goto switch_end;
			case 1:
				source_port = temp1;
				pos = 2;
				goto switch_end;
			case 2:
				port_name = temp1;
				pos = 3;
				goto switch_end;
			case 3:
				port_num = temp1;
				pos = 4;
				goto switch_end;
			case 4:
				hop_distance = temp1;
				pos = 5;
				goto switch_end;
			case 5:
				total_dist = temp1;				
				
				if (port_name != router1.src) {

					Neighbour *temp;
					bool found = false;

					if (router1.head == NULL) {
						break;
					}
					else {
						temp = router1.head;

						while (temp->next_node != NULL) {
							if (temp->port_name == port_name) {
								found = true;
							}
						
							temp = temp->next_node;
						}

						if (temp->port_name == port_name) {
							found = true;
						}

						if (!found) {
							//Setting up new neighbour via another node when sent a DV
							Neighbour *New_Neighbour = new Neighbour;
							New_Neighbour->port_name = port_name;
							New_Neighbour->next_node = NULL;
							New_Neighbour->port = stoi(port_num);
							New_Neighbour->hop_distance = stoi(hop_distance);
							New_Neighbour->via_port = stoi(source_port);
							New_Neighbour->via_port_name = source;
							
							//New_Neighbour->total_dist = stoi(total_dist) + New_Neighbour->hop_distance;

							temp->next_node = New_Neighbour;
						}

					}
				}			
				
				pos = 0;
				goto switch_end;
			}
		switch_end:
			char_to_string.str("");
		}
		else {
			char_to_string << table[i];
		}
		
		i++;
	}
}




//Enter 1. router name , 2. router port, 3. URL
int main(int argc,const char* argv[]){

	pthread_t connection_thread;
	initial_graph();

	if(argc<3)
	{
		std::cout<<"Please enter all details\n";
		exit(0);
	}

	router1.src = argv[1][0];
	router1.port = atoi(argv[2]);
	//url = argv[3];								

	std::cout << "Router has been set up ";

	std::cout<<"\n\tConnection: Connection thread is running...";
	 // int threadNum = (intptr_t) threadID;
	int routerSock;
	struct sockaddr_in routerAddr;
    struct sockaddr_storage senderAddr;
    char sendBuf[256], recvBuf[256];
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

	//No accepting or listening on UDP connection
	//Setting up neighbours
	router1.head = NULL;
	Initial_forwarding_table(&router1);
	print_neighbours(router1);
std::cout<<"total number of edges are:";
//std::cout<<graph_of_edges->total_number_of_vertices<< endl;
BellmanFord(graph_of_edges,router1.src.at(0));

	//A check to see if the forwarding table has stabilized
	bool stabilized = false;

	//Loop that will send and receive forwarding tabel until it stabilizes

	//Setting up delay for displaying the forwarding table
	using namespace std::this_thread;
	using namespace std::chrono;

	while (!stabilized) {
		
		Neighbour *temp;

		if (router1.head == NULL) {
			cout << "No neighbours found" << endl;
			break;
		}
		else {
			temp = router1.head;

			while (temp != NULL) {

				//Setting next port
				int temp_port = temp->port;
				
				//Sending table
				cout << "Sending DV " << endl;

				//making char array to send 
				ostringstream send_array;
				
				Neighbour *forwadring_table_send = router1.head;

				//Send DV to all neighbours
				while (forwadring_table_send != NULL) {
					
					//send_array << ;
					forwadring_table_send = forwadring_table_send->next_node;
				}
				string string_to_char = send_array.str();
		
				//Turning the DV into a char array to be sent to neightbours
				char *sending_array = new char[string_to_char.length() + 1];
				strcpy(sending_array,string_to_char.c_str());

				//Set up socket to send the forwading table on
				int Fowardtable_sock = socket(AF_INET, SOCK_DGRAM, 0);

				//Creates a temporary socket to forwarding table to
				sockaddr_in addrSend = {};
				addrSend.sin_family = AF_INET;
				addrSend.sin_port = htons(temp_port);
				addrSend.sin_addr.s_addr = inet_addr("127.0.0.1");

				//Binding temp socket
				bind(Fowardtable_sock, (struct sockaddr *) &addrSend, sizeof(addrSend));

				if ((numBytes = (sendto(Fowardtable_sock, sending_array, 400, 0, (struct sockaddr *) &addrSend, 20))) == -1) {
					std::cout << "\n\t\tSender: Couldn't send table\n\n";
				}

				temp = temp->next_node;

				sleep_for(seconds(1));

				close(Fowardtable_sock);
			}
		}

	
		cout << "Waiting to receive" << endl;

		recvLen = recvfrom(routerSock, recvBuf, 256, 0, (struct sockaddr *) &senderAddr, &addrLen);

		std::cout << "\n\tConnection: Received " << recvLen << " bytes.";
		cout << recvBuf << endl;


		//Reading_in_forwardingtable(recvBuf);

		//print_neighbours(router1);

	}

	
	cout << "Enters loop" << endl;
	string destination_chosen;
	int temp_port = 0;
	//Infinite loop that sends and is open to recieve packets
	for(;;) {

		cout << "Preping receive" << endl;
		//Open to receive
		cout << "Enter dest" << endl;
		getline(cin, destination_chosen);

		if (destination_chosen == router1.src) {			
		}
		else {
			
			Neighbour *temp;

			if (router1.head == NULL) {
				return 0;
			}
			else {
				temp = router1.head;

				while (temp->next_node != NULL) {
					if (temp->port_name == destination_chosen) {
						temp_port = temp->port;
					}
					temp = temp->next_node;
				}
			}

			char sendBuf[] = "hello";
			int sendSock = socket(AF_INET, SOCK_DGRAM, 0);
			//Creates a temporary socket to send packets to
			sockaddr_in addrSend = {};    //zero-int, sin_port is 0 which picks a random port for bind
			addrSend.sin_family = AF_INET;
			addrSend.sin_port = htons(temp_port);
			addrSend.sin_addr.s_addr = inet_addr("127.0.0.1");

			//Binding temp socket
			bind(sendSock, (struct sockaddr *) &addrSend, sizeof(addrSend));     //Not checking failure for now..
			//    cout<<"\n\t\t BIND FAIL";    
			if ((numBytes = (sendto(sendSock, sendBuf, 20, 0, (struct sockaddr *) &addrSend, 20))) == -1) {
				std::cout << "\n\t\tSender: Couldn't send. :(";
				//  return;
			}
			//cout<<"\n\t\tSender: Message sent "<<numBytes<<" to "<<destnR->destn<<" on port "
			  //  << destnR->port <<" using port: "<<sendSock<<"\n"<<"\t\tMESSAGE: "<< sendBuf; 
			close(sendSock);
		}

		recvLen = recvfrom(routerSock, recvBuf, 256, 0, (struct sockaddr *) &senderAddr, &addrLen); 

        std::cout<<"\n\tConnection: Received "<<recvLen<<" bytes.";
		cout << recvBuf << endl;
        if(recvLen > 0) {

			recvBuf[recvLen] = 0;
			char sendBuf[]="hello";
			int sendSock = socket(AF_INET, SOCK_DGRAM, 0);

			//Creates a temporary socket to send packets to
			sockaddr_in addrSend = {};    //zero-int, sin_port is 0 which picks a random port for bind
			addrSend.sin_family = AF_INET;
			addrSend.sin_port = htons(atoi(url.c_str()));
			addrSend.sin_addr.s_addr = inet_addr("127.0.0.1"); 
	
			//Binding temp socket
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






   

