/*
 *Kenneth Kiprotich
 */
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

//sample order of args: (argv[0]) ./http-client (1)www.gnu.org (2)80 (3)/software/make/manual/make.html

//john wick for the program...
static void die(const char *s) { perror(s); exit(1); }

int main(int argc, char **argv) {

        unsigned short port = atoi(argv[2]);

        if(argc != 4 || isalpha(port) == 0) {
                printf("%s\n", "usage: http-client <host name> <port number> <file path>");
                printf("%s\n", "       ex. http-client www.example.com 80 /index.html");
                exit(1);
        }

        FILE *file = NULL;

        //getting the port number to int type
        //unsigned short port = atoi(argv[2]);

        //retrieving file name for storing the downloaded stuff
        char *name;
        if (strrchr(argv[3], '/') != NULL)
                name = strrchr(argv[3], '/');
        else {
                printf("%s\n", "usage: http-client <host name> <port number> <file path>");
                printf("%s\n", "       ex. http-client www.example.com 80 /index.html");
                exit(0);
        }
        //error check to make sure file path is good, and filename can be retrieved
        if( (strcmp(name, "/") == 0) ) {
                printf("%s", "can't open file: there's no such file or directory\n");
                exit(0);
        }

        char *fname = name+1;


        /* converting host name to ip address (code given by Jae in lab6 description)*/
        struct hostent *he;
        char *serverName = argv[1];
        // get server ip from server name
        if ((he = gethostbyname(serverName)) == NULL) {
            die("gethostbyname failed");
        }
        char *serverIP = inet_ntoa(*(struct in_addr *)he->h_addr);
        /* Jae's provided code ends here */


        // Create a socket for TCP connection
        int sock; // socket descriptor
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                die("socket failed");

        // Construct a server address structure
        struct sockaddr_in servaddr;
        memset(&servaddr, 0, sizeof(servaddr)); // must zero out the structure
        servaddr.sin_family      = AF_INET;
        servaddr.sin_addr.s_addr = inet_addr(serverIP); //serverIP is from the additional code snippetby Jae
        servaddr.sin_port        = htons(port); // from commandline (usually 80 for web-servers)


        // Establish a TCP connection to the server
        if (connect(sock, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
                die("connect failed");


        //prep http request to send to server
        int stringsize = 1024;
        char sending[stringsize];
        char receiving[stringsize];
        int sendsize, n;

        sprintf(sending,  "GET %s HTTP/1.0\r\nHost: %s:%s\r\n\r\n", argv[3], argv[1], argv[2]);
        sendsize = strlen(sending);

        //sending...
        if( send(sock, sending, sendsize, 0) != sendsize)
                die("send size failed");

        //open socket to make it a file desriptor
        FILE* input = fdopen(sock, "r");
        if(input == NULL)
                die("sock input error");
        char ch[1000];

        //reading the http response part for 200 OK
        if(fgets(ch, sizeof(ch), input) != NULL) {
                if(strstr(ch, "200") == 0) {
                        printf("%s\n", ch);
                        fclose(input);
                        exit(1);
                } else{}
                        //printf("%s" , ch);    
        }

        //looking for /r in order to stop fgetting
        while(fgets(ch, sizeof(ch), input) != NULL) {
            if(ch[0] == '\r')
                        break;
                else{}
                        //printf("%s", ch);
        }

        //(self reminder)SHIFTED POSITION FOR OPENING FILE TO HERE
        file = fopen(fname, "wb");

        while( (n = fread(receiving, 1, sizeof(receiving), input )) ) {

                if (n < 0) {
                        die("fread failed");
                } else if( n == 0) {
                        exit(1);
                } else {
                        if (fwrite(receiving, 1, n, file) != n)
                                die("fwrite failed");
                }
        }

        //if(n<0)
        //      die("receive size failed");

        fclose(input);
        close(sock);
        fclose(file);

}