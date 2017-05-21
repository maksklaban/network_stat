#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <getopt.h>
#include <unistd.h>
#include <dirent.h>

#include "container.c"

// TODO: Implement config_file

#define SOCK_PATH "echo_socket"
#define MAX_BUFFER_SIZE 100
#define DAEMON_HOMEDIR "/tmp"
#define DATABASES_HOMEDIR "/tmp/db/"

char interface[20] = "wlp3s0";

void print_data(char* interface) {
    char file_name[30];
    FILE* f;

    strcpy(file_name, DATABASES_HOMEDIR);
    strcat(file_name, interface);

    if ((f = fopen(file_name, "rb")) == NULL) {
        perror("Missing stat from iface");
        return;
    }
    printTree(f, interface);
    fclose(f);
}

void print_all_data() {
    DIR *dp;
    struct dirent *ep;
    
    dp = opendir(DATABASES_HOMEDIR);

    if (dp != NULL) {
        while (ep = readdir(dp)){
            if (strcmp(ep->d_name, "..") != 0 && strcmp(ep->d_name, ".") != 0) {
                print_data(ep->d_name);
            }
        }
        closedir (dp);
    }
    else {
        perror ("Couldn't open the directory");
    }
}

void print_ip_info(char* ip) {
    struct node* buffer;
    char file_name[30];
    FILE* f;

    strcpy(file_name, DATABASES_HOMEDIR);
    strcpy(file_name, interface);
}

void send_message(char* buffer) {
    int s, t, len;
    struct sockaddr_un remote;
    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    if ((chdir(DAEMON_HOMEDIR)) < 0) {  
        perror("[daemon]FAILED change the current directory");
        exit(EXIT_FAILURE);
    }

    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, SOCK_PATH);
    
    len = strlen(remote.sun_path) + sizeof(remote.sun_family);
    
    if (connect(s, (struct sockaddr *)&remote, len) == -1) {
        perror("connect");
        exit(1);
    }
    if (send(s, buffer, strlen(buffer), 0) == -1) {
        perror("send");
        exit(1);
    }
    close(s);
}

void print_help(char* app_name) {
    printf("Usage: %s [OPTIONS]\n\n", app_name);
    printf(" OPTIONS:\n");
    printf(" -h --HELP                'print this help'\n");
    printf(" -s --start               'packets are being sniffed from now on from default iface(eth0)'\n");
    printf(" -S --stop                'packets are not sniffed'\n");
    printf(" -c --count   ipaddress   'print number of packets received from ip address'\n");
    printf(" -i --select  interface   'select interface for sniffing(eth0, wlan0, ethN, wlanN...)'\n");
    printf(" -I --stat    interface   'show all collected statistics for particular interface,\n");
    printf("                           if iface omitted - for all interfaces'\n\n");
}

int main(int argc, char** argv) {
    struct request* reqv;
    char str[MAX_BUFFER_SIZE];
    char* app_name = argv[0];

    int value;

    if ( argc < 2) {
        printf("Usage: %s [OPTIONS]\n", app_name);
        printf("Enter the:  %s -h  for details\n", app_name);
    }

    while(1) {
        static struct option long_options[] = {
            {"start", no_argument, 0, 's'},
            {"stop", no_argument, 0, 'S'},
            {"help", no_argument, 0, 'h'},
            {"count", required_argument, 0, 'c'},
            {"select", required_argument, 0, 'i'},
            {"stat", optional_argument, 0, 'I'},
            {0, 0, 0, 0}
        };
        int option_index = 0;

        reqv = (struct request*)&str;
        
        value = getopt_long(argc, argv, ":sShc:i:I:", long_options, &option_index);

        if ( value == -1) {
            break;
        }

        switch (value) {
            case(0):
                if (long_options[option_index].flag != 0) {
                    break;
                }
                reqv->option = value;   
                if (optarg) {
                    strcpy(reqv->argument, optarg);
                } else {
                    reqv->argument[0] = 0;
                }
                send_message(str);
                break;
            case('s'):
                reqv->option = value;
                reqv->argument[0] = 0;
                send_message(str);
                break;
            case('S'):                
                reqv->option = value;
                reqv->argument[0] = 0;
                send_message(str);
                break;
            case('i'):
                reqv->option = value;
                strcpy(reqv->argument, optarg);
                strcpy(interface, optarg);
                send_message(str);
                break;
            case('I'):
                print_data(optarg);
                break;
            case('c'):
                reqv->option = value;
                strcpy(reqv->argument, optarg);
                break;
            case('h'):
                print_help(app_name);
                break;
            case(':'):
                switch(optopt) {
                    case('I'):
                        print_all_data();
                        break;
                    default:
                        fprintf(stderr, "option -%c is missing a required argument\n", optopt);
                        return EXIT_FAILURE;
                }
                break;
            default:
                break;
        }
        
    }

    return 0;
}