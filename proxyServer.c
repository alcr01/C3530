#include "proxyServer.h"

// Prints out the error and exits
void error(const char *msg){
	perror(msg);
	exit(-1);
}

int blacklist_check(char* URL){
	FILE *ofp;
	char* buffer=malloc(BUFFSIZE);
	size_t length=0;
	ofp=fopen("blacklist.txt","r");
	while(getline(&buffer,&length,ofp)!=-1){
		if(buffer[strlen(buffer)-1]=='\n')
			buffer[strlen(buffer)-1]='\0';
		if(strcasecmp(buffer,URL)==0){
			printf("Blacklisted site! Sending blacklist response.\n");
			return -1;
		}
	}
	fclose(ofp);
	return 0;
}

void* handle_client(void* socket_desc){
	char *buffer = malloc(BUFFSIZE);// Buffer where everything will be saved
	char* request=malloc(BUFFSIZE);
	char* response=malloc(BUFFSIZE);
	char* path;
	char* host;
	char *URL;
	int socket_client=*(int*)socket_desc;
	int byteRead; 
	int socket_server;
	int i;
	struct sockaddr_in server_address;		// Servers connection
	struct hostent *server_URL;
	int size_of_response;
	pthread_mutex_t   mutex_lock = PTHREAD_MUTEX_INITIALIZER;
	FILE* ofp;
	if((byteRead=recv(socket_client, buffer, BUFFSIZE, 0)) <= 0){
		close(socket_client);
	    error("ERROR recieving request.");
	}
	printf("Recieved request!\n");
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
	path=malloc(strlen(URL));
	host=malloc(strlen(URL));
	for(i=0;i<strlen(URL);i++){
		if(URL[i]=='/'){
			strncpy(host,URL,i);
			strncpy(path,URL+i+1,strlen(URL)-i);
			break;
		}
		else if(i==strlen(URL)-1){
			strncpy(host,URL,strlen(URL));
			break;
		}
	}
	response[0]='\0';
	buffer[0]='\0';
	if(blacklist_check(URL)==-1){
		strcat(response,"HTTP/1.1 200 OK\r\n");
		strcat(response,"Content-Type: text/html\r\n");
		strcat(response,"Connection: close\r\n");
		strcat(response,"\r\n<p>Cannot access Website. Blacklisted!</p>\r\n\r\n");
		send(socket_client,response,strlen(response),0);
	}
	else if(!access(URL,F_OK)){
		printf("FILE EXISTS\n");
		// The file exists
		for(i=0;i<strlen(URL)+1;i++){
    		if(URL[i]=='/')
    			URL[i]='_';
    	}
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
		server_URL = gethostbyname(host);
		if(server_URL == NULL){
			close(socket_client);
			error("Couldn't get an address");
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
		strcat(request,"GET /");
		strcat(request,path);
		strcat(request," HTTP/1.1\r\n");
		strcat(request,"Host: ");
		strcat(request,host);
		strcat(request,"\r\n");
		strcat(request,"Connection: close\r\n");
		strcat(request,"If-Modified-Since: 0\r\n\r\n\0");
		if (write(socket_server, request, strlen(request)) < 0){
			close(socket_client);
			close(socket_server);
   		 	error("ERROR on server request");
   		}
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
    	FILE* ofp;
    	for(i=0;i<strlen(URL)+1;i++){
    		if(URL[i]=='/')
    			URL[i]='_';
    	}
    	pthread_mutex_lock(&mutex_lock);
    	ofp = fopen(URL, "w");	// Opens the file
		// Checks for file error
		if(ofp == NULL) {
			close(socket_client);
			close(socket_server);
			error("ERROR accessing cache!");
		}
		fprintf(ofp, "%s\n", buffer);
		fclose(ofp);
		pthread_mutex_unlock(&mutex_lock);
    	if(write(socket_client,response,strlen(response))<0){
    		close(socket_client);
    		close(socket_server);
    		error("ERROR on sending response to client");
    	}
    	printf("Response sent to client!\n");
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
		if(pthread_create(&thread,NULL,handle_client,(void *)&socket_client)!=0){
			close(socket_listen);
			close(socket_client);
			error("ERROR on threading client.");
		}
	}
}
