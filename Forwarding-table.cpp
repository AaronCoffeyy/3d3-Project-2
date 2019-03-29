//Must do:
//The forwarding table function is going ot be passed a tabel most likely in string form
//It will have to parse the information and update its own internal table
//Each time the table is update it will write it to file with a timestamp and table before
//after the update
//++----------------------------------------------------------------------------------++
//When submitting code have it so it only prints to file if there is a change to the 
//Forwarding table.
//++----------------------------------------------------------------------------------++
//Not sure if this function will be in forwarding table code:
//The router will then send its newly to all neighbouring nodes/routers
//++----------------------------------------------------------------------------------++

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

using namespace std;

//Neighbour struct with holds the port number, hop distance and if they are connected
struct Neighbour {
	int port;
	int hop_distance;
	bool connected;
	string port_name;
	Neighbour *next_node;
};

//Temp just for testing purposes of the forwarding table
struct router {
	int port;
	string portname;
	Neighbour *head;
};

void Neighbour_setup(router *host, string source, string dest, string port_num, string weight) {
	
	Neighbour *temp;

	//Setting up new neighbour
	Neighbour *New_Neighbour = new Neighbour;
	New_Neighbour->port_name = dest;
	New_Neighbour->next_node = NULL;
	New_Neighbour->port = stoi(port_num);
	New_Neighbour->hop_distance = stoi(weight);

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

//Setting up connection from file
void Initial_forwarding_table(router *init_router) {

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
				if (init_router->portname == source) {
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

//
void Forwarding_table(){
	
	
	
	
	
	
	
	
	
	
	
}

//Prints out neighbour of the router
void print_neighbours(router print_router) {

	cout << "Router " << print_router.portname << endl;

	for (Neighbour *pointer = print_router.head; pointer != NULL; pointer = pointer->next_node) {
		cout << pointer->port_name << " -> " << pointer->port << " -> " << pointer->hop_distance << endl;
	}
}

int main() {

	//A
	router Router_A;
	Router_A.portname = "A";
	Router_A.head = NULL;
	Initial_forwarding_table(&Router_A);
	print_neighbours(Router_A);
	
	//B
	router Router_B;
	Router_B.portname = "B";
	Router_B.head = NULL;
	Initial_forwarding_table(&Router_B);
	print_neighbours(Router_B);

	//C
	router Router_C;
	Router_C.portname = "C";
	Router_C.head = NULL;
	Initial_forwarding_table(&Router_C);
	print_neighbours(Router_C);

	return 0;
}