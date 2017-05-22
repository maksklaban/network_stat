#include "container.h"


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
    return 0;
}

void restoreTree(struct node** tree, FILE* file) {
    struct node* buffer = (struct node*)malloc(sizeof(struct node));

    while (fread(buffer, sizeof(struct node), sizeof(char), file) != 0) {
        insertNode(buffer->counter, buffer->ip, tree);
    }
    free(buffer);
}

void printLeafs(struct node* tree) {
    if ( tree != 0 ) {
        printf("IP-address - %s, Count - %d\n", tree->ip, tree->counter);
        printLeafs(tree->left);
        printLeafs(tree->right);
    }
}

void saveTree(struct node* tree, FILE* file) {
    if (tree != 0) {
        fwrite(tree, sizeof(struct node), sizeof(char), file);
        saveTree(tree->left, file);
        saveTree(tree->right, file);
    }
}

void printTree(FILE* file, char* interface) {
    struct node* tree = (struct node*)malloc(sizeof(struct node));

    printf("Current iface: %s\n", interface);
    while (fread(tree, sizeof(struct node), sizeof(char), file) != 0) {
        printf("IP-address - %s, Count - %d\n", tree->ip, tree->counter);
    }

    free(tree);
}

