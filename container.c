#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_IP_SIZE 15

struct request {
    char option;
    char argument[50];
};

struct node {
    int counter;
    char ip[MAX_IP_SIZE];
    struct node* left;
    struct node* right;
};

void insertNode(int counter, char* ip, struct node** leaf) {
    if ( *leaf == 0 ) {
        *leaf = (struct node*)malloc(sizeof(struct node));
        (*leaf)->counter = counter;
        strcpy((*leaf)->ip, ip);
        (*leaf)->left = 0;
        (*leaf)->right = 0;
        return;
    } 

    if ( strcmp(ip, (*leaf)->ip) < 0 ) {
        insertNode(counter, ip, &(*leaf)->left);
    } else if ( strcmp(ip, (*leaf)->ip ) > 0) {
        insertNode(counter, ip, &(*leaf)->right);
    } else {
        (*leaf)->counter += counter;
    }
}

void destroyTree(struct node* leaf) {
    if ( leaf != 0 ) {
        destroyTree(leaf->left);
        destroyTree(leaf->right);
        free(leaf);
    }
}

struct node* searchNode(char* ip, struct node* leaf) {
    if ( leaf == 0 ) {
        return 0;
    }

    if ( strcmp(ip, leaf->ip ) == 0 ) {
        return leaf;
    } else if ( strcmp(ip, leaf->ip ) < 0 ) {
        searchNode(ip, leaf->left);
    } else {
        searchNode(ip, leaf->left);
    }
}

void printTree(struct node* tree) {
    if ( tree != 0 ) {
        printf("IP-address - %s, Count - %d\n", tree->ip, tree->counter);
        printTree(tree->left);
        printTree(tree->right);
    }
}

void saveTree(struct node* tree, FILE* file) {
    if (tree != 0) {
        fwrite(tree, sizeof(struct node), sizeof(char), file);
        saveTree(tree->left, file);
        saveTree(tree->right, file);
    }
}

void recoverTree(struct node* tree, FILE* file) {
    tree = (struct node*)malloc(sizeof(struct node));

    while (fread(tree, sizeof(struct node), sizeof(char), file) != 0) {
        printf("IP-address - %s, Count - %d\n", tree->ip, tree->counter);
    }
}

