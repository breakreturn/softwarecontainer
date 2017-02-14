
#include <stdio.h>
#include <stdlib.h> // exit()
#include <unistd.h>
#include <signal.h>


FILE *fh = NULL;


void init_mylog()
{
    fh = fopen("/tmp/init-log", "w");
    if (fh == NULL) {
        fprintf(stderr, "Error writing to file\n");
    }
}


void close_mylog()
{
    fclose(fh);
}


void mylog(const char *message)
{
    if (fh) {
        fprintf(fh, message);
        fflush(fh);
    } else {
        fprintf(stderr, "Could not log to file\n");
    }
}


void sig_handler(int signo)
{
    int written = 0;

    if (fh) {
        written = fprintf(fh, "got a signal: %d\n", signo);
        fflush(fh);
    } else {
        printf("Could not use file descriptor\n");
    }

    switch (signo) {
        case 30:
            exit(0);
        default:
            break;
    }

    printf("Wrote: %d\n", written);

    printf("Got a signal\n"); 
}


int main()
{
    printf("Hej\n");

    init_mylog();

    signal(SIGHUP, sig_handler);
    signal(SIGINT, sig_handler);
    signal(SIGQUIT, sig_handler);
// SIGILL
// SIGTRAP
// SIGABRT
// SIGBUS
// SIGFPE
// SIGKILL
    signal(SIGUSR1, sig_handler);
// SIGSEGV
    signal(SIGUSR2, sig_handler);
// SIGPIPE
// SIGALRM
    signal(SIGTERM, sig_handler);
    signal(SIGCHLD, sig_handler);
    signal(SIGSTOP, sig_handler);
    signal(SIGPWR, sig_handler);

    const char *message = "This is from the init process\n";
    mylog(message);

    while (1)
	sleep(1);

    close_mylog();

    return 0;
}

