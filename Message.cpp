#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>

#include <pthread.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdio.h>

#define UPDT_VECTOR '1'
#define HEAD_MSG '2
public bool CHECK;

struct Packet {
    char type;        // Update Vector
    char dest_id;     // Destination Node ID
    char src_id;      // Source Node ID
    std::string data; // Data | Empty when unidentified packet
};

typedef struct Packet Packet;

struct RouterEntry {
    bool is_neighbor;
    char router_id;
    unsigned int cost, alt_cost;		// Cost to neighbour & Alternative Cost
    unsigned int port;
    char ref_router_id; 	// ID of node with the minimum cost
    long last_update; 		// Timestamp of last vector to check if its alive
};

typedef struct RouterEntry RouterEntry;

class Connection {
	private:
		struct addrinfo addr_hints, addrinfo *setup;
		int sockfd;
		std::size_t from_len;
		struct sockaddr_in their_addr;
		size_t addr_len;
		void initial_setup();
	public:
		Connection();
		~Connection();
		bool setup_connection(std::string address, std::string port);
		bool setup_connection(std::string address, int port);
		int send_udp(std::string request, std::string address_in, std::string port_in);
		int send_udp(std::string request, std::string address_in, int port_in);
		int recv_udp(std::string &request);
};
	
class Router {
	private:
		std::vector<Packet> packet_queue;
		void handle_packet(Packet &, std::string);
		void build_table(char *filename);
		void update_vector(Packet *);
		void forward_message(Packet &);
		pthread_t adv_thread;
		void run_advertisement_thread();
	public:
		Connection connection;
		char router_id;
		int port;
		pthread_mutex_t mutex_routing_table = PTHREAD_MUTEX_INITIALIZER;
		std::vector<RouterEntry> routing_table;
		unsigned int get_link_cost(RouterEntry &router);
		RouterEntry *get_router_by_id(char id);
		bool is_router_dead(RouterEntry &router);
		void print_routing_table();
		std::string serialize_packet(Packet *);
		void run_router(); // Call router to initiate and start
		Router(char);
		~Router();
};

void Router::run_router() {
    std::string message;
    for (;;) {
        connection.recv_udp(message);
        Packet packet;
        handle_packet(packet, message);
    }

}void Router::print_routing_table() {
    std::cout << "Routing Table:" << std::endl;
    for (int i = 0; i < routing_table.size(); i++)
	{
        RouterEntry &router = routing_table.at(i);
        std::cout << router.router_id << " ";
        if (router.is_neighbor) 
		{
            if (router.ref_router_id != -1) 
			{
                std::string str_ref(1, router.ref_router_id);
                std::cout << router.cost << " @ " << router.port << ", "
                          << router.alt_cost << " through "
                          << (router.ref_router_id < 'A' && router.ref_router_id > 'Z' ? std::to_string(router.ref_router_id) : str_ref)
                          << " ";
            } 
			else 
			{
                std::cout << router.cost << " @ " << router.port << " ";
            }
            std::cout << "(Last Received: "
                      << (std::time(0) - router.last_update) << " sec ago)";
        } 
		else 
		{
            std::cout << router.alt_cost << " through " << router.ref_router_id;
        }
        std::cout << std::endl;
    }
}

void Router::update_vector(Packet *packet) 
{
    unsigned int new_cost;
    char to_router_id;
    bool has_updated_table = false; // If added or updated the entry, used for printing
    to_router_id = packet->data[0];
    std::string cost_str = packet->data.substr(1);
    new_cost = atoi((char *)cost_str.c_str());
    pthread_mutex_lock(&mutex_routing_table); // Lock the mutex
    RouterEntry *src_router = get_router_by_id(packet->src_id); // Source from where the vector was recieved.
    if (CHECK && src_router->is_dead) 
	{
        std::cout << "Neighbor " << src_router->router_id
                  << " is alive now. After "
                  << (std::time(0) - src_router->last_update) << " sec."
                  << std::endl;
        src_router->is_dead = false;
    }
    src_router->last_update = std::time(0); // Updating the timestamp
    new_cost += src_router->cost;
    RouterEntry *router = get_router_by_id(to_router_id);
    if (router != NULL) {
        if (router->is_neighbor) { // Treat router as the neighbour if the cost is smaller than the previous neighbour.
            if (new_cost < router->alt_cost || (src_router->router_id == router->ref_router_id && new_cost != router->alt_cost)) 
			{
                if (CHECK)
                    std::cout << "Updating new alt_cost to " << new_cost
                              << " of neighbor " << to_router_id
                              << ". Received from " << src_router->router_id
                              << std::endl;
                router->alt_cost = new_cost;
                router->ref_router_id = src_router->router_id;
                has_updated_table = true;
            }
        } 
		else 
		{
            if (new_cost < router->alt_cost || (src_router->router_id == router->ref_router_id && new_cost != router->alt_cost)) {
                if (CHECK)
                    std::cout << "Updating new cost to " << new_cost
                              << " of distant router " << to_router_id
                              << ". Received from " << src_router->router_id
                              << std::endl;
                router->alt_cost = new_cost;
                router->ref_router_id = src_router->router_id;
                has_updated_table = true;
            }
        }
    } else {
        RouterEntry router;		// Check and add if it is the distant router
        router.is_neighbor = false;
        router.router_id = to_router_id;
        router.alt_cost = new_cost;
        router.ref_router_id = packet->src_id;
        router.port = 0;
        routing_table.push_back(router);
        has_updated_table = true;
        if (CHECK)
            std::cout << "Inserting new router " << to_router_id
                      << " in table with cost: " << new_cost
                      << ". Received from " << src_router->router_id
                      << std::endl;
    }
    pthread_mutex_unlock(&mutex_routing_table); // Unlock the previously locked mutex
    if (has_updated_table)
	{
       print_routing_table();
	}
}

void Router::forward_message(Packet &packet)
{
    int index = -1;
    int ref_router_id = -1;
    if (CHECK)
        std::cout << "Packet destination: " << packet.dest_id
                  << ". Source: " << packet.src_id << std::endl;
    RouterEntry *fwd_router = get_router_by_id(packet.dest_id);
    if (fwd_router == NULL)
	{
        if (CHECK)
            std::cout << "Couldn't find router " << packet.dest_id
                      << " in table. Packet dropped." << std::endl;
        return;
    }
    if (fwd_router != NULL) 
	{
        if (!fwd_router->is_neighbor) {
            fwd_router = get_router_by_id(fwd_router->ref_router_id);
        }
    }
    if (fwd_router == NULL) 
	{
        if (CHECK)
            std::cout << "Couldn't find ref router for " << packet.dest_id
                      << " in table. Packet dropped." << std::endl;
        return;
    }
    if (is_router_dead(*fwd_router)) 
	{
        std::cout << "Couldn't forward packet. Neighbor "
                  << fwd_router->router_id << " is dead. Packet dropped."
                  << std::endl;
        return;
    }
    std::cout << "Forwarding the packet to neighbor " << fwd_router->router_id
              << " on Port: " << fwd_router->port << std::endl;
    std::string message = serialize_packet(&packet);
    connection.send_udp(message, HOME_ADDR, fwd_router->port);
}

void Router::handle_packet(Packet &packet, std::string message) {
    if (message.length() < 2) {
        std::cerr << "Invalid Packet Received. Message Length(): "
                  << message.length() << std::endl;
        return;
    }
    // De-serializing the packets
    packet.type = message[0];
    packet.dest_id = message[1];
    packet.src_id = message[2];
    packet.data = message.substr(3);
    // Revieving the thread / Reading Data
    if (packet.dest_id != router_id) 
	{
        if (CHECK)
		{
            std::cout << "Forward Message" << std::endl;
		}
        pthread_mutex_lock(&mutex_routing_table);
        forward_message(packet);
        pthread_mutex_unlock(&mutex_routing_table);
    } 
	else 
	{
        switch (packet.type) 
		{
			case UPDT_VECTOR:
				update_vector(&packet);
				break;
			case HEAD_MSG:
				std::cout << "Final Destination: Source: " << packet.src_id
						  << std::endl
						  << "Message:" << std::endl
						  << packet.data << std::endl;
				break;
			default:
				std::cout << "HEADER_FIELD_TYPE_INVALID" << std::endl;
        }
    }
}

RouterEntry *Router::get_router_by_id(char id) 
{
    for (int i = 0; i < routing_table.size(); i++) 
	{
        RouterEntry &router = routing_table.at(i);
        if (router.router_id == id)
            return &router;
    }
   return NULL;
}

bool Router::is_router_dead(RouterEntry &router) 
{
    if (!router.is_neighbor)
	{
        if (router.ref_router_id == -1) // No neighbour router to reach
            return true;
        else 
		{
            RouterEntry *ref_router = get_router_by_id(router.ref_router_id);
            if (ref_router == NULL)
                return true;
            return std::time(0) - ref_router->last_update > SLEEP_SEC + 1; // No reply from the neighbour after the time interval
        }
    }

    return std::time(0) - router.last_update > SLEEP_SEC + 1; //  No reply from the neighbour after the time interval
}

unsigned int Router::get_link_cost(RouterEntry &router) { // Returns the last known link cost from the current router
    if (router.is_neighbor) {
        if (router.ref_router_id != -1 && router.alt_cost < router.cost)
            return router.alt_cost;
        else
            return router.cost;
    } else
        return router.alt_cost;
}

void update_vector(Router *r, RouterEntry &dest_router) {
    for (int j = 0; j < r->routing_table.size(); j++) {
        RouterEntry &router = r->routing_table.at(j);
        if (router.router_id == dest_router.router_id)
            continue; 									// Skip sending neighbour cost
        Packet packet;
        packet.type = UPDT_VECTOR;
        packet.data = ((char)router.router_id); 		// Reach last router
        unsigned int cost = r->get_link_cost(router); 	// Last known cost
        if (r->is_router_dead(router))                	// Check if the router is dead
            cost = INT_MAX;
        packet.data += std::to_string(cost); 			// Cost of the link
        packet.src_id = r->router_id;           		// This() router
        packet.dest_id = dest_router.router_id; 		// Neighbour is destination
        std::string message = r->serialize_packet(&packet);
        r->connection.send_udp(message, HOME_ADDR, dest_router.port); // Send to neighbor (tc port)
    }
}

void *adv_thread_func(void *args) {
    Router *r = (Router *)args;		// Sending the whole routing table to its neighbors
    for (;;) {
        pthread_mutex_lock(&r->mutex_routing_table);
        bool p_rt = false;			// Loop through all neighbors
        for (int i = 0; i < r->routing_table.size(); i++) 
		{
            RouterEntry &neighbor_i = r->routing_table.at(i);
            if (!neighbor_i.is_neighbor)
                continue; 			// Avoid sending distance routers info
            if (r->is_router_dead(neighbor_i)) {
                if (CHECK && !neighbor_i.is_dead) {
                    std::cout << "Neighbor " << neighbor_i.router_id
                              << " is dead. Last Update: "
                              << (std::time(0) - neighbor_i.last_update)
                              << " sec ago" << std::endl;
                    neighbor_i.is_dead = true;
                }					// Update the table
                for (int j = 0; j < r->routing_table.size(); j++) {
                    RouterEntry &router = r->routing_table.at(j);
									// Update all the routers' link cost reached through dead router.
                    if (router.ref_router_id == neighbor_i.router_id) {
                        p_rt = p_rt || router.alt_cost !=
                                           INT_MAX; // Print routing table if updated
                        router.alt_cost = INT_MAX;
                        router.ref_router_id = -1;
                    }
                }
            }
        }
        if (p_rt)
            r->print_routing_table();
        pthread_mutex_unlock(&r->mutex_routing_table);
        sleep(SLEEP_SEC);
    }
    pthread_exit(NULL);
}

std::string Router::serialize_packet(Packet *packet) {
    std::string str = "";
    str += packet->type;
    str += packet->dest_id;
    str += packet->src_id;
    str += packet->data;
    return str;
}
// Build initial table with known neighbors
void Router::build_table(char *filename) {
    char input[LINELENGTH];
    FILE *input_file;
    char router;
    char edge;
    unsigned int port;
    unsigned int weight;
    input_file = fopen(filename, "r");
    if (!input_file) {
        std::cerr << "Unable to open file: " << filename << std::endl;
    } else {
        while (!feof(input_file)) {
            fgets(input, LINELENGTH, (FILE *)input_file);
            router = strtok(input, (char *)",")[0];
            edge = strtok(NULL, (char *)",")[0];
            port = (unsigned int)atoi(strtok(NULL, (char *)","));
            weight = (unsigned int)atoi(strtok(NULL, (char *)"\n"));
            if (router != router_id) {
                if (edge == router_id)
                    this->port = port;
                continue;
            }
            RouterEntry router;
            router.router_id = edge;
            router.cost = weight;
            router.alt_cost = INT_MAX;
            router.ref_router_id = -1;
            router.port = port;
            router.is_neighbor = true;
            router.last_update =
                std::time(0); // 0 denoted increased time past the delay.
            routing_table.push_back(router);
        }
    }
    fclose(input_file);
}