#include <iostream>
#include <thread>
#include <mutex>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <memory>

#define MAX_LEN 200
#define NUM_COLORS 6
#define MAX_CLIENTS 100 // Maximum number of clients

using namespace std;

// Structure to represent a terminal client
struct Terminal {
    int id; // Client ID
    string name; // Client name
    int socket; // Socket for communication
    std::unique_ptr<std::thread> th; // Using unique_ptr to manage threads
};

// Array to store connected clients
Terminal clients[MAX_CLIENTS];

// ANSI color codes for output
string def_col = "\033[0m";
string colors[] = {"\033[31m", "\033[32m", "\033[33m", "\033[34m", "\033[35m", "\033[36m"};

// Global variables
int seed = 0; // Seed for client IDs
mutex cout_mtx, clients_mtx; // Mutexes for thread synchronization
int num_clients = 0; // Number of connected clients

// Function prototypes
string color(int code); // Function to get ANSI color code based on input code
void set_name(int id, char name[]); // Function to set client's name
void shared_print(string str, bool endLine = true); // Function for thread-safe printing
void broadcast_message(string message, int sender_id); // Function to broadcast message to all clients except sender
void end_connection(int id); // Function to end connection with a client
void handle_client(int client_socket, int id); // Function to handle individual client communication
string encryptCaesarCipher(const string& text, int shift); // Function to encrypt text using Caesar Cipher

// Main function
int main() {
    // Create server socket
    int server_socket;
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket: ");
        exit(-1);
    }

    // Define server address
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(10000);
    server.sin_addr.s_addr = INADDR_ANY;
    bzero(&server.sin_zero, 0);

    // Bind server socket to the address
    if ((bind(server_socket, (struct sockaddr *)&server, sizeof(struct sockaddr_in))) == -1) {
        perror("bind error: ");
        exit(-1);
    }

    // Listen for incoming connections
    if ((listen(server_socket, 8)) == -1) {
        perror("listen error: ");
        exit(-1);
    }

    // Accept and handle incoming connections
    struct sockaddr_in client;
    int client_socket;
    unsigned int len = sizeof(sockaddr_in);

    // Display welcome message
    cout << colors[NUM_COLORS - 1] << "\n\t  ====== Welcome to the chat-room ======   " << endl << def_col;

    while (1) {
        // Accept incoming connection
        if ((client_socket = accept(server_socket, (struct sockaddr *)&client, &len)) == -1) {
            perror("accept error: ");
            exit(-1);
        }
        seed++;
        // Create thread to handle client
        thread t(handle_client, client_socket, seed);
        // Lock clients array and add new client
        lock_guard<mutex> guard(clients_mtx);
        if (num_clients < MAX_CLIENTS) {
            clients[num_clients++] = {seed, "Anonymous", client_socket, std::make_unique<std::thread>(std::move(t))};
        }
        else {
            cerr << "Maximum number of clients reached. Can't accept more connections." << endl;
            close(client_socket); // Close the socket for this client
        }
    }

    // Join threads and close server socket
    for (int i = 0; i < num_clients; i++) {
        if (clients[i].th->joinable())
            clients[i].th->join();
    }
    close(server_socket);
    return 0;
}

// Function to get ANSI color code
string color(int code) {
    return colors[code % NUM_COLORS];
}

// Function to set client's name
void set_name(int id, char name[]) {
    for (int i = 0; i < num_clients; i++) {
        if (clients[i].id == id) {
            clients[i].name = string(name);
        }
    }
}

// Function for thread-safe printing
void shared_print(string str, bool endLine) {
    lock_guard<mutex> guard(cout_mtx);
    cout << str;
    if (endLine)
        cout << endl;
}

// Function to broadcast message to all clients except sender
void broadcast_message(string message, int sender_id) {
    char temp[MAX_LEN];
    strcpy(temp, message.c_str());
    for (int i = 0; i < num_clients; i++) {
        if (clients[i].id != sender_id) {
            send(clients[i].socket, temp, strlen(temp), 0);
        }
    }
}

// Function to end connection with a client
void end_connection(int id) {
    for (int i = 0; i < num_clients; i++) {
        if (clients[i].id == id) {
            lock_guard<mutex> guard(clients_mtx);
            clients[i].th->detach(); // Detach the thread
            close(clients[i].socket);
            for (int j = i; j < num_clients - 1; j++) {
                clients[j] = std::move(clients[j + 1]); // Move the elements after deletion
            }
            num_clients--; // Decrement the count of clients
            break;
        }
    }
}

// Function to handle individual client communication
void handle_client(int client_socket, int id) {
    char name[MAX_LEN], str[MAX_LEN];
    recv(client_socket, name, sizeof(name), 0);
    set_name(id, name);

    // Display welcome message
    string welcome_message = string(name) + " has joined";
    broadcast_message("#NULL", id);
    broadcast_message(to_string(id), id); // Convert id to string
    broadcast_message(welcome_message, id);
    shared_print(color(id) + welcome_message + def_col);

    while (true) {
        int bytes_received = recv(client_socket, str, sizeof(str), 0);
        if (bytes_received <= 0)
            return;

        // Null-terminate the received message
        str[bytes_received] = '\0';

        if (strcmp(str, "#exit") == 0) {
            // Display leaving message
            string message = string(name) + " has left";
            broadcast_message("#NULL", id);
            broadcast_message(to_string(id), id); // Convert id to string
            broadcast_message(message, id);
            shared_print(color(id) + message + def_col);
            end_connection(id);
            return;
        }

        // Encrypt the received message before broadcasting it
        string encrypted_message = encryptCaesarCipher(str, 3); // Encrypt using Caesar Cipher with shift 3

        // Display the encrypted message in the server terminal
        shared_print(color(id) + name + " : " + def_col + encrypted_message); // Display encrypted message only

        // Broadcast the original message along with sender's ID and encrypted message
        string structured_message = string(name) + "|" + to_string(id) + "|" + encrypted_message;
        broadcast_message(structured_message, id);
    }
}

// Function to encrypt text using Caesar Cipher
string encryptCaesarCipher(const string& text, int shift) {
    string result = text;
    for (size_t i = 0; i < text.length(); ++i) {
        if (isalpha(text[i])) {
            if (isupper(text[i])) {
                result[i] = 'A' + (text[i] - 'A' + shift) % 26; // Apply Caesar Cipher encryption for uppercase letters
            } else {
                result[i] = 'a' + (text[i] - 'a' + shift) % 26; // Apply Caesar Cipher encryption for lowercase letters
            }
        }
    }
    return result;
}
