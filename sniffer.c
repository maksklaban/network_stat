#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "container.c"

#define MAX_BUFFER 65536


// FILE *logfile;
int total=0; 
 
int main(int argc, char** argv) {
    int saddr_size , data_size;
    struct sockaddr_in saddr;
    char* ip;
    unsigned char buffer[MAX_BUFFER];
    struct node* tree;
    int i = 3000;
    FILE* f;
     
    f = fopen("wlp3s0", "wb");
    memset(&saddr, 0, sizeof(saddr));
    // logfile=fopen("log.txt","w");
    // if(logfile==NULL) {
    //     printf("Unable to create log.txt file.");
    // }
    printf("Starting...\n");
     
    int sock_raw = socket( AF_PACKET , SOCK_RAW , htons(ETH_P_ALL)) ;
    if ( setsockopt(sock_raw , SOL_SOCKET , SO_BINDTODEVICE , "wlp3s0" , strlen("wlp3s0") + 1) < 0 ) {
        perror("Binding error");
        return 1;
    }
     
    if ( sock_raw < 0 ) {
        //Print the error with proper message
        perror("Socket Error");
        return 1;
    }

    while( i > 0 ) {
        saddr_size = sizeof(saddr);
        //Receive a packet
        data_size = recvfrom(sock_raw , buffer , MAX_BUFFER , 0 , (struct sockaddr *)&saddr , (socklen_t*)&saddr_size);
        if ( data_size < 0 ) {
            printf("Recvfrom error , failed to get packets\n");
            return 1;
        }
        struct iphdr *iph = (struct iphdr *)(buffer  + sizeof(struct ethhdr) );
        saddr.sin_addr.s_addr = iph->saddr;
        ip = inet_ntoa(saddr.sin_addr);

        insertNode(data_size, ip, &tree);
        i--;
        // printf("Number - %d, IP - %s\n", data_size, ip);
    }

    printf("BEFORE\n");
    printTree(tree);

    saveTree(tree, f);
    // destroyTree(tree);
    fclose(f);
    f = fopen("wlp3s0", "rb");
    recoverTree(tree, f);
    printf("AFTER\n");
    printTree(tree);
    fclose(f);
    // TODO/ correct close app
    close(sock_raw);
    destroyTree(tree);
    printf("Finished\n");
    return 0;
}

