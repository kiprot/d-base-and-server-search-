/*
 *kenneth kiprotich
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


static void die(const char *s) { perror(s); exit(1); }


//define some errors and a buch of other stuff i might need
char *SUCCESS_200 =     "HTTP/1.0 200 OK\r\n\r\n";
char *ERROR_301 =       "HTTP/1.0 301 Moved Permanently\r\n\r\n<html><body><h1>HTTP/1.0 301 Moved Permanently</h1></body></html>\r\n\r\n";
char *ERROR_400 =       "HTTP/1.0 400 Bad Request\r\n\r\n<html><body><h1>HTTP/1.0 400 Bad Request</h1></body></html>\r\n\r\n";
char *ERROR_404 =       "HTTP/1.0 404 Not found\r\n\r\n<html><body><h1>HTTP/1.0 404 Not found</h1></body></html>\r\n\r\n";
char *ERROR_501 =       "HTTP/1.0 501 Not Implemented\r\n\r\n<html><body><h1>HTTP/1.0 501 Not Implemented</h1></body></html>\r\n\r\n";
char *HTML_ERROR_MSG =  "<html><body><h1> </h1></body></html>";
char *V10 =             "HTTP/1.0";
char *V11 =             "HTTP/1.1";

char *startTable =      "<p><table border>\n";
char *startRow =        "<tr><td>\n";
char *endRow =          "</td></tr>\n";
char *endTable =        "</table>\n";

//hardcoded as provided by Jae in the lab description
const char* form =      "<h1>mdb-lookup</h1>\n"
                        "<p>\n"
                        "<form method=GET action=/mdb-lookup>\n"
                        "lookup: <input type=text name=key>\n"
                        "<input type=submit>\n"
                        "</form>\n"
                        "<p>\n";

const char *endHTML =   "</body></html>";
//speical urls to recognize
#define LOOKUP          "/mdb-lookup"
#define KEYED_LOOKUP    "/mdb-lookup?key="
char dbfetch[4096]; //for storing results from mdb_sock

// .http-server 8000 /home/jae/html 127.0.0.1 9000


//func 1(transferring from my former external c file)
static int dbLookupMaker( char *port, char *dbhostname) {

        int mdbsock;
        struct hostent *dbhost;
        unsigned short mdbport = atoi(port);

        if((dbhost = gethostbyname(dbhostname)) == NULL)
        die("gethostbyname(dbhostname) failure");

        if( (mdbsock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
                die("mdbsock creation failure");

        //boilerplate stuff
        struct sockaddr_in mdbservaddr;
        memset(&mdbservaddr, 0, sizeof(mdbservaddr));
        mdbservaddr.sin_family = AF_INET;
        mdbservaddr.sin_addr.s_addr = htonl(INADDR_ANY); // any network interface
        mdbservaddr.sin_port = htons(mdbport); //httpport

        if( connect( mdbsock, (struct sockaddr *) &mdbservaddr, sizeof(mdbservaddr) ) < 0)
                die("mbd connect() failure");

        return mdbsock;

} //end of mbbsock function

//from my former external c file
int httpServerMaker (char *port ) {

        int servsock;
        if((servsock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP) ) < 0)
                die("servsock socket() failure");

        unsigned short httpport = atoi(port);

        // Construct local address structure
        struct sockaddr_in httpservaddr;
        memset(&httpservaddr, 0, sizeof(httpservaddr));
        httpservaddr.sin_family = AF_INET;
        httpservaddr.sin_addr.s_addr = htonl(INADDR_ANY); // any network interface
        httpservaddr.sin_port = htons(httpport); //httpport


        // Bind to the local address (boierplate)
        if (bind(servsock, (struct sockaddr *) &httpservaddr, sizeof(httpservaddr)) < 0)
                die("bind failed");

        if (listen(servsock, 5 /*MAX NUN5 */ ) < 0 )
                die("servsock listen() failure");

        return servsock;
}


int main(int argc, char **argv) {

        if(argc != 5) {
                printf("%s\n", "usage: ./http-server <server_port> <web_root> <mdb-lookup-host> <mdb-lookup-port>");
                exit(1);
        }



        int serv_sock = httpServerMaker (argv[1]);
                int mdb_sock = dbLookupMaker(argv[4], argv[3]);

        FILE *mdbfile = fdopen(mdb_sock, "r");
        if(mdbfile == NULL)
                die("fdopen mdbfile failed");

        char buff1[2000];
        char requestLine[4096];
        struct sockaddr_in clntaddr;

        while(1) {

                unsigned int clntlen;
                clntlen = sizeof(clntaddr);
                int clntsock;
                if ((clntsock = accept(serv_sock, (struct sockaddr *)&clntaddr, &clntlen)) < 0)
                        die("accept() failure");


                //socket file creation
                FILE *clntfile = fdopen(clntsock, "r");

                if(clntfile == NULL)
                        die("fdopen() failure");

                //memset(rcv,0, sizeof(rcv));
                //fgets here

                if(fgets(requestLine, sizeof(requestLine), clntfile) != NULL) { }
                
                else {
                        printf( "400 Bad Request (in fgets)\n");
                        fclose(clntfile);
                        if(send(clntsock, ERROR_301, strlen(ERROR_301), 0 ) != strlen(ERROR_301) )
                               die("send() of ERROR_301 to client failed");

                        //printf( "%s \" %s %s %s\" 301 Moved Permanently \n", IPaddress, method, requestURI, httpVersion );
                        fclose(clntfile); //RECHECK LATER
                }

                //ip address forclarity in printing
                char *IPaddress = inet_ntoa(clntaddr.sin_addr);

                char *token_separators = "\t \r\n"; // tab, space, new line
                char *method = strtok(requestLine, token_separators);
                char *requestURI = strtok(NULL, token_separators);
                char *httpVersion = strtok(NULL, token_separators);

                //check http version
                if(httpVersion == NULL) {
                        //my 501 bolierplate
                        if(send(clntsock, ERROR_301, strlen(ERROR_301), 0 ) != strlen(ERROR_301) )
                               die("send() of ERROR_301 to client failed");

                        printf( "%s \" %s %s %s\" 301 Moved Permanently \n", IPaddress, method, requestURI, httpVersion );
                        fclose(clntfile); //RECHECK LATER

                }


                if( strcmp(httpVersion, V10) != 0 && strcmp(httpVersion, V11) != 0) {

                        //my 501 bolierplate
                        if(send(clntsock, ERROR_501, strlen(ERROR_501), 0 ) != strlen(ERROR_501) )
                               die("send() of ERROR_501 to client failed");

                        printf( "%s \" %s %s %s\" 501 Not Implemented \n", IPaddress, method, requestURI, httpVersion );
                        fclose(clntfile); //RECHECK LATER
                
                }

                //check to ensure all three exist
                if((requestLine == 0) || (method == 0) || (httpVersion == 0)  ) {

                        //my 501 bolierplate
                        if(send(clntsock, ERROR_501, strlen(ERROR_501), 0 ) != strlen(ERROR_501) )
                               die("send() of ERROR_501 to client failed");

                        printf( "%s \" %s %s %s\" 501 Not Implemented \n", IPaddress, method, requestURI, httpVersion );
                        fclose(clntfile); //RECHECK LATER

                }

                //check for get method in header
                if(strcmp(method, "GET") != 0) {

                        //my 501 bolierplate
                        if(send(clntsock, ERROR_501, strlen(ERROR_501), 0 ) != strlen(ERROR_501) )
                                die("send() of ERROR_501 to client failed");

                        printf("%s \" %s %s %s\" 501 Not Implemented \n", IPaddress, method, requestURI, httpVersion );
                        fclose(clntfile); //RECHECK LATER

                }

                if( (strcmp(requestURI, strchr(requestURI, '/')) != 0) && (strchr(requestURI, '/')) ) {

                        if(send(clntsock, ERROR_400, strlen(ERROR_400), 0 ) != strlen(ERROR_400) )
                                die("send() of ERROR_400 to client failed");

                        printf("%s \" %s %s %s\" 400 Bad Request \n", IPaddress, method, requestURI, httpVersion );


                }

                if((strstr(requestURI, "/..")) != NULL ) {

                        if(send(clntsock, ERROR_400, strlen(ERROR_400), 0 ) != strlen(ERROR_400) )
                                die("send() of ERROR_400 to client failed");

                        printf("%s \" %s %s %s\" 400 Bad Request \n", IPaddress, method, requestURI, httpVersion );
                        fclose(clntfile);

                }

                while(1) {
                        //reading hearders and content
                        if(fgets(buff1, sizeof(buff1), clntfile) == NULL) {
                                //my err400 bolierplate
                                fprintf(stderr, "%s \" %s %s %s\" 400 Bad Request \n", IPaddress, method, requestURI, httpVersion );
                                fclose(clntfile);
                        }

                        //serch fro end charactera
                        if(strcmp(buff1, "\n") == 0)
                                break;

                        if(strcmp(buff1, "\r\n") == 0)
                                break;

                }


                char sender[4096]; //for use in the sending of the hrml and image files


                //if(strncmp(requestURI, KEYED_LOOKUP, strlen(KEYED_LOOKUP)) == 0 ) {

                if (strcmp(requestURI, LOOKUP) == 0 ) {

                        char *IP = inet_ntoa(clntaddr.sin_addr);
                        int a, z;
                        a = send(clntsock, SUCCESS_200, strlen(SUCCESS_200), 0);
                        if(a != strlen(SUCCESS_200))
                                die("seding success_200 failed");

                        //sending the hardcoded from to the user screen
                        z = send(clntsock, form, strlen(form), 0);
                        if(z != strlen(form) )
                                die("dending form failure");
                        //to my terminal screen
                        printf( "%s \"  %s %s %s \" 200 OK look\n", IP, method, requestURI, httpVersion );


                }// END OF INITIAL IF 

                //if the lookup on uri has additional keys for searching

                //else if(strncmp(requestURI, KEYED_LOOKUP, strlen(KEYED_LOOKUP)) == 0 ) {
                else if(strstr(requestURI, KEYED_LOOKUP) != NULL && strcmp(requestURI, strstr(requestURI, KEYED_LOOKUP)) == 0 ) {

                        char *IP = inet_ntoa(clntaddr.sin_addr);
                        char *withsign = strstr(requestURI, "=");

                        char *search = withsign+1;

                        //appending the end line character on the search string
                         strcat(search, "\n");

                        int c, b, d, e;
                        //sending the search string (moved this here from its initial postion down there)

                        //another ok200 to socket
                        d = send(clntsock, SUCCESS_200, strlen(SUCCESS_200), 0);
                        if(d != strlen(SUCCESS_200))
                                die("seding success_200 failed");

                        char toprint[strlen(search)];
                        strcpy(toprint, search);
                        toprint[strlen(toprint)-1] = 0;

                        printf("%s Looking for: %s  \"  %s %s %s \" 200 OK\n", IP, toprint, method, toprint,  httpVersion );


                        b = send(clntsock, form, strlen(form), 0);
                        if(b != strlen(form) )
                               die("dending form failure");

                        //send the inital formation of the html table for rendering the lookup results
                        e = send(clntsock, startTable, strlen(startTable), 0);
                        if (e != strlen(startTable) )
                                die("sending startTable to clientsock failed");

                        //sending the search term to the lookup server
                        c = send(mdb_sock, search, strlen(search), 0);
                        if(c != strlen(search))
                              die("seding string to search failed");

                        while(fgets(dbfetch, sizeof(dbfetch), mdbfile) != NULL ) {
                                //printf("%s\n", dbfetch);

                                if(strcmp(dbfetch, "\n") == 0 )
                                        break;

                                if(strcmp(dbfetch, "\r\n") == 0 )
                                        break;

                                char msg[strlen(startRow) + strlen(dbfetch)];
                                strcpy(msg, startRow);
                                strcat(msg, dbfetch);

                                if (send(clntsock, msg, strlen(msg),0) != strlen(msg) )
                                        die("seding msg to clintsock failed");

                        }

                        if (send(clntsock, endTable, strlen(endTable),0) != strlen(endTable) )
                                die("seding endTable to clintsock failed");
                }



                //page rendering option
                else { 

                        char *IP = inet_ntoa(clntaddr.sin_addr);

                        FILE *file = NULL;
                        //char buff[2000];

                        //rememered to free()
                        char *filename =  malloc(strlen(argv[2]) + strlen(requestURI) + 50);

                        if(filename == NULL)
                                die("filename malloc failure");

                        strcpy(filename, argv[2]);
                        strcat(filename, requestURI);
                        printf("%s\n", filename);
                        int len = strlen(filename)-1;
                        char *append = "index.html";
                        if(filename[len] == '/')
                                strcat(filename, append);

                        file = fopen(filename, "r");

                        //other null check approach
                        if(file != NULL) {
                                int a;
                                a = send(clntsock, SUCCESS_200, strlen(SUCCESS_200), 0);
                                if(a != strlen(SUCCESS_200) )
                                        die("sending 200 failed");

                                printf( "%s \" %s %s %s \" 200 OK \n", IP, method, requestURI, httpVersion );


                                size_t n;
                                while( (n = fread(sender, 1, sizeof(sender), file) ) > 0) {

                                        if(send(clntsock, sender, n, 0) != n)
                                                die("send()ing failed");
                                        //printf("%s\n", buff);
                                        //printf("%d\n", sizeof(buff));
                                        memset(sender, 0, sizeof(sender));
                                }
                                memset(sender, 0, sizeof(sender));
                                fclose(file);
                        }
                        else { //file not present
                                int n;
                                n = send(clntsock, ERROR_404, strlen(ERROR_404), 0);

                                if (n != strlen(ERROR_404))
                                        die("seding error_404 failed");

                                printf( "%s \" %s %s %s \" 404 Not Found 1\n", IP, method, requestURI, httpVersion );
                        }

                        fclose(mdbfile);
                        fclose(clntfile);
                        close(clntsock);


                } //1st while(1)
        return 0;

}