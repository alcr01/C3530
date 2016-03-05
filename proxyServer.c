#include "proxyServer.h"

// Prints out the error and exits
void error(const char *msg){
	perror(msg);
	exit(-1);
}

void* handle_client(void* socket_desc){
	char *buffer = malloc(BUFFSIZE);// Buffer where everything will be saved
	char* request=malloc(BUFFSIZE);
	char* response=malloc(BUFFSIZE);
	char *URL;
	int socket_client=*(int*)socket_desc;
	int byteRead; 
	int socket_server;
	struct sockaddr_in server_address;		// Servers connection
	struct hostent *server_URL;
	int size_of_response;
	pthread_mutex_t   mutex_lock = PTHREAD_MUTEX_INITIALIZER;
	FILE* ofp;
	if((byteRead=recv(socket_client, buffer, BUFFSIZE, 0)) <= 0){
		close(socket_client);
	    error("ERROR recieving request.");
	}
 	buffer[byteRead] = '\0';//adds in NULL terminator as recv does not do this.
 	URL=malloc(strlen(buffer)+1);
 	strcpy(URL,buffer);
 	URL = strtok(URL, " ");	// Makes tokens out of the website
	// String compare to get the GET line in website 
	while(strcmp(URL, "GET")!=0) 
		URL = strtok(NULL," \r\n");
	URL = strtok(NULL," ");	// Make another token to isolate the web address
	if (URL[0] == '/')
		memmove(URL, URL + 1, strlen(URL));
	if(!access(URL,F_OK)){
		printf("FILE EXISTS\n");
		// The file exists
		ofp = fopen(URL, "r");	// Opens the file
		// Checks for file error
		if(ofp == NULL){
			close(socket_client);
			error("Can't open file!");
		}
		fseek(ofp, SEEK_SET, 0);			// Seek to the beg. of file
		fread(buffer, BUFFSIZE + 1, 1, ofp);	// Read the file into buffer
		fclose(ofp);
		write(socket_client, buffer, BUFFSIZE);		// Prints out the saved file to the client
		printf("Sent cached data to client!\n");
	}
	else{
		memset(&server_address, 0, sizeof(server_address));		// Clears up server_address so that there is nothing
		server_address.sin_port= htons(80);
		server_address.sin_family = AF_INET;
		if((server_URL = gethostbyname(URL)) == NULL){
			close(socket_client);
			error("Couldn't get an address\n");
		}
		printf("Server address found!\n");
		if((socket_server = socket(AF_INET, SOCK_STREAM, 0)) <0){
			close(socket_client);
			error("ERROR on Server Socket Creation!\n");
		}
		printf("Server socket created!\n");
		// Will create the server socket that will connect to the internet
		// Will get the address information for the website (ie IP address)
		// Copies the IP address (that is used to get the website) and stores it in the server_address's address
		memcpy((char *) &server_address.sin_addr.s_addr, (char *) server_URL->h_addr,server_URL->h_length);
		// Opens the HTTP port needed to connect to the internet
		// Connect's the server to the internet
		if(connect(socket_server, (struct sockaddr *) &server_address, sizeof(server_address)) != 0){
			close(socket_client);
			close(socket_server);
			error("Couldn't connect\n");
		}
		printf("Connection made to server.\n");
		request[0]='\0';
		strcat(request,"GET / HTTP/1.1\r\n");
		strcat(request,"Host: ");
		strcat(request,URL);
		strcat(request,"\r\n");
		strcat(request,"Connection: close\r\n");
		strcat(request,"If-Modified-Since: 0\r\n\r\n\0");
		if (write(socket_server, request, strlen(request)) < 0){
			close(socket_client);
			close(socket_server);
   		 	error("ERROR on server request");
   		}
	   	response[0]='\0';
	   	buffer[0]='\0';
	   	do{	
		   	if((size_of_response=read(socket_server,buffer,BUFFSIZE))<0){
		   		close(socket_client);
		   		close(socket_server);
    			error("ERROR on server response");
    		}
    		else if(size_of_response>0){
    			strcat(response,buffer);
    		}
    	}while(size_of_response>0);
    	printf("Server response recieved!\n");
    	pthread_mutex_lock(&mutex_lock);
    	FILE* ofp;
    	ofp = fopen(URL, "w");	// Opens the file
		// Checks for file error
		if(ofp == NULL) {
			close(socket_client);
			close(socket_server);
			error("ERROR accessing cache!");
		}
		fprintf(ofp, "%s\n", buffer);
		fclose(ofp);
    	if(write(socket_client,response,strlen(response))<0){
    		close(socket_client);
    		close(socket_server);
    		error("ERROR on sending response to client");
    	}
    	printf("Response sent to client!\n");
    	pthread_mutex_unlock(&mutex_lock);
    	close(socket_server);
    }
    close(socket_client);
}	


int main(){
	// Variables
	int socket_listen,socket_client;
	int* socket_temp;
	socklen_t listen_addrlen;
	struct sockaddr_in socket_client_addr,listen_addr;
	pthread_t thread;
	int portNo;
			// Asks the user to enter a port number to be used
	printf("Port No: ");
	scanf("%d", &portNo);

	// Creates the socket and checks if it created successfully
	if((socket_listen = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		error("ERROR on socket creation.");	
	}
	printf("Socket created!\n");
	memset(&listen_addr, 0, sizeof(listen_addr));
	
	// Something to do with sockets and port access (binding)
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_addr.s_addr = INADDR_ANY;
	listen_addr.sin_port = htons(portNo);

		//Binds the port to socket
	if(bind(socket_listen, (struct sockaddr *) &listen_addr, sizeof(listen_addr)) == -1){
		close(socket_listen);
		error("ERROR on binding.");
	}
	printf("Succesfully Bound!\n");
	// Checks if socket is listening on the port
	if(listen(socket_listen, 30) == -1){
		close(socket_listen);
		error("ERROR on Listening!");
	}
	printf("Listening...\n");
	// Checks if the client is able to connect
	listen_addrlen=sizeof(listen_addr);
	while(1){
		if((socket_client=accept(socket_listen,(struct sockaddr*)&socket_client_addr,&listen_addrlen))==-1){
			close(socket_listen);
			error("ERROR accepting client.");
		}
//		socket_temp=malloc(sizeof(int));
//		*socket_temp=socket_client;
		if(pthread_create(&thread,NULL,handle_client,(void *)&socket_client)!=0){
			close(socket_listen);
			close(socket_client);
			error("ERROR on threading client.");
		}
	}


/*	while(1){
		if((socket_client = accept(socket_listen, (struct sockaddr *) &addr, &addrlen)) < 0){
			close(socket_listen);
			error("ERROR on accept!");
		}
		printf("Client connected!\n");
		// Will recieve something from the client
		int byteRead = recv(socket_client, buffer, BUFFSIZE, 0);
	    if(byteRead <= 0)
	       error("Unable to recieve connection. Exiting...");

 	   buffer[byteRead] = '\0';//adds in NULL terminator as recv does not do this.

	    strcpy(website,buffer);	// Copy the buffer to use tokens
		website = strtok(website, " ");	// Makes tokens out of the website
			// String compare to get the GET line in website 
		while(strcmp(website, "GET")!=0) 
			website = strtok(NULL," \r\n");
		website = strtok(NULL," ");	// Make another token to isolate the web address
		if (website[0] == '/')
			memmove(website, website + 1, strlen(website));
	 	
			// Removes the forward slash that is retrieved as part of the website
		/*******************************************************************************************
		// Checks if the file exists and goes on accordingly
		if(!access(website, F_OK)) {
			printf("FILE EXISTS\n");
			// The file exists
			ofp = fopen(website, "r");	// Opens the file
				// Checks for file error
			if(ofp == NULL) 
				error("Can't open file!");
			fseek(ofp, SEEK_SET, 0);			// Seek to the beg. of file
			fread(buffer, BUFFSIZE + 1, 1, ofp);	// Read the file into buffer
			fclose(ofp);
			write(socket_client, buffer, BUFFSIZE);		// Prints out the saved file to the client
		}
		else{ // File doesn't exist
			// SERVER CONNECTS TO INTERNET
		// Creates the socket and checks if it created successfully
			if((server = socket(AF_INET, SOCK_STREAM, 0)) <0)
				error("ERROR on Server Socket Creation!\n");
			printf("Server socket created!\n");
			// Will create the server socket that will connect to the internet
	   	 	server = socket(AF_INET, SOCK_STREAM, 0);
	    	memset(&server_address, 0, sizeof(server_address));		// Clears up server_address so that there is nothing
	    	// Will get the address information for the website (ie IP address)
			if((getURL = gethostbyname(website)) == NULL)
				error("Couldn't get an address\n");
			printf("Got an address.\n");
			// Copies the IP address (that is used to get the website) and stores it in the server_address's address
			memcpy((char *) &server_address.sin_addr.s_addr, (char *) getURL->h_addr, getURL->h_length);
			// Opens the HTTP port needed to connect to the internet
			server_address.sin_port= htons(80);
			server_address.sin_family = AF_INET;
			// Connect's the server to the internet
			if(connect(server, (struct sockaddr *) &server_address, sizeof(server_address)) != 0)
				error("Couldn't connect\n");
			printf("Connection made to server.\n");	
			// Sending HTTP info
			// Client request info (client->server)
			strcpy(request,"GET / HTTP/1.1");
			strcat(request,"\r\nHost: ");
			strcat(request,website);
			strcat(request,"\r\nConnection: close");
			strcat(request,"\r\n\r\n");
			// Server's request to the internet (server->internet)
			write(server, request, strlen(request));
			// Server recieving request (internet->server)
			read(server, buffer, BUFFSIZE);
			close(server);
			printf("Response Recieved! Sending information to client...\n");	

			write(socket_client,buffer,BUFFSIZE);
			// Saves the info recieved from the internet (internet->file)
			ofp = fopen(website, "w");
			// Check if there is an error
			if(ofp == NULL)	
				error("Can't open file for cache!");	
			// Prints to the file
			fprintf(ofp, "%s\n", buffer);
			// Writes to client (server->client)
		}
	}
	close(socket_listen);
    close(socket_client);//closes connection to the host
    free(buffer);
    free(request);
	return 0;*/
}