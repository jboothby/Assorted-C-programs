This is an implementation of a simple File Transfer Protocol (FTP) System
It uses linux system calls to interface with the file system. It is a culmination
of lessons learned from the other programs in this repo and implements the following:

IPv4 Internet sockets
Multi-Process execution using fork()
Pipes
I/O Redirection

Interface: 

client serverIP <-d>
    
    serverIP is a symbolic host name or an IPv4 dotted-decimal address to the server
    -d: debugging flag that enables verbose output
    

server <-d> <-p portnum>

    -d: debugging flag that enables verbose output
    -p portnum: portnum is the desired port to make the server on


After running the client program and a successful connection to the server is made, the following
commands may be entered into the client when prompted by the server

exit            - terminate all child processes of the server and then the client
cd <pathname>   - change the current working directory of the client process to <pathname>
rcd <pathname>  - change the current working directory of the server process to <pathname>
ls              - execute the ls -l command on the client, show output 20 lines at a time. press space to see next 20 lines
rls             - execute the ls -l command on the server, show output 20 lines at a time. press space to see next 20 lines
get <pathname>  - retrieve <pathname> from the server and store it locally in the clients cwd. Must be a regular file, and readable
show <pathname> - retrive the contents of <pathname> and show 20 lines at a time. Press space to show more. Must be regular file and readable.
put <pathname>  - transmite the contents of <pathname> to server and store in server cwd. Must be a regular file and readable.
