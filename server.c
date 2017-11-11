#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include<signal.h>

#include <sys/socket.h>
#include <sys/select.h>
#include <linux/if.h>
#include <linux/if_tun.h>

bool running = true;
void signal_handler(int sig_number){
    if (sig_number == SIGINT){
        fprintf(stderr, "Caught SIGINT, exiting.\n");
        running = false;
    }
}

int tun_alloc(char *dev, int *fd){
    struct ifreq ifr;
    int err;

    if( (*fd = open("/dev/net/tun", O_RDWR)) < 0 ){
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN;
    if (*dev){
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    }

    if ((err = ioctl(*fd, TUNSETIFF, (void *) &ifr)) < 0 ){
        close(*fd);
        return err;
    }
    strcpy(dev, ifr.ifr_name);
    return 0;
}

int main(int argc, char *argv[]){
    char* my_ip;
    char* remote_ip;
    if (argc >= 3){
        my_ip = argv[argc-2];
        remote_ip = argv[argc-1];
        fprintf(stderr, "Local IP: %s\nRemote IP: %s\n", my_ip, remote_ip);
    } else {
        fprintf(stderr, "You should include a local and remote IP pair for the tun.");
        exit(-1);
    }

    if (signal(SIGINT, signal_handler) == SIG_ERR){
        fprintf(stderr, "Can't attach handler for SIGINT");
        exit(-1);
    }

    char *device_name = calloc(32, sizeof(char));
    strcpy(device_name, "tun%d");
    int tun;
    int err;

    if ((err = tun_alloc(device_name, &tun)) != 0){
        fprintf(stderr, "Couldn't tun_alloc: %d\n", err);
        exit(err);
    }
    fprintf(stderr, "Was allocated device %s\n", device_name);

    char *setup_command = calloc(256, sizeof(char));
    sprintf(setup_command, "/sbin/ifconfig %s inet %s pointopoint %s mtu 150 up", device_name, my_ip, remote_ip);
    fprintf(stderr, "About to run `%s`\n", setup_command);
    if (system(setup_command) != 0){
        fprintf(stderr, "Interface config failed, exiting\n");
        close(tun);
        exit(-1);
    }

    fd_set fdset;
    struct timeval tv;
    while (running){
        FD_ZERO(&fdset);
        FD_SET(tun, &fdset);

        tv.tv_sec = 1;
        tv.tv_usec = 0;

        if(select(tun+1, &fdset, NULL, NULL, &tv) < 0){
            fprintf(stderr, "`select` failed... ");
            if(errno != EAGAIN && errno != EINTR){
                fprintf(stderr, "badly! Breaking.\n");
                break;
            } else {
                fprintf(stderr, "due to timeout. Continuing...\n");
                continue;
            }
        }

        if (FD_ISSET(tun, &fdset)){
            char buffer[1000];
            size_t len = read(tun, buffer, 1000);
            for (size_t i = 0; i < len; i++){
                printf("%02X ", buffer[i]);
            }
            printf("\n");
        }
    }
    fprintf(stderr, "Closing tunnel %s.\n", device_name);
    close(tun);
}
