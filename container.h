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

void insertNode(int counter, char* ip, struct node** leaf);
void destroyTree(struct node* leaf);
struct node* searchNode(char* ip, struct node* leaf);
void restoreTree(struct node** tree, FILE* file);
void printLeafs(struct node* tree);
void printLeafs(struct node* tree);
void saveTree(struct node* tree, FILE* file);
void printTree(FILE* file, char* interface);
