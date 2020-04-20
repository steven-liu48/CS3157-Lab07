#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ctype.h>

static void die(const char *s) { perror(s); exit(1); }


int main(int argc, char **argv)
{
    if (argc != 5) {
        fprintf(stderr, "usage: %s <server_port> <web_root> <mdb-lookup-host> <mdb-lookup-port>\n", argv[0]);
        exit(1);
    }
	
	unsigned short server_port = atoi(argv[1]);
	char *web_root = argv[2];
	char *host = argv[3];
	unsigned short mdb_port = atoi(argv[4]);
	
    // Create a listening socket (also called server socket) 

    int servsock;
    if ((servsock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        die("socket failed");
    
    // Construct local address structure
    
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // any network interface
    servaddr.sin_port = htons(server_port);

    // Bind to the local address

    if (bind(servsock, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
        die("bind failed");

    // Start listening for incoming connections

    if (listen(servsock, 5 /* queue size for connection requests */ ) < 0)
        die("listen failed");

    int clntsock;
    socklen_t clntlen;
    struct sockaddr_in clntaddr;

	while (1){
				
		// Accept an incoming connection

        clntlen = sizeof(clntaddr); // initialize the in-out parameter

        if ((clntsock = accept(servsock,(struct sockaddr *) &clntaddr, &clntlen)) < 0)
            die("accept failed");
		
		// Get stuff

	    FILE *request = fdopen(clntsock, "rb");

	    // Process the first line
		
		char firstLine[1000];
		fgets(firstLine, 1000, request);

		// Parse the request line

		char *token_separators = "\t \r\n"; // tab, space, new line
		char *method = strtok(firstLine, token_separators);
		char *requestURI = strtok(NULL, token_separators);
		char *httpVersion = strtok(NULL, token_separators);
		
		char response[1000];

		// Check GET
		
		if (!strcmp(method, "GET")){
			
		} else {
			//printf("GET Err\n");
			printf("%s \"GET %s\" 501 Not Implemented\n", inet_ntoa(clntaddr.sin_addr), requestURI);
			sprintf(response, "HTTP/1.0 501 Not Implemented\r\n\r\n<html><body><h1>501 Not Implemented</h1></body></html>\r\n");
			send(clntsock, response, strlen(response), 0);
			close(clntsock);
			continue;
		}
		
		// Check URI

		if (requestURI[0] == '/' && !strstr(requestURI, "/..")){
			
		} else {
			//printf("URI Err\n");
			printf("%s \"GET %s\" 400 Bad Request\n", inet_ntoa(clntaddr.sin_addr), requestURI);
			sprintf(response, "HTTP/1.0 400 Bad Request\r\n\r\n<html><body><h1>400 Bad Request</h1></body></html>\r\n");
            send(clntsock, response, strlen(response), 0);
			close(clntsock);
			continue;
		}
		
		// Check HTTP 1.0

        if (!strcmp(httpVersion, "HTTP/1.1") || !strcmp(httpVersion, "HTTP/1.0")){
          	
        } else {
            //printf("HTTP Ver Err\n");
			printf("%s \"GET %s\" 400 Bad Request\n", inet_ntoa(clntaddr.sin_addr), requestURI);
            sprintf(response, "HTTP/1.0 400 Bad Request\r\n\r\n<html><body><h1>400 Bad Request</h1></body></html>\r\n");
            send(clntsock, response, strlen(response), 0);
			close(clntsock);
			continue;
        }
		
		// Check the request file path

		char filePath[1000];
		sprintf(filePath, "%s%s", web_root, requestURI);
		
		struct stat st;
		stat(filePath, &st);
		int dir = S_ISREG(st.st_mode);
		uint32_t size = st.st_size;
		//printf("%s: %d\n", filePath, st);

		if (dir == 0){ // If the path is a directory
			if (requestURI[strlen(requestURI)-1] != '/'){ // If the path is directory but does not have '/' at the end
				//printf("[%c]\n", requestURI[strlen(requestURI)-1]);
        		printf("%s \"GET %s\" 301 Moved Permanently\n", inet_ntoa(clntaddr.sin_addr), requestURI);
            	sprintf(response, "HTTP/1.0 301 Moved Permanently\r\nLocation: http://clac.cs.columbia.edu%s/\r\n\r\n<html><body><h1>301 Moved Permanently</h1></body></html>\r\n", requestURI);
            	send(clntsock, response, strlen(response), 0);
            	close(clntsock);
				continue;
			} else { // If the path is directory and has '/' at the end
				sprintf(filePath, "%s%sindex.html", web_root, requestURI); // Add index.html to the end
			}
		}

		// Open the file and send the response

		FILE * file;
		file = fopen(filePath, "r");
		
		if (file){ // File exists
			// Send the success message
			sprintf(response, "HTTP/1.0 200 OK\r\n\r\n");
			send(clntsock, response, strlen(response), 0);

			// Print out the success message
			printf("%s \"GET %s\" 200 OK\n", inet_ntoa(clntaddr.sin_addr), requestURI);

			// Send the file
			char buffer[4096];
			int i;
    		while (1){
        		i = fread(buffer, 1, 4096, file);
				send(clntsock, buffer, i, 0);
				//printf("sob: %d", sizeof(buffer));
        		//fwrite(buffer, 1, i, fp);
        		if (i < 4096) {
					sprintf(buffer, "\r\n");
					send(clntsock, buffer, strlen(buffer), 0);
					break;	
				}
				
			}
			// Close the file
			fclose(file);
		} else { // File does not exist
			printf("%s \"GET %s\" 404 Not Found\n", inet_ntoa(clntaddr.sin_addr), requestURI);
            sprintf(response, "400 Not Found\r\n\r\n<html><body><h1>404 Not Found</h1></body></html>\r\n");
			send(clntsock, response, strlen(response), 0);
            close(clntsock);
			continue;
		}

		// Close file pointer
		fclose(request);

		//Clean up
		close(clntsock);
	}
}