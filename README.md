README
------Client------<br>
The client runs a looped menu. When a user inputs a command, the parameters are parsed and the client tries to connect to each of the servers listed in the configuration file. 
Upon, successful connection, a thread is spawned to handle that connection. A message is sent to the server and receieved from the server. Received messages from each server are stored in a linked list in dynamic memory.
Then, once all threads are finished, the linked list is parsed into two more linked lists. One linked list has a node for each filename received in list. The other linked list has a node for each file chunk recieved for a given file. 
To check if a file is complete and to piece a file back together, this linked list is parsed. The chunk number of each received chunk is summed and checked to see if all chunks were received.

For PUT, XOR encryption is performed with the user's password. A system call to MD5 is made to compute the hash of the file and delegate the chunks to the servers according to the instructions.

-------Server------<br>
Upon connection to a client, a thread is spawned so the server can handle concurrent TCP connections with multiple clients. For PUT, the server reads from the socket and stores both received file chunks after parsing the received message to extract the chunk number. For GET the server crawls the server dir/user dir fo the filename, and sends the contents back to the client with the chunk number pre-pended. For LIST, the server sends each found filename back to the client. The client then parses the filename to extract the chunk number.

Compilation commands:
gcc dfclient.c ClientHelper.c -o client -pthread
gcc ServerHelper.c dfserver.c -o server
