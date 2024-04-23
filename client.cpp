#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <thread>
#include <signal.h>
#include <mutex>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

// Define constants
#define MAX_LEN 200
#define NUM_COLORS 6

using namespace std;

// Global variables
bool exit_flag = false; // Flag to indicate if program should exit
thread t_send, t_recv; // Threads for sending and receiving messages
int client_socket; // Socket for client-server communication
string def_col = "\033[0m"; // Default color for output
string colors[] = {"\033[31m", "\033[32m", "\033[33m", "\033[34m", "\033[35m", "\033[36m"}; // Array of ANSI color codes

// Mutex for thread synchronization
mutex mtx;

// Function prototypes
void catch_ctrl_c(int signal); // Function to handle CTRL+C signal
string color(int code); // Function to get ANSI color code based on input code
void eraseText(int cnt); // Function to erase text from the console
void send_message(int client_socket); // Function to send messages to the server
void recv_message(int client_socket); // Function to receive messages from the server
string decryptCaesarCipher(const string& text, int shift); // Function to decrypt text using Caesar Cipher
string encryptCaesarCipher(const string& text, int shift); // Function to encrypt text using Caesar Cipher
bool verifyLogin(const string& username, const string& password); // Function to verify user login credentials
bool userExists(const string& username); // Function to check if a user already exists

// Main function
int main() {
    // Create a socket for communication with the server
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket: "); // Print error if socket creation fails
        exit(-1);
    }

    // Define server address
    struct sockaddr_in client;
    client.sin_family = AF_INET; // Address family is Internet
    client.sin_port = htons(10000); // Port no. of server
    client.sin_addr.s_addr = INADDR_ANY; // Connect to any available address
    bzero(&client.sin_zero, 0); // Fill remaining space with zeros

    // Connect to the server
    if ((connect(client_socket, (struct sockaddr *)&client, sizeof(struct sockaddr_in))) == -1) {
        perror("connect: "); // Print error if connection fails
        exit(-1);
    }

    // Register signal handler for CTRL+C
    signal(SIGINT, catch_ctrl_c);

    // Get user's name
    char name[MAX_LEN];
    cout << "Enter your name : ";
    cin.getline(name, MAX_LEN);

    // Send user's name to the server
    send(client_socket, name, sizeof(name), 0);

    // Display welcome message
    cout << colors[NUM_COLORS - 1] << "\n\t  ====== Welcome to the chat-room ======   " << endl << def_col;

    // Prompt user to sign in or sign up
    string choice;
    bool valid_choice = false;
    while (!valid_choice) {
        cout << "Do you want to sign in or sign up? (1/2): ";
        getline(cin, choice);

        if (choice == "1" || choice == "2") {
            valid_choice = true;
        } else {
            cout << "Invalid choice. Please enter '1' for sign in or '2' for sign up." << endl;
        }
    }

    // Handle user sign in or sign up
    if (choice == "1") {
        // Sign in process
        string username, password;
        bool valid_credentials = false;
        while (!valid_credentials) {
            cout << "Username: ";
            getline(cin, username);
            cout << "Password: ";
            getline(cin, password);

            // Verify user login credentials
            if (verifyLogin(username, password)) {
                valid_credentials = true;
            } else {
                cout << "Invalid username or password. Please try again." << endl;
            }
        }
        cout << "Login successful." << endl;
    } else if (choice == "2") {
        // Sign up process
        string username, password;
        bool valid_username = false;
        while (!valid_username) {
            cout << "Username: ";
            getline(cin, username);
            if (!userExists(username)) {
                valid_username = true;
            } else {
                cout << "User already exists. Please choose a different username." << endl;
            }
        }

        // Get user's password
        bool valid_password = false;
        while (!valid_password) {
            cout << "Password: ";
            getline(cin, password);
            // Additional validation for password can be added here if needed
            valid_password = true;
        }

        // Encrypt the password before saving it to the file
        password = encryptCaesarCipher(password, 3); // Encrypt using Caesar Cipher with shift 3

        // Save user credentials (for demonstration purposes)
        ofstream file("users.txt", ios::app);
        if (file.is_open()) {
            file << username << ":" << password << endl;
            file.close();
            cout << "User successfully registered." << endl;
        } else {
            cerr << "Error: Unable to open users.txt for writing." << endl;
            return 1;
        }
    }

    // Start threads for sending and receiving messages
    thread t1(send_message, client_socket);
    thread t2(recv_message, client_socket);

    // Move threads to global variables
    t_send = move(t1);
    t_recv = move(t2);

    // Wait for threads to finish
    if (t_send.joinable())
        t_send.join();
    if (t_recv.joinable())
        t_recv.join();

    return 0;
}

// Signal handler for CTRL+C
void catch_ctrl_c(int signal) {
    char str[MAX_LEN] = "#exit";
    send(client_socket, str, sizeof(str), 0); // Send exit command to server
    exit_flag = true; // Set exit flag to true
    t_send.detach(); // Detach send thread
    t_recv.detach(); // Detach receive thread
    close(client_socket); // Close socket
    exit(signal); // Exit program
}

// Function to get ANSI color code
string color(int code) {
    return colors[code % NUM_COLORS];
}

// Function to erase text from the console
void eraseText(int cnt) {
    char back_space = 8;
    for (int i = 0; i < cnt; i++) {
        cout << back_space;
    }
}

// Function for sending messages to the server
void send_message(int client_socket) {
    while (1) {
        cout << colors[1] << "You : " << def_col; // Display user prompt
        char str[MAX_LEN];
        cin.getline(str, MAX_LEN); // Get user input

        // Check if the message is for exit
        if (strcmp(str, "#exit") == 0) {
            exit_flag = true; // Set exit flag to true
            // Detach threads and close socket
            t_recv.detach();
            close(client_socket);
            return;
        }

        // Send the message without encryption
        send(client_socket, str, strlen(str), 0);
    }
}

// Function for receiving messages from the server
void recv_message(int client_socket) {
    while (1) {
        if (exit_flag)
            return;
        
        char structured_message[MAX_LEN];
        int bytes_received = recv(client_socket, structured_message, sizeof(structured_message), 0);
        if (bytes_received <= 0)
            continue;

        // Null-terminate the received message
        structured_message[bytes_received] = '\0';

        // Split the structured message into its components
        string received_message(structured_message);
        size_t delim_pos1 = received_message.find("|");
        size_t delim_pos2 = received_message.find("|", delim_pos1 + 1);
        if (delim_pos1 != string::npos && delim_pos2 != string::npos) {
            string sender_name = received_message.substr(0, delim_pos1);
            int sender_id = stoi(received_message.substr(delim_pos1 + 1, delim_pos2 - delim_pos1 - 1));
            string encrypted_message = received_message.substr(delim_pos2 + 1);

            // Decrypt the encrypted message
            string decrypted_message = decryptCaesarCipher(encrypted_message, 3);

            // Print the decrypted message
            eraseText(6);
            cout << color(sender_id) << sender_name << " : " << def_col << decrypted_message << endl;
            cout << colors[1] << "You : " << def_col;
            fflush(stdout);
        }
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

// Function to decrypt text using Caesar Cipher
string decryptCaesarCipher(const string& text, int shift) {
    string result = text;
    for (size_t i = 0; i < text.length(); ++i) {
        if (isalpha(text[i])) {
            if (isupper(text[i])) {
                result[i] = 'A' + (text[i] - 'A' + 26 - shift) % 26; // Apply Caesar Cipher decryption for uppercase letters
            } else {
                result[i] = 'a' + (text[i] - 'a' + 26 - shift) % 26; // Apply Caesar Cipher decryption for lowercase letters
            }
        }
    }
    return result;
}

// Function to verify user login credentials
bool verifyLogin(const string& username, const string& password) {
    ifstream file("users.txt"); // Open file containing user credentials
    string line;
    while (getline(file, line)) {
        size_t pos = line.find(':');
        if (pos != string::npos) {
            string existingUsername = line.substr(0, pos);
            if (existingUsername == username) {
                string encryptedPassword = line.substr(pos + 1);

                // Decrypt the stored encrypted password
                string storedPassword = decryptCaesarCipher(encryptedPassword, 3); // Assuming Caesar Cipher with shift 3

                // Compare the decrypted password with the entered password
                if (storedPassword == password) {
                    file.close(); // Close file
                    return true; // Return true if login is successful
                }
            }
        }
    }
    file.close(); // Close file
    return false; // Return false if login fails
}

// Function to check if a user already exists
bool userExists(const string& username) {
    ifstream file("users.txt"); // Open file containing user credentials
    string line;
    while (getline(file, line)) {
        size_t pos = line.find(':');
        if (pos != string::npos) {
            string existingUsername = line.substr(0, pos);
            if (existingUsername == username) {
                file.close(); // Close file
                return true; // Return true if user exists
            }
        }
    }
    file.close(); // Close file
    return false; // Return false if user does not exist
}
