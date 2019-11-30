/*
 * kenneth kiprotich
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
//#include <time.h>
//#include <signal.h>

//die operation
static void die(const char *s) {
        perror(s);
        exit(1);
}

//fork, track...
int main(int argc, char *argv[]) {

        char buf[1000];

        printf("port number: ");

        while(fgets(buf, sizeof(buf), stdin)) {

                //buffering...
                if(buf[strlen(buf)-1] == '\n')
                        buf[strlen(buf)-1] = 0;

                //initializing pid for fork()ing and formerpid for tracking dead kids
                pid_t pid, formerpid;

                //printing the terminated children, if there are any    
                while( (formerpid =  waitpid( (pid_t) -1, NULL, WNOHANG) ) > 0)
                        printf("[pid=%d] mdb-lookup-server terminated \n", formerpid);


                if(strlen(buf) > 1) {

                        pid = fork();

                        //doing the switch-case for 0 and -1
                        switch(pid) {

                        case -1:
                                die("fork failed\n");


                        case 0:
                                fprintf(stderr, "[pid=%d] ", (int) getpid());

                                fprintf(stderr, "mdb-lookup-server2 started on port %s\n", buf );

                                execl("./mdb-lookup-server-nc.sh", "mdb-lookup-server-nc.sh", buf, (char *)0);

                                die("execl failure");
                        }

                        printf("port number: ");

                } else {
                        //fflush(buf);
                        printf("port number: ");
                }
        } //end of while loop 
        return 0;
} //end of main function 