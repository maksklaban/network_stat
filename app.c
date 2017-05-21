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
#include <ifaddrs.h>

#include "container.c"

// TODO: Implement config_file

#define SOCK_PATH "echo_socket"
#define MAX_BUFFER_SIZE 100
#define DAEMON_HOMEDIR "/tmp"
#define DATABASES_HOMEDIR "/tmp/db/"

char interface[20] = "wlp3s0";

// Check current iface if exist

int check_iface(char* iface, char* iface_list[], int size) {
    for ( int i = 0; i <= size; i++ ) {
        if (strcmp(iface_list[i], iface) == 0) {
            return 1;
        }
    }

    return 0;
}

// Print all available network ifaces

void print_all_ifaces(char* iface_list[], int size) {
    printf("Available ifaces:\n");

    for ( int i = 0; i < size; i++ ) {
        printf("    %s\n", iface_list[i]);
    }
}

// Save all available network ifaces

int save_all_ifaces(char* iface_list[]) {
    struct ifaddrs *addrs,*tmp;
    int i;

    getifaddrs(&addrs);
    tmp = addrs;

    
    for (i = 0; tmp != 0; ) {
        if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_PACKET) {
            iface_list[i] = malloc(sizeof(tmp->ifa_name) + 1);
            strcpy(iface_list[i], tmp->ifa_name);
            i++;
        }

        tmp = tmp->ifa_next;
    }
    freeifaddrs(addrs);
    return i;
}

// Print all ip and count for given interface 

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

// Print all ip and count for all interfaces 

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

// Print packets count for given ip and current iface

void print_ip_info(char* ip) {
    struct node* buffer;
    struct node* needle;
    char file_name[30];
    FILE* f;

    strcpy(file_name, DATABASES_HOMEDIR);
    strcat(file_name, interface);

    if ((f = fopen(file_name, "rb")) == NULL) {
        perror("Missing stat from iface");
        return;
    }

    restoreTree(&buffer, f);

    if ((needle = searchNode(ip, buffer)) == NULL) {
        printf("IP %s not found\n", ip);

        free(buffer);
        free(needle);
        return;
    }

    printf("IP-address - %s, Count - %d\n", needle->ip, needle->counter);
    free(needle);
}

// Send to daemon users commands

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

// Print useful info

void print_help(char* app_name) {
    printf("Usage: %s [OPTIONS]\n\n", app_name);
    printf(" OPTIONS:\n");
    printf(" -h --HELP                'print this help'\n");
    printf(" -s --start               'packets are being sniffed from now on from default iface(eth0)'\n");
    printf(" -S --stop                'packets are not sniffed'\n");
    printf(" -c --count   ipaddress   'print number of packets received from ip address'\n");
    printf(" -i --select  interface   'select interface for sniffing(eth0, wlan0, ethN, wlanN...)'\n");
    printf(" -l --list                'print all available network ifaces'");
    printf(" -I --stat    interface   'show all collected statistics for particular interface,\n");
    printf("                           if iface omitted - for all interfaces'\n\n");
}

int main(int argc, char** argv) {
    struct request* reqv;
    char str[MAX_BUFFER_SIZE];
    char* app_name = argv[0];
    char* iface_list[20];
    int list_count;

    int value;

    if ( argc < 2) {
        printf("Usage: %s [OPTIONS]\n", app_name);
        printf("Enter the:  %s -h  for details\n", app_name);
    }
    list_count = save_all_ifaces(iface_list);

    while(1) {
        static struct option long_options[] = {
            {"start", no_argument, 0, 's'},
            {"stop", no_argument, 0, 'S'},
            {"help", no_argument, 0, 'h'},
            {"list", no_argument, 0, 'l'},
            {"count", required_argument, 0, 'c'},
            {"select", required_argument, 0, 'i'},
            {"stat", optional_argument, 0, 'I'},
            {0, 0, 0, 0}
        };
        int option_index = 0;

        reqv = (struct request*)&str;
        
        value = getopt_long(argc, argv, ":sShlc:i:I:", long_options, &option_index);

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
                if (check_iface(optarg, iface_list, list_count) == 0) {
                    fprintf(stderr, "Iface %s is does not exist\n", optarg);
                    fprintf(stderr, "Type %s -l to show a list of available ifaces\n", app_name);
                    return EXIT_FAILURE;
                }
                strcpy(reqv->argument, optarg);
                strcpy(interface, optarg);
                send_message(str);
                break;
            case('I'):
                print_data(optarg);
                break;
            case('c'):
                print_ip_info(optarg);
                break;
            case('h'):
                print_help(app_name);
                break;
            case('l'):
                print_all_ifaces(iface_list, list_count);
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
            case('?'):
                fprintf(stderr, "option %c invalid\n", value);
                fprintf(stderr, "Type -h to help\n" );
                return EXIT_FAILURE;
            default:
                break;
        }
        
    }

    return 0;
}