
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <lxc/lxccontainer.h>


void sig_handler(int signo)
{
//    int written = 0;
//
//    if (fh) {
//        written = fprintf(fh, "got a signal: %d\n", signo);
//        fflush(fh);
//    } else {
//        printf("Could not use file descriptor\n");
//    }
//
//    switch (signo) {
//        case 30:
//            exit(0);
//        default:
//            break;
//    }
//
//    printf("Wrote: %d\n", written);

    printf("Got a signal: %d\n", signo); 
}


int run_me_in_container(void *data)
{
    FILE *fh = NULL;
    fh = fopen("/tmp/program-in-container", "w");
    if (fh == NULL) {
        printf("Couldn't open the file\n");
        exit(1);
    }
    fprintf(fh, "From the attached program\n");
    fprintf(fh, "I am %d, my parent is %d\n", getpid(), getppid());
    fclose(fh);
}


int main(int argc, char *argv[]) {
    signal(SIGCHLD, sig_handler);


    struct lxc_container *c;
    int ret = 1;

    /* Setup container struct */
    c = lxc_container_new("jogr-container", NULL);
    if (!c) {
        fprintf(stderr, "Failed to setup lxc_container struct\n");
        goto out;
    }

    if (c->is_defined(c)) {
        fprintf(stderr, "Container already exists\n");
        goto out;
    }

    c->set_config_item(c, "lxc.logfile", "/tmp/lxc-log");

    char *loglevel = "debug";
    if (argc == 2) {
        loglevel = argv[1];
    }
    c->set_config_item(c, "lxc.loglevel", loglevel);

    c->want_daemonize(c, 1);

    /* Create the container */
    if (!c->create(c, "jogr", NULL, NULL, 0, NULL)) {
        fprintf(stderr, "Failed to create container rootfs\n");
        goto out;
    }

    char command[] = "/sbin/init";
    char* const args[] = {command, NULL};

    /* Start the container */
    if (!c->start(c, 0, args)) {
        fprintf(stderr, "Failed to start the container\n");
        goto out;
    }

    /* Query some information */
    printf("Container state: %s\n", c->state(c));
    printf("Container PID: %d\n", c->init_pid(c));

    lxc_attach_options_t options = LXC_ATTACH_OPTIONS_DEFAULT;
    int pid = 0;
//    if (c->attach(c, run_me_in_container, NULL, &options, &pid) < 0) {
//        printf("Could not attach to container\n");
//    } else {
//        printf("Attached to container, pid: %d\n", pid);
//    }

//    waitpid(pid, NULL, 0);
//    printf("After waiting for attached process\n");

    int timeout = 3;
    printf("Shutting down the container with timeout %d seconds...\n", timeout);

    /* Stop the container */
    if (!c->shutdown(c, timeout)) {
        printf("Failed to cleanly shutdown the container, forcing.\n");
        if (!c->stop(c)) {
            fprintf(stderr, "Failed to kill the container.\n");
            goto out;
        }
    } else {
	printf("Performed clean shutdown\n");
    }

    /* Destroy the container */
    if (!c->destroy(c)) {
        fprintf(stderr, "Failed to destroy the container.\n");
        goto out;
    } else {
	printf("Destroyed container\n");
    }

    ret = 0;
out:
    lxc_container_put(c);
    return ret;
}

