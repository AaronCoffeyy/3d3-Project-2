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
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <mutex>  

//Task-list
/*
1. Remove node from list when it is found to be dead + remove edges with dead router in the graph
2. make each time the table changes print to a file with a  ****timestanmp****
*/


//Setting up delay for displaying the forwarding table
using namespace std::this_thread;
using namespace std::chrono;
using namespace std;

string send_via_only(char buf[], int type, string destn, string path_taken);

#define routers_num 8

//void *router_connection(void *threadID);
static int clientSockfd;
int numBytes;

//Starting timestamp
long Starting_time;

//Mutex for the router 
mutex reading_in;

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
	long last_update;
	bool direct_neighbours;
	bool alive;
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

void Neighbour_setup(routerData *host, string source, string dest, string port_num, string weight, bool direct_neighbours) {

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
	New_Neighbour->last_update = std::time(0);
	New_Neighbour->direct_neighbours = direct_neighbours;
	New_Neighbour->alive = true;
	//cout << "time 1: " << New_Neighbour->last_update << endl;

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

//Add link the to graph struct 
void add_Link(string start, string end, string weight) {

	edge *new_edge = new edge;
	int w = stoi(weight);

	//Setting up new edge
	new_edge->start = start;
	new_edge->end = end;
	new_edge->weight = stoi(weight);

	bool edge_exists = false;

	//Checking to see if the graph is empty 
	if (graph_of_edges->total_number_of_vertices == 0) {
		graph_of_edges->total_edges++;
		graph_of_edges->total_number_of_vertices += 2;
		graph_of_edges->edges.push_back(new_edge);
		//cout << "\nAdded First Edge: \t" << g->edges[0]->start << "->" << g->edges[0]->end << " W: " << g->edges[0]->weight;
	}
	else {
		//First checks to see if the edges exists 
		for (int i = 0; i < graph_of_edges->edges.size(); i++) {
			if (graph_of_edges->edges[i]->start == start && graph_of_edges->edges[i]->end == end) {
				//cout << "\nEdge" << graph_of_edges->edges[0]->start << "->" << graph_of_edges->edges[0]->end << "already exists" << endl;
				edge_exists = true;
				if (graph_of_edges->edges[i]->weight != w) {
					graph_of_edges->edges[i]->weight = w;
				}
			}
			//Changing weight only
			if (graph_of_edges->edges[i]->start == end && graph_of_edges->edges[i]->end == start) {
				if (graph_of_edges->edges[i]->weight != w) {
					graph_of_edges->edges[i]->weight = w;
				}
			}
		}
		//If the number of vertices that have not already been seen on previous edges
		if (edge_exists == false && end != router1.src) {
			int  Vertice1 = 1, Vertice2 = 1;

			//Add one ot total edges as edges does not already exist
			graph_of_edges->total_edges++;

			for (int i = 0; i < graph_of_edges->edges.size(); i++)
			{
				if (graph_of_edges->edges[i]->start == start || graph_of_edges->edges[i]->end == start) {
					Vertice1 = 0;
				}
				if (graph_of_edges->edges[i]->start == end || graph_of_edges->edges[i]->end == end) {
					Vertice2 = 0;
				}
			}
			graph_of_edges->total_number_of_vertices = graph_of_edges->total_number_of_vertices + Vertice2 + Vertice1;
			graph_of_edges->edges.push_back(new_edge);
		}
	}
}

//Setting up connection from file
void Initial_forwarding_table(routerData *init_router) {

	ifstream nodefile("Node set up.txt");

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
					Neighbour_setup(init_router, source, neighbour, port_num, weight, true);

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

	long current_time = std::time(0);

	long time_delay = current_time - Starting_time;

	/*
	string filename = router1.src + "_Routing table changes.txt";

	ofstream outfile(filename, std::ios::app);

	outfile << setw(15) << left << "Timestamp: " << time_delay << "secs" << endl;
	outfile << setw(15) << left << "Destination";
	outfile << setw(5) << left << "Cost";
	outfile << setw(20) << left << "Outgoing UDP Port";
	outfile << setw(20) << left << "Destination UDP port" << endl;

	for (Neighbour *pointer = router1.head; pointer != NULL; pointer = pointer->next_node) {
		outfile << setw(15) << left << pointer->port_name;
		outfile << setw(5) << left << pointer->total_dist;
		outfile << left << pointer->via_port << " (Node " << pointer->via_port_name << ")" << setw(6) << " ";
		outfile << left << " Node " << pointer->port_name << "" << endl;
	}

	outfile.close();
	*/
	///*

	cout << endl;
	cout << setw(15) << left << "Timestamp: " << time_delay << "secs" << endl;
	cout << setw(15) << left << "Destination";
	cout << setw(5) << left << "Cost";
	cout << setw(20) << left << "Outgoing UDP Port";
	cout << setw(20) << left << "Destination UDP port" << endl;

	for (Neighbour *pointer = print_router.head; pointer != NULL; pointer = pointer->next_node) {
		if (pointer->total_dist < 998) {
			cout << setw(15) << left << pointer->port_name;
			cout << setw(5) << left << pointer->total_dist;
			cout << left << pointer->via_port << " (Node " << pointer->via_port_name << ")" << setw(6) << " ";
			cout << left << pointer->port << left << " (Node " << pointer->port_name << ")" << endl;
		}
	}

	cout << "\n\n" << endl;
	cout << "Edges" << endl;

	int E = graph_of_edges->total_edges;

	for (int j = 0; j < E; j++)
	{
		cout << graph_of_edges->edges[j]->start << " " << graph_of_edges->edges[j]->end << " " << graph_of_edges->edges[j]->weight << endl;
	}
	//*/
}

void BellmanFord(struct graph* g, int src)
{
	//std::////cout<<"\nhello\n";
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

	while (iterator < V - 1)
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
				////cout << "Checker" << endl;
				//std:://cout << "to get to node " << (char)nextnode[v % 65] << " go through " << (char)node[u % 65] << endl;
				//If there is no link node between currernt and destiantion then destiantion is next node

				if (node[u % 65] == node[src % 65]) {
					//std:://cout << "\nto get to node " << (char)nextnode[v % 65] << " go through " << (char)node[v % 65] << endl;
					nextnode[v % 65] = (char)node[v % 65];
				}
				else {
					////cout << "second" << endl;
					//std:://cout << "\nto get to node " << (char)nextnode[v % 65] << " go through " << (char)node[u % 65] << endl;
					nextnode[v % 65] = (char)node[u % 65];
				}
			}
		}
	}

	pos = 0;
	iterator = 0;
	char my_node, my_nodes_next_node;

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
						////cout << "change " << nextnode[inner_pos] << " to " << my_nodes_next_node << endl;
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

	Neighbour *temp;
	temp = router1.head;

	bool changes = false;

	while (temp != NULL) {
		if (temp->via_port_name.at(0) != nextnode[temp->port_name.at(0) % 65] || temp->total_dist != shortestdist[temp->port_name.at(0) % 65]) {
			temp->via_port_name.at(0) = nextnode[temp->port_name.at(0) % 65];
			temp->total_dist = shortestdist[temp->port_name.at(0) % 65];

			Neighbour *temp1;
			temp1 = router1.head;
			while (temp1 != NULL) {

				if (temp->via_port_name == temp1->port_name) {
					temp->via_port = temp1->port;
					break;
				}
				temp1 = temp1->next_node;
			}
			changes = true;
		}
		//std:://cout << "\nto get to node " << temp->port_name.at(0) << " go through " << temp->via_port_name.at(0) << " in " << temp->total_dist << "\n";
		temp = temp->next_node;
	}

	if (changes == false) {
		//cout << "No changes made " << endl;
	}
	else {
		print_neighbours(router1);
	}

	//std:://cout << "\nRouting table regenerated.\n";
	//std:://cout << "\n----------------------------------------------------------------------------------\n";
}

//Adding new "Neighbour" to graph via receiving a DV
void Add_destination(string port_name, string hop_distance, string source) {

	Neighbour *temp;
	int via_port = 0;
	bool found = false;
	int cost = stoi(hop_distance);

	if (router1.head == NULL) {
		//return;
	}
	else if (port_name == router1.src || source == router1.src) { //Can only check for its neighbours whether the neighbour is alive if it is it scraps old message

		temp = router1.head;

		while (temp->next_node != NULL) {
			if (temp->port_name == source && temp->alive && cost != 999) {
				//cout << "P: " <<  port_name << " H: " << hop_distance << " S: " << source << " alive\n";
				add_Link(source, port_name, hop_distance);
				BellmanFord(graph_of_edges, router1.src.at(0));
			}
			if (temp->port_name == source && !temp->alive && cost == 999) {
				//cout << "P: " << port_name << " H: " << hop_distance << " S: " << source << " not alive\n";
				add_Link(source, port_name, hop_distance);
				BellmanFord(graph_of_edges, router1.src.at(0));
			}
			if (temp->port_name == port_name && temp->alive && cost != 999) {
				//cout << "P: " << port_name << " H: " << hop_distance << " S: " << source << " alive\n";
				add_Link(source, port_name, hop_distance);
				BellmanFord(graph_of_edges, router1.src.at(0));
			}
			if (temp->port_name == port_name && !temp->alive && cost == 999) {
				//cout << "P: " << port_name << " H: " << hop_distance << " S: " << source << " not alive\n";
				add_Link(source, port_name, hop_distance);
				BellmanFord(graph_of_edges, router1.src.at(0));
			}
			temp = temp->next_node;
		}

		//cout << "here2\n";
		//return;
	}
	else {
		temp = router1.head;

		while (temp->next_node != NULL) {
			if (temp->port_name == port_name) {
				found = true;
			}
			else if (temp->port_name == source) {
				via_port = temp->port;
			}
			temp = temp->next_node;
		}

		if (temp->port_name == port_name) {
			found = true;
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
			New_Neighbour->direct_neighbours = false;
			New_Neighbour->alive = true;

			New_Neighbour->total_dist = New_Neighbour->hop_distance;

			temp->next_node = New_Neighbour;

			//cout << "New destinaiton added" << endl;

			add_Link(source, port_name, hop_distance);
			BellmanFord(graph_of_edges, router1.src.at(0));

		}
		else {
			//cout << "Found already " << source << " " << port_name  << endl;

			add_Link(source, port_name, hop_distance);
			BellmanFord(graph_of_edges, router1.src.at(0));
		}
	}
}

void Is_alive(string router_name) {

	Neighbour *temp;
	temp = router1.head;

	while (temp != NULL) {
		if (temp->port_name == router_name && temp->direct_neighbours == true) {
			temp->last_update = std::time(0);
			temp->alive = true;
			//cout << temp->port_name << " updated\n";
		}
		temp = temp->next_node;
	}
}

//takes in DV message and builds the forwarding table from that
void Reading_in_DV(char table[]) {

	ostringstream char_to_string;
	string temp1;

	string protocol, source, destination, cost;

	string receice_message = string(table);
	size_t first, second, end_of_message;
	string tempstring;

	//finding end of message
	end_of_message = receice_message.find_first_of("\r\n\r\n");

	while (receice_message != "\r\n\r\n\r\n") {
		//setting Protocol
		first = receice_message.find_first_of("/");
		protocol = receice_message.substr(0, first);
		cout << "Protocol: " << protocol << endl;

		//setting destination
		tempstring = receice_message.substr(first + 1, receice_message.size() - (first + 1));
		second = tempstring.find_first_of("/");
		destination = receice_message.substr(first + 1, second);
		cout << "Dest: " << destination << endl;

		//setting source
		first += second + 2;
		tempstring = receice_message.substr(first, receice_message.size() - (first));
		second = tempstring.find_first_of("/");
		source = receice_message.substr(first, second);
		cout << "Source: " << source << endl;

		//setting weight
		first += second + 1;
		tempstring = receice_message.substr(first, receice_message.size() - (first));
		second = tempstring.find_first_of("\r\n");
		cost = receice_message.substr(first, second + 2);
		cout << "cost: " << cost << endl;

		//Passing to add destination function
		//if (destination != router1.src) {
		Add_destination(destination, cost, source);
		//}

		second += first + 2;
		receice_message = receice_message.substr(second, receice_message.size() - (second));

		cout << "received message: " << receice_message << endl;
	}
}

//Need to add stuff for routing message
void read_via_only(char buf[]) {
	ostringstream char_to_string;
	string destn, src, path, msg, cost, type, port, cost1, dv;
	int pos = 0;
	string recieve_message = string(buf);
	string tempstring;
	size_t first, second, end_of_message;

	//setting destination
	first = recieve_message.find_first_of(":");
	tempstring = recieve_message.substr((first + 2), recieve_message.size() - (first + 2));
	second = tempstring.find_first_of("\r\n");
	destn = tempstring.substr(0, second);
	//cout << "Destination: " << destn << "!" <<  endl;

	//setting source
	tempstring = tempstring.substr((second), tempstring.size() - (second));
	first = tempstring.find_first_of(":");
	tempstring = tempstring.substr((first + 2), tempstring.size() - (first + 2));
	second = tempstring.find_first_of("\r\n");
	src = tempstring.substr(0, second);
	//cout << "Source: " << src << "!" << endl;

	//Updating the last time the router was updated - keeping it alive
	Is_alive(src);

	//setting port
	tempstring = tempstring.substr((second), tempstring.size() - (second));
	first = tempstring.find_first_of(":");
	tempstring = tempstring.substr((first + 2), tempstring.size() - (first + 2));
	second = tempstring.find_first_of("\r\n");
	port = tempstring.substr(0, second);
	//cout << "Port: " << port << "!" << endl;

	//setting Type
	tempstring = tempstring.substr((second), tempstring.size() - (second));
	first = tempstring.find_first_of(":");
	tempstring = tempstring.substr((first + 2), tempstring.size() - (first + 2));
	second = tempstring.find_first_of("\r\n");
	type = tempstring.substr(0, second);
	//cout << "Type: " << type << "!" << endl;

	string DV_message_string = tempstring.substr((second), tempstring.size() - (second));

	if (type == "DV") {
		while (DV_message_string != "\r\n\r\n") {

			string cost, source, destination;
			//setting Protocol
			first = DV_message_string.find_first_of("/");

			//setting destination
			tempstring = DV_message_string.substr(first + 1, DV_message_string.size() - (first + 1));
			second = tempstring.find_first_of("/");
			destination = DV_message_string.substr(first + 1, second);
			//cout << "Dest: " << destination << endl;

			//setting source
			first += second + 2;
			tempstring = DV_message_string.substr(first, DV_message_string.size() - (first));
			second = tempstring.find_first_of("/");
			source = DV_message_string.substr(first, second);
			//cout << "Source: " << source << endl;

			//setting weight
			first += second + 1;
			tempstring = DV_message_string.substr(first, DV_message_string.size() - (first));
			second = tempstring.find_first_of("\r\n");
			cost = DV_message_string.substr(first, second + 2);
			//cout << "cost: " << cost << endl;

			//Passing to add destination function
			//if (destination != router1.src) {
			Add_destination(destination, cost, source);
			//}

			second += first + 2;
			DV_message_string = DV_message_string.substr(second, DV_message_string.size() - (second));

			//cout << "received message: " << DV_message_string << endl;
		}
	}
	else if (type == "MSG") {

		//setting Path
		tempstring = tempstring.substr((second), tempstring.size() - (second));
		first = tempstring.find_first_of(":");
		tempstring = tempstring.substr((first + 2), tempstring.size() - (first + 2));
		second = tempstring.find_first_of("\r\n");
		path = tempstring.substr(0, second);

		//cout << "Path: " << path << endl;

		
		if (router1.src == destn) {
			cout << "Message reecieved from port " << port << endl;
		}
		else {
			for (Neighbour *pointer = router1.head; pointer != NULL; pointer = pointer->next_node) {

				char sendBuf[] = "hello";

				//Finds a valid destintation
				if (destn == pointer->port_name && destn != router1.src) {

					if (pointer->total_dist < 998) {
						int sending_port = pointer->via_port;
						string sender = send_via_only(sendBuf, 2, pointer->port_name, path);

						//Send to next router
						struct sockaddr_storage senderAddr;
						char sendBuf[256], recvBuf[256];
						int recvLen;

						int sendSock = socket(AF_INET, SOCK_DGRAM, 0);

						//Creates a temporary socket to send packets to
						sockaddr_in addrSend = {};    //zero-int, sin_port is 0 which picks a random port for bind
						addrSend.sin_family = AF_INET;
						addrSend.sin_port = htons(sending_port);
						addrSend.sin_addr.s_addr = inet_addr("127.0.0.1");

						//Binding temp socket
						bind(sendSock, (struct sockaddr *) &addrSend, sizeof(addrSend));     //Not checking failure for now..

						//Turning the DV into a char array to be sent to neightbours
						char *sending_array = new char[sender.length() + 1];
						strcpy(sending_array, sender.c_str());

						if ((numBytes = (sendto(sendSock, sending_array, 100, 0, (struct sockaddr *) &addrSend, 100))) == -1) {
							std::cout << "\n\t\tSender: Couldn't send. :(";
							//  return;
						}
					}
					else {
						cout << "Error invalid destination\n";
					}
				}

			}
		}
	}
	else {
		cout << "Bad message received\n";
		return;
	}
}

string send_via_only(char buf[], int type, string destn, string path_taken) {
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
			message << "DV: ";
			message << dv;
			message << "\r\n\r\n";
		}
		else {
			//type of funtion
			message << "Type: ";
			message << "MSG\r\n";

			//for path
			message << "Path: ";
			message << path_taken;
			message << ",";
			message << router1.src;
			message << "\r\n";

			//for dv
			string dv = string(buf);
			message << "DV: ";
			message << dv;
			message << "\r\n\r\n";
		}
	}
	else {
		std::cout << "funtion not supported";
	}

	string return_message = message.str();
	return return_message;
}

string Build_Message(string destination_chosen) {

	//making char array to send 
	ostringstream send_array;

	//Send DV to all neighbours
	//Structure of DV message - protocol, Destination,next node,int distance
	send_array << "MSG/" << destination_chosen << "/" << router1.src << "/" << "message sent !!!!!!!!!!";

	//Adding closing part that protocol can detect as message fully delivered
	send_array << "\r\n\r\n";

	string returning_string = send_array.str();

	return returning_string;
}

//Checks to see when the last_update was preformed and if its over a certain treshold the router is declared
//Dead and all edges relating to that node have their costs sent to 999
//Need to run on a seperate thread that will operate every 5ms
void Is_router_dead() {

	for (;;) {

		//Very rudimnetal timer to check if nodes are asleep every 1 second
		sleep_for(milliseconds(100));

		//Current time ot check against last update time
		long current_time = std::time(0);

		Neighbour *checker = router1.head;
		bool discovered = false;

		Neighbour *last_router;
		while (checker != NULL) {

			if (checker->direct_neighbours == true) {
				if ((current_time - checker->last_update) > 10) {
					//cout << "current_time: " << current_time << " last update: " << checker->last_update << endl;
					//cout << checker->port_name << " is dead" << endl;

					//Possible changes needed
					//add_Link(router1.src, checker->port_name, "999");
					checker->alive = false;

					for (int i = 0; i < graph_of_edges->edges.size(); i++) {
						if (graph_of_edges->edges[i]->start == checker->port_name || graph_of_edges->edges[i]->end == checker->port_name) {
							graph_of_edges->edges[i]->weight = 999;
						}
						//cout << graph_of_edges->edges[i]->start << "->" << graph_of_edges->edges[i]->end << " W: " << graph_of_edges->edges[i]->weight;

					}

					//Removing dead node from the list


					BellmanFord(graph_of_edges, router1.src.at(0));

					//print_neighbours(router1);
				}
			}
			checker = checker->next_node;
		}
	}
}

//Continually sends DV messages to all its neighbouring routers
void Stabilize_network(int routerSock) {

	struct sockaddr_storage senderAddr;
	char sendBuf[256], recvBuf[256];
	int recvLen;

	socklen_t addrLen = sizeof(senderAddr);

	for (;;) {

		Neighbour *temp;

		if (router1.head == NULL) {
			//cout << "No neighbours found" << endl;
			break;
		}
		else {
			temp = router1.head;

			while (temp != NULL) {
				//Dont send DV to routers that are presumed dead
				if (temp->alive == true && temp->direct_neighbours) {
					//Setting next port
					int temp_port = temp->port;

					//Sending table
					//cout << "Sending DV " << endl;

					//making char array to send 
					ostringstream send_array;

					Neighbour *forwadring_table_send = router1.head;

					//Send DV to all neighbours
					//Structure of DV message - protocol, Destination,next node,int distance
					for (int i = 0; i < graph_of_edges->edges.size(); i++) {
						send_array << "DV/" << graph_of_edges->edges[i]->end << "/" << graph_of_edges->edges[i]->start << "/" << graph_of_edges->edges[i]->weight << "\r\n";
					}

					string string_to_char1 = send_array.str();

					//Turning the DV into a char array to be sent to neightbours
					char *sending_array1 = new char[string_to_char1.length() + 1];
					strcpy(sending_array1, string_to_char1.c_str());


					string string_to_char = send_via_only(sending_array1, 1, temp->port_name, router1.src);

					//Turning the DV into a char array to be sent to neightbours
					char *sending_array = new char[string_to_char.length() + 1];
					strcpy(sending_array, string_to_char.c_str());

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


					sleep_for(seconds(1));

					close(Fowardtable_sock);
				}
				temp = temp->next_node;
			}
		}
	}
}

//Enter 1. router name , 2. router port, 3. URL
int main(int argc, const char* argv[]) {

	//Setting starting times
	Starting_time = std::time(0);

	initial_graph();

	if (argc < 3)
	{
		std::cout << "Please enter all details\n";
		exit(0);
	}

	router1.src = argv[1][0];
	router1.port = atoi(argv[2]);

	std::cout << "Router has been set up ";

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

	//Setting up text file
	string filename = router1.src + "_Routing table changes.txt";

	ofstream outfile(filename);

	outfile.clear();
	outfile.close();

	//No accepting or listening on UDP connection
	//Setting up neighbours
	router1.head = NULL;
	Initial_forwarding_table(&router1);
	print_neighbours(router1);

	//testing total number of vertices is correct
	//std::cout << "total number of edges are:";
	//std::cout<<graph_of_edges->total_number_of_vertices<< endl;

	//Make a thread to continually send Dv messages to routers
	//Loop that will send and receive forwarding table until it stabilizes
	std::thread Stabilize_network_thread = thread(Stabilize_network, routerSock);
	Stabilize_network_thread.detach();

	//Thread for is alive checker
	std::thread Is_router_dead_thread = thread(Is_router_dead);
	Is_router_dead_thread.detach();

	cout << "\nEnters listnening loop" << endl;

	//Infinite loop that listens and if it receives a DV or message will make a new thread
	for (;;) {

		recvLen = recvfrom(routerSock, recvBuf, 256, 0, (struct sockaddr *) &senderAddr, &addrLen);

		//std::cout << "\n\tConnection: Received " << recvLen << " bytes\n";
		//cout << recvBuf << endl;


		//reading_in.lock();
		std::thread Protocol_decider_thread = thread(read_via_only, recvBuf);
		Protocol_decider_thread.detach();
		//reading_in.unlock();
		//print_neighbours(router1);
	}
	close(routerSock);



	return 0;
}