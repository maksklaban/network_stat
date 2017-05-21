#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <time.h>
#include <getopt.h>

#include "container.c"

// TODO: Implement config_file

#define MAX_BUFFER 65536
#define SOCK_PATH "echo_socket"
#define LOGS_FILENAME "logs.txt"
#define LOCK_FILENAME "daemon.lock"
#define DAEMON_HOMEDIR "/tmp"
#define DATABASES_HOMEDIR "/tmp/db/"

void LogMessage(char* message, char* error);
void daemonize();
void signal_handler(int sig);
void socket_server();
void sniff_packets(char* interface);
void backup_data(char* file_name, struct node* tree);

static char interface[15] = "wlp3s0";

void LogMessage(char* message, char* error) {
    struct tm* timeinfo;
    FILE* logs;
    time_t rawtime;
    
    logs = fopen(LOGS_FILENAME, "a");

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    fprintf(logs, "%s %s !%s", message, error, asctime(timeinfo));

    fclose(logs);
}

void signal_handler(int sig) {
    switch (sig) {
        case SIGHUP:
            LogMessage("[sig_nand] hangup signal catched", NULL);
            break;
        case SIGTERM:
            LogMessage("[sig_nand] terminate signal catched", NULL);
            exit(EXIT_SUCCESS);
            break;
    }
}

void daemonize() {
    pid_t pid;
    int lock_file, fd;
    char str_pid[10];

    pid = fork();

    if (pid < 0) {
        perror("[daemon]FAILED fork");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    if (setsid() < 0) {
        perror("[daemon]FAILED create a new SID for the child process");
        exit(EXIT_FAILURE);
    }

    if ((chdir(DAEMON_HOMEDIR)) < 0) {  
        perror("[daemon]FAILED change the current directory");
        exit(EXIT_FAILURE);
    }

    for (fd = sysconf(_SC_OPEN_MAX); fd >= 0; fd--) {
        close(fd);
    }

    stdin = fopen("/dev/null", "r");
    stdout = fopen("/dev/null", "w+");
    stderr = fopen("/dev/null", "w+");

    umask(0);

    lock_file = open(LOCK_FILENAME, O_RDWR|O_CREAT, 0640);
    
    if (lock_file < 0 ) {
        LogMessage("[daemon]FAILED open lock file", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (lockf(lock_file, F_TEST, 0) < 0) {
        exit(EXIT_SUCCESS);
    }

    lockf(lock_file,F_TLOCK,0);

    sprintf(str_pid, "%d\n", getpid());
    write(lock_file, str_pid, strlen(str_pid));

    signal(SIGCHLD,SIG_IGN);
    signal(SIGTSTP,SIG_IGN);
    signal(SIGTTIN,SIG_IGN);
    signal(SIGHUP,signal_handler);
    signal(SIGTERM,signal_handler);

}

void socket_server() {
    int s, s2, len, pid;
    int pid_ch = 0;
    struct sockaddr_un local, remote;

    char str[100];
    struct request* reqv;
    int need_child = 1;

    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        LogMessage("[Unix socket] Socket create Error ", strerror(errno));
        exit(1);
    }

    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, SOCK_PATH);
    unlink(local.sun_path);
    
    len = strlen(local.sun_path) + sizeof(local.sun_family);
    
    if (bind(s, (struct sockaddr *)&local, len) == -1) {
        LogMessage("[Unix socket] Socket bind error ", strerror(errno));
        exit(1);
    }

    if (listen(s, 5) == -1) {
        LogMessage("[Unix socket] Socket listen error", strerror(errno));
        exit(1);
    }

    while(1) {
        int n;
        int t = sizeof(remote);

        if ((s2 = accept(s, (struct sockaddr *)&remote, &t)) < -1) {
            LogMessage("[Unix socket] accept error", strerror(errno));
            continue;
        }

        if ((n = recv(s2, str, 100, 0)) < 0) {
            LogMessage("[Unix socket] recv error", strerror(errno));
            continue;
        }

        reqv = (struct request*)&str;

        switch(reqv->option) {
            case('s'):
                if ( need_child > 0) {
                    if ((pid = fork()) < 0) {
                        LogMessage("[Unix socket] fork error", strerror(errno));
                        break;
                    } else if (pid == 0) {
                        sniff_packets(interface);
                    } else {
                        pid_ch = pid;
                        need_child = 0;
                    }
                }

                LogMessage("Start sniffing packets", interface);
                break;
            case('S'):

                LogMessage("Stop sniffing packets", interface);

                if (pid_ch > 0) {
                    kill(pid_ch, SIGTERM);
                    need_child = 1;
                }

                break;
            case('i'):
                strcpy(interface, reqv->argument);
                LogMessage("Change default interface", interface);
                break;
            default:
                break;
        }

        close(s2);
    }
    return;
}

void sniff_packets(char* interface) {
    int saddr_size , data_size;
    int sock_raw;
    struct sockaddr_in saddr;
    struct node* tree;
    struct iphdr* iph;
    unsigned char buffer[MAX_BUFFER];
    char file_name[50];
    FILE* f;
    
    char* ip;
    
    memset(&saddr, 0, sizeof(saddr));

    strcpy(file_name, DATABASES_HOMEDIR);
    strcat(file_name, interface);
     
    f = fopen(file_name, "rb");

    restoreTree(&tree, f);

    sock_raw = socket( AF_PACKET , SOCK_RAW , htons(ETH_P_ALL)) ;

    if (sock_raw < 0) {
        LogMessage("[Sniff packets] socket error", strerror(errno));
        exit(1);
    }
    if (setsockopt(sock_raw , SOL_SOCKET , SO_BINDTODEVICE , interface , strlen(interface) + 1) < 0) {
        LogMessage("[Sniff packets] setsocktopt error", strerror(errno));
        exit(1);
    }
    while (1) {
        saddr_size = sizeof(saddr);
        data_size = recvfrom(sock_raw , buffer , MAX_BUFFER , 0 , (struct sockaddr *)&saddr , (socklen_t*)&saddr_size);

        if (data_size < 0) {
            LogMessage("[Sniff packets] Recvfrom error , failed to get packets", strerror(errno));
            return;
        }

        iph = (struct iphdr *)(buffer  + sizeof(struct ethhdr) );
        saddr.sin_addr.s_addr = iph->saddr;
        ip = inet_ntoa(saddr.sin_addr);
        insertNode(data_size, ip, &tree);
        backup_data(file_name, tree);
    }
}

void backup_data(char* file_name, struct node* tree) {
    FILE* f;

    f = fopen(file_name, "wb");
    saveTree(tree, f);
    fclose(f);
}

int main() {

    daemonize();

    socket_server();

    return 0;
}