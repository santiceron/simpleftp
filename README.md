# simpleftp
This project consists of a simple FTP client-server application implemented in C. The client (myftp) connects to the server (myftpsrv) over TCP/IP, and supports operations like user authentication, file retrieval (get), and quitting the session (quit). The server manages connections from multiple clients, authenticates users, and handles file retrieval requests.

After compiling client and server execute with the following formats:

Server: ./mysrv <SERVER_PORT>

Client: ./myftp <SERVER_IP> <SERVER_PORT>
