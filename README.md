# Chat Application Guide
## Table of Contents
Program Compiling  
Launching The Server  
Launching The Client  
Exiting The Program  
Program Compiling  



The first step in using the chat application is to compile both the server and client program files. Follow these instructions:

Open a terminal in the program folder path.
Enter the following commands:
bash  

Copy code  

g++ server.cpp -lpthread -o server    
g++ client.cpp -lpthread -o client  

The first command compiles the server program file, generating the executable, while the second command does the same for the client version. Once completed, the application is ready to use.

## Launching The Server
To launch the server for enabling communication among users, follow these steps:

Open a terminal in the directory where the application is stored.
Run the command ./server.
The server will be activated and display a welcome message.
The server code contains sockets that listen for client requests and facilitates connections. It uses multithreading to handle multiple clients simultaneously, ensuring smooth communication without blocking.

## Launching The Client
After the server is up and running, follow these steps to establish a connection:

Open a terminal window in the application directory.
Type the command ./client.
The client will be greeted with a welcome message and prompted to enter their name.
Choose between signing in or signing up.
## Signup:  
 Enter a desired username and password. Upon successful registration, a confirmation message will be received, granting access to the chat room.
## Sign In: 
Enter valid credentials. Incorrect credentials will prompt the user to try again. Upon successful authentication, access to the room is granted.
Once connected to the server, users can send messages visible to other clients.

## Exiting The Program
Exiting the program is straightforward:

Press CTRL+C to exit the shell, terminating the active session.
Upon termination, the client socket is shut down, and resources are reallocated.
