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
static int sendSocket;
int send_size;

//Neighbour struct with holds the port number, hop distance and if they are connected
struct Neighbour {
	int port;
	int hop_distance;
	bool connected;
	bool first;
	string port_name;
	int via_port;
	string via_port_name;
	Neighbour *next_node;
	int total_dist;
};

int routersock = socket(AF_INET, SOCK_DGRAM, 0);

struct routerData {     //MY ROUTER'S data
	string src;
	//char src_ip[16];
	int port;
	Neighbour *head;
}router1;

struct edge {
	string start, end;
	int weight;
};

struct graph {
	int total_number_of_vertices, total_edges;
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
	New_Neighbour->first=true;
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
	graph_of_edges->total_number_of_vertices = 0;
	graph_of_edges->total_edges = 0;
	graph_of_edges->edges.resize(0);
}

void add_Link(string start, string end, string weight) {

	edge *new_edge = new edge;
	struct graph *g;
	g = graph_of_edges;
	int w = stoi(weight);

	new_edge->start = start;
	new_edge->end = end;
	new_edge->weight = stoi(weight);

	char x = start.at(0);
	char y = end.at(0);
	int flag = 0;

	if (g->total_number_of_vertices == 0) {
		g->total_edges++;
		g->total_number_of_vertices += 2;
		g->edges.push_back(new_edge);
		cout << "\n\t\t\tGRAPHER: Added First Edge: \t" << g->edges[0]->start << "->" << g->edges[0]->end << " W: " << g->edges[0]->weight;
	}
	else {
		for (int i = 0; i < g->edges.size(); i++) {     //Check if given edge already exists in graph
			if (g->edges[i]->start.at(0) == x) {
				if (g->edges[i]->end.at(0) == y) {
					cout << "\n\t\t\tGRAPHER: Edge already exists." << endl;
					flag = 1;
					if (g->edges[i]->weight != w) {
						cout << "\n\t\t\tGRAPHER: Updating weight..." << endl;
						g->edges[i]->weight = w;
						flag = 1;
					}
				}
			}
		}
		if (flag == 0) {     //Adding edge if it doesn't already exist in the graph
			int index, flag1 = 1, flag2 = 1;
			g->total_edges++;
			//g->V+=2;

			for (int i = 0; i < g->edges.size(); i++)
			{
				if (g->edges[i]->start.at(0) == x || g->edges[i]->end.at(0) == x)
					flag1 = 0;
				if (g->edges[i]->start.at(0) == y || g->edges[i]->end.at(0) == y)
					flag2 = 0;
			}
			g->total_number_of_vertices = g->total_number_of_vertices + flag1 + flag2;
			g->edges.push_back(new_edge);
			index = g->edges.size() - 1;
			cout << "\n\t\t\tGRAPHER: Inserted new edge***: \t" << g->edges[index]->start
				<< "->" << g->edges[index]->end << " W: " << g->edges[index]->weight;
		}
	}


	//Need to add in check to see if edge already exists 
	/*
	for (int x = 0; x < size; x++) {
		cout << "edge " << x << " weight " << graph_of_edges->edges[x]->ending_node << endl;
	}*/
}

//Setting up connection from file
void Initial_forwarding_table(routerData *init_router) {

	ifstream nodefile("ff.txt");

	char temp;
	int pos = 0;
	ostringstream char_to_string;
	bool next = false;
	string source, neighbour, port_num, weight;

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

				//If the source read in from file matche our router we set up the neighbour 
				if (init_router->src == source) {
					Neighbour_setup(init_router, source, neighbour, port_num, weight);

//Add all edges to the grpah for the DV and bellmond ford calculations - shortest path
add_Link(source, neighbour, weight);
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
		cout << left << pointer->port << left << " (Node " << pointer->port_name << ")" << endl;
	}
}


bool BellmanFord(struct graph* g, int src)
{
	//std::cout<<"\nhello\n";
	int V = g->total_number_of_vertices;
	int E = g->total_edges;
	char node[V];
	int shortestdist[V];
	char nextnode[V];

	Neighbour *checker;
	int pos = 0;
	int iterator = 0;

	// Step 1: Initialize distances from src to all other vertices
	// as INFINITE that have been discouvered

	while(iterator < V - 1 )
	{			
		checker = router1.head;
		bool discovered = false;

		while (checker != NULL) {

			if (checker->port_name.at(0) == (char)(pos + 65)) {
				discovered = true;	
				break;
			}
			checker = checker->next_node;
		}
		
		if (discovered == true) {
			node[pos] = (char)(pos + 65);
			shortestdist[pos] = 10000;
			nextnode[pos] = -1;	
			pos++;
			iterator++;
		}
		else {
			pos++;			
		}		
	}
	//Assuming A is first vertex and so on...
	shortestdist[src % 65] = 0;

	// Step 2: Relax all edges |V| - 1 times.
	for (int i = 1; i <= V - 1; i++)
	{
		for (int j = 0; j < E; j++)
		{
			int u = g->edges[j]->start.at(0);
			int v = g->edges[j]->end.at(0);
			int weight = g->edges[j]->weight;
			
			if (shortestdist[u % 65] != 10000 && shortestdist[u % 65] + weight < shortestdist[v % 65]) {
				shortestdist[v % 65] = shortestdist[u % 65] + weight;
				//cout << "Checker" << endl;
				//std::cout << "to get to node " << (char)nextnode[v % 65] << " go through " << (char)node[u % 65] << endl;
				//If there is no link node between currernt and destiantion then destiantion is next node

				if (node[u % 65] == node[src % 65]) {
					//std::cout << "\nto get to node " << (char)nextnode[v % 65] << " go through " << (char)node[v % 65] << endl;
					nextnode[v % 65] = (char)node[v % 65];
				}				
				else {
					//cout << "second" << endl;
					//std::cout << "\nto get to node " << (char)nextnode[v % 65] << " go through " << (char)node[u % 65] << endl;
					nextnode[v % 65] = (char)node[u % 65];					
				}			
			}
		}
	}

	pos = 0;
	iterator = 0;
	char my_node,my_nodes_next_node;

	//this part changes all the nextnode values to negihbour only
	//i.e to bet to D you go to F so this funcitn swaps F with B because B
	//Is the closest router to F
	while (iterator < V - 1)
	{
		checker = router1.head;
		bool discovered = false;

		while (checker != NULL) {

			if (checker->port_name.at(0) == (char)(pos + 65)) {
				my_node = node[pos];
				my_nodes_next_node = nextnode[pos];
				discovered = true;
				break;
			}
			checker = checker->next_node;
		}

		if (discovered == true) {

			int inner_iterator = 0;
			int inner_pos = 0;
			Neighbour *inner_checker;

			//Inner Loop
			while (inner_iterator < V - 1) {
				inner_checker = router1.head;
				bool inner_discovered = false;

				while (inner_checker != NULL) {

					if (inner_checker->port_name.at(0) == (char)(inner_pos + 65)) {
						inner_discovered = true;
						break;
					}
					inner_checker = inner_checker->next_node;
				}

				if (inner_discovered == true) {
					if (nextnode[inner_pos] == my_node) {
						//cout << "change " << nextnode[inner_pos] << " to " << my_nodes_next_node << endl;
						nextnode[inner_pos] = my_nodes_next_node;
					}
					inner_pos++;
					inner_iterator++;
				}
				else {
					inner_pos++;
				}
			}

			pos++;
			iterator++;
		}
		else {
			pos++;
		}
	}

	int i = 0;
	Neighbour *temp;
	temp = router1.head;
	while (temp != NULL) {
		temp->via_port_name.at(0) = nextnode[temp->port_name.at(0) % 65];
		temp->total_dist = shortestdist[temp->port_name.at(0) % 65];
		temp->via_port = (nextnode[temp->port_name.at(0) % 65] % 65) + 10000;
		std::cout << "\nto get to node " << temp->port_name.at(0) << " go through " << temp->via_port_name.at(0) << " in " << temp->total_dist << "\n";
		temp = temp->next_node;
	}

	std::cout << "\nRouting table regenerated.\n";
	std::cout << "\n----------------------------------------------------------------------------------\n";
}

//Adding new "Neighbour" to graph via receiving a DV
void Add_destination(string port_name,string hop_distance,string source) {

	Neighbour *temp;
	int via_port = 0;
	int previous_cost = 0;
	bool found = false;

	if (router1.head == NULL) {
		return;
	}
	else {
		temp = router1.head;

		//Checking to see if destination already exists on forwarding table
		while (temp->next_node != NULL) {
			if (temp->port_name == port_name) {
				previous_cost = temp->hop_distance;
				found = true;
			}
			else if(temp->port_name == source) {
				via_port = temp->port;
			}
			
			temp = temp->next_node;
		}

		if (temp->port_name == port_name) {
			found = true;
			previous_cost = temp->hop_distance;
		}
		else if (temp->port_name == source) {
			via_port = temp->port;
		}

		if (!found) {
			//Setting up new neighbour via another node when sent a DV
			Neighbour *New_Neighbour = new Neighbour;
			New_Neighbour->port_name = port_name;
			New_Neighbour->next_node = NULL;
			//New_Neighbour->port = stoi(port_num);
			New_Neighbour->hop_distance = stoi(hop_distance);
			New_Neighbour->via_port = via_port;
			New_Neighbour->via_port_name = source;
			New_Neighbour->first = false;

			New_Neighbour->total_dist = New_Neighbour->hop_distance;

			temp->next_node = New_Neighbour;

			cout << "New destinaiton added" << endl;
			
			add_Link(source,port_name,hop_distance);
			
		}
		else {
			cout << "Found already " << source << " " << port_name  << endl;
			
			add_Link(source, port_name, hop_distance);
			BellmanFord(graph_of_edges, router1.src.at(0));
		}
	}
}

//takes in DV message and builds the forwarding table from that
void Reading_in_DV(char table[]) {

	int pos = 0;
	ostringstream char_to_string;
	string temp1;

	string protocol, source, destination, cost;

	string receice_message = string(table);
	size_t first, second, end_of_message;
	string tempstring;

	//finding end of message
	end_of_message = receice_message.find_first_of("\r\n\r\n");

	while (receice_message != "\r\n"){
		//setting Protocol
		//first = receice_message.find_first_of("/");
		protocol = receice_message.substr(0, 2);
		//cout << "Protocol: " << protocol << endl;

		//setting destination
		//tempstring = receice_message.substr(first + 1, receice_message.size() - (first + 1));
		//second = tempstring.find_first_of("/");
		destination = receice_message.substr(3,1);
		//cout << "Dest: " << destination << endl;

		//setting source
	//	first += second + 2;
	//	tempstring = receice_message.substr(first, receice_message.size() - (first));	
	//	second = tempstring.find_first_of("/");
		source = receice_message.substr(5, 1);
		//cout << "Source: " << source << endl;

		//setting weight
	//	first += second + 1;
	//	tempstring = receice_message.substr(first, receice_message.size() - (first));
	//	second = tempstring.find_first_of("\r\n");
		cost = receice_message.substr(7, 1);
		cout << "cost: " << cost << endl;

		print_neighbours(router1);

		//Passing to add destination function
		if (destination != router1.src) {
			Add_destination(destination, cost, source);
		}
		
		print_neighbours(router1);

		second += first + 2;
		receice_message = receice_message.substr(10, receice_message.size() - (10));
	}
}
void read_via_only(char buf[]){
	ostringstream char_to_string;
	string destn,src,path,msg,cost,type,port,cost1,dv;
	int pos=0;
	string recieve_message = string(buf);
	size_t first, second, end_of_message;
		if(buf[0]!=router1.src.at(0)){
			//send_via_only(buf,3,recieve_message.substr(0,1));
		}
		else{
			//setting destination 
			destn=recieve_message.substr(0,1);
			//std::cout<<destn.c_str();
			//setting source 
			first = recieve_message.find_first_of("\r\n");
			src=recieve_message.substr(first+2,1);
			//std::cout<<src.c_str();
			
			//setting port
			cost=recieve_message.substr(first+2,recieve_message.size()-(first+2));
			first=cost.find_first_of("\r\n");
			cost=cost.substr(first+2,recieve_message.size()-(first+2));
			//first=cost.find_first_of("\r\n");			
			second=cost.find_first_of("-");
			port=cost.substr(0,second);
			//std::cout<<port.c_str();
			
			//setting type		
			cost1=cost.substr(first+2,recieve_message.size()-(first+2));
			first=cost1.find_first_of("\r\n");
			cost1=cost.substr(first+2,recieve_message.size()-(first+2));
			first=cost1.find_first_of("\r\n");
			second=cost1.find_first_of("-");
			type=cost1.substr(first+2,second-2);
			//std::cout<<type.c_str();
			first=cost1.find_first_of("\r\n");
			cost1=cost1.substr(first+2,recieve_message.size()-(first+2));
			first=cost1.find_first_of("\r\n");
			cost1=cost1.substr(first+2,recieve_message.size()-(first+2));
			//std::cout<<cost1.c_str();			
			//setting message
			//setting type		
			//cost2=cost1.substr(first+2,recieve_message.size()-(first+2));
			//setting message
			//first=cost1.find_first_of("\r\n");
			second=cost1.find_first_of("-");
			msg=cost1.substr(0,second);
			//std::cout<<msg.c_str();	
			first=cost1.find_first_of("\r\n");
			cost1=cost1.substr(first+2,recieve_message.size()-(first+2));
			second=cost1.find_first_of("-");
			dv=cost1.substr(0,second);				
			//std::cout<<dv.c_str();
			char end[]="\r\n";
			strcat((char*)dv.c_str(),end);
			cout<<dv.c_str();	
			Reading_in_DV((char*)dv.c_str());
	}



}


void send_via_only(char buf[],int type,string destn){
	ostringstream message;
	//std::cout<<"hello";
	if(type==1)// for dv
	{	//final destination of the dv
		string destination=destn;
		message<<destination;
		message<<"-destination\r\n";
		

		//src of the message
		string source=router1.src;
		message<<source;
		message<<"-source\r\n";
		
		//for port
		string port=to_string(router1.port);
		message<<port;
		message<<"-port\r\n";

		//type of funtion
		string type="1";
		message<<type;
		message<<"-type\r\n";
		

		//for message
		string msg="aliveee";
		message<<msg;
		message<<"-message\r\n";

		//for dv
		string dv=string(buf);
		message<<dv;
		message<<"-dv\r\n";

		//for path
		message<<"path-";
		message<<router1.src;
		message<<"/\r\n\r\n";	
		
	}
	
	else if(type==2)//for message
	{
		//final destination of the dv
		string destination=destn;
		message<<destination;
		message<<"-destination\r\n";
		

		//src of the message
		string source=router1.src;
		message<<source;
		message<<"-source\r\n";
		
		//for port
		string port=to_string(router1.port);
		message<<port;
		message<<"-port\r\n";

		//type of funtion
		string type="2";
		message<<type;
		message<<"-type\r\n";
		

		//for message
		string msg=string(buf);
		message<<msg;
		message<<"-message\r\n";

		//for dv
		string dv="alive";
		message<<dv;
		message<<"-dv\r\n";

		//for path
		message<<"path-";
		message<<router1.src;
		message<<"/\r\n\r\n";
	}
	else if(type==3)//for intermediate node
	{	
		size_t end_of_message;
		string recieve_message=string(buf);
		end_of_message = recieve_message.find_first_of("\r\n\r\n");
		//just a edit to add path
		string new_message=recieve_message.substr(0,end_of_message);
		message<<new_message;
		message<<router1.src;
		message<<"/\r\n\r\n";
				
	}

	else{
   		std::cout<<"funtion not supported";
}
using namespace std::this_thread;
using namespace std::chrono;

if(type==1){
		Neighbour *temp;
		temp=router1.head;
		int send_port;
			while(temp != NULL){
			
				if(temp->port_name.at(0)==destn.at(0)){
						send_port=temp->port;			
					}
				temp=temp->next_node;
  			}
				string message1=message.str();
		//Turning the DV into a char array to be sent to neightbours
				char *sending_array = new char[message1.length() + 1];
				strcpy(sending_array, message1.c_str());

				//Set up socket to send the forwading table on
				int Fowardtable_sock = socket(AF_INET, SOCK_DGRAM, 0);

				//Creates a temporary socket to forwarding table to
				sockaddr_in addrSend = {};
				addrSend.sin_family = AF_INET;
				addrSend.sin_port = htons(send_port);
				addrSend.sin_addr.s_addr = inet_addr("127.0.0.1");
				//Binding temp socket
				bind(Fowardtable_sock, (struct sockaddr *) &addrSend, sizeof(addrSend));

				if ((send_size = (sendto(Fowardtable_sock, sending_array, 400, 0, (struct sockaddr *) &addrSend, 20))) == -1) {
					std::cout << "\n\t\tSender: Couldn't send bbbbbbbbbbbbbbbbbtable\n\n";
				}

				//temp = temp->next_node;

				sleep_for(seconds(1));

				close(Fowardtable_sock);
	}	
	else if(type==2){
		Neighbour *temp;
		temp=router1.head;
		int send_port;
			while(temp != NULL){
			
				if(temp->port_name.at(0)==destn.at(0)){
						send_port=temp->via_port;			
					}
				temp=temp->next_node;
  			}string message1=message.str();
		//Turning the DV into a char array to be sent to neightbours
				char *sending_array = new char[message1.length() + 1];
				strcpy(sending_array, message1.c_str());

				//Set up socket to send the forwading table on
				int Fowardtable_sock = socket(AF_INET, SOCK_DGRAM, 0);

				//Creates a temporary socket to forwarding table to
				sockaddr_in addrSend = {};
				addrSend.sin_family = AF_INET;
				addrSend.sin_port = htons(send_port);
				addrSend.sin_addr.s_addr = inet_addr("127.0.0.1");
				//Binding temp socket
				bind(Fowardtable_sock, (struct sockaddr *) &addrSend, sizeof(addrSend));

				if ((send_size = (sendto(Fowardtable_sock, sending_array, 400, 0, (struct sockaddr *) &addrSend, 20))) == -1) {
					std::cout << "\n\t\tSender: Couldn't send table\n\n";
				}

				temp = temp->next_node;

				//sleep_for(seconds(1));

				close(Fowardtable_sock);
	}	
	else if(type==3){
		Neighbour *temp;
		temp=router1.head;
		int send_port;
			while(temp != NULL){
			
				if(temp->port_name.at(0)==destn.at(0)){
						send_port=temp->via_port;			
					}
				temp=temp->next_node;
  			}string message1=message.str();
		//Turning the DV into a char array to be sent to neightbours
				char *sending_array = new char[message1.length() + 1];
				strcpy(sending_array, message1.c_str());

				//Set up socket to send the forwading table on
				int Fowardtable_sock = socket(AF_INET, SOCK_DGRAM, 0);

				//Creates a temporary socket to forwarding table to
				sockaddr_in addrSend = {};
				addrSend.sin_family = AF_INET;
				addrSend.sin_port = htons(send_port);
				addrSend.sin_addr.s_addr = inet_addr("127.0.0.1");
				//Binding temp socket
				bind(Fowardtable_sock, (struct sockaddr *) &addrSend, sizeof(addrSend));

				if ((send_size = (sendto(Fowardtable_sock, sending_array, 400, 0, (struct sockaddr *) &addrSend, 20))) == -1) {
					std::cout << "\n\t\tSender: Couldn't send table\n\n";
				}

				//temp = temp->next_node;

				//sleep_for(seconds(1));

				close(Fowardtable_sock);

	}
}


//Enter 1. router name , 2. router port, 3. URL
int main(int argc, const char* argv[]) {

	pthread_t connection_thread;
	initial_graph();

	if (argc < 3)
	{
		std::cout << "Please enter all details\n";
		exit(0);
	}

	router1.src = argv[1][0];
	router1.port = atoi(argv[2]);
	//url = argv[3];								

	std::cout << "Router has been set up ";

	std::cout << "\n\tConnection: Connection thread is running...";
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

	if (bind(routerSock, (struct sockaddr *) &routerAddr, sizeof(routerAddr)) < 0)
	{
		std::cerr << "\n\tConnection: Router binding failed. Killing thread...";
		exit(1);
	}

	//No accepting or listening on UDP connection
	//Setting up neighbours
	router1.head = NULL;
	Initial_forwarding_table(&router1);
	print_neighbours(router1);
	//std::cout << "total number of edges are:";
	//std::cout<<graph_of_edges->total_number_of_vertices<< endl;

	//A check to see if the forwarding table has stabilized
	bool stabilized = false;

	//Loop that will send and receive forwarding tabel until it stabilizes

	//Setting up delay for displaying the forwarding table
	using namespace std::this_thread;
	using namespace std::chrono;

	print_neighbours(router1);

	while (!stabilized) {

		Neighbour *temp;

		if (router1.head == NULL) {
			cout << "No neighbours found" << endl;
			break;
		}
		else {
			temp = router1.head;

			while (temp != NULL) {
				
				if(temp->first==true){
				//Setting next port
				int temp_port = temp->port;

				//Sending table
				cout << "Sending DV " << endl;
				//cout<<"hello";
				//making char array to send 
				ostringstream send_array;

				Neighbour *forwadring_table_send = router1.head;
				//std::cout<<"hello";
				//Send DV to all neighbours
				//Structure of DV message - protocol, Destination,next node,int distance
				for (int i = 0; i < graph_of_edges->edges.size(); i++) {
					send_array << "DV/" << graph_of_edges->edges[i]->end << "/" << graph_of_edges->edges[i]->start << "/" << graph_of_edges->edges[i]->weight << "\r\n";

				}

				//Adding closing part that protocol can detect as message fully delivered
				//send_array << "\r\n";

				string string_to_char = send_array.str();

				//Turning the DV into a char array to be sent to neightbours
				char *sending_array = new char[string_to_char.length() + 1];
				strcpy(sending_array, string_to_char.c_str());
				//cout<<"hello";
				//Set up socket to send the forwading table on
					send_via_only(sending_array,1,temp->port_name);		
				}
					temp=temp->next_node;			
				}
}


		cout << "Waiting to receive" << endl;

		recvLen = recvfrom(routerSock, recvBuf, 256, 0, (struct sockaddr *) &senderAddr, &addrLen);

		std::cout << "\n\tConnection: Received " << recvLen << " bytes." << endl;
		//cout << recvBuf << endl;

		cout << "Reading DV message" << endl;
		read_via_only(recvBuf);

		//print_neighbours(router1);

	}


	cout << "Enters loop" << endl;
	string destination_chosen;
	int temp_port = 0;
	//Infinite loop that sends and is open to recieve packets
	for (;;) {

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
			
			sockaddr_in addrSend = {};   
			addrSend.sin_family = AF_INET;
			addrSend.sin_port = htons(temp_port);
			addrSend.sin_addr.s_addr = inet_addr("127.0.0.1");

			//Binding temp socket
			bind(sendSock, (struct sockaddr *) &addrSend, sizeof(addrSend));     //Not checking failure for now..
			//      
			if ((send_size = (sendto(sendSock, sendBuf, 20, 0, (struct sockaddr *) &addrSend, 20))) == -1) {
				std::cout << "\n\t\tSender: Couldn't send. :(";
				
			}
			
			close(sendSock);
		}

		recvLen = recvfrom(routerSock, recvBuf, 256, 0, (struct sockaddr *) &senderAddr, &addrLen);

		std::cout << "\n\tConnection: Received " << recvLen << " bytes.";
		cout << recvBuf << endl;
		if (recvLen > 0) {

			recvBuf[recvLen] = 0;
			char sendBuf[] = "hello";
			int sendSock = socket(AF_INET, SOCK_DGRAM, 0);

			//Creates a temporary socket to send packets to
			sockaddr_in addrSend = {};    //zero-int, sin_port is 0 which picks a random port for bind
			addrSend.sin_family = AF_INET;
			addrSend.sin_port = htons(atoi(url.c_str()));
			addrSend.sin_addr.s_addr = inet_addr("127.0.0.1");

			//Binding temp socket
			bind(sendSock, (struct sockaddr *) &addrSend, sizeof(addrSend));     //Not checking failure for now..
			   
			if ((send_size = (sendto(sendSock, sendBuf, 20, 0, (struct sockaddr *) &addrSend, 20))) == -1) {
				std::cout << "\n\t\tfailed to send";
				
			}
			
close(sendSock);
		}
	}

	close(routerSock);



	return 0;
}

    

