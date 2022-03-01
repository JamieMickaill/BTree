/*############################################################################

    COMP2017 USYD 2021 SEMSTER 1
    
    ASSIGNMENT 5 : "BTREE"

    SID: 500611960

    BTREESTORE.H

    "Library of functions used to implement a btree with multithreading capabilities"

#############################################################################*/

#ifndef BTREESTORE_H
#define BTREESTORE_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <limits.h>
#include <pthread.h>

/*############################################################################

    STRUCTS

#############################################################################*/

// BTREE DATA STRUCTURE
struct btree {
   struct tree_node * root;
    size_t counter;
    size_t branching;
    size_t threads;
    pthread_mutex_t Mlock;

};

// VALE DATA STRUCTURE
struct info {
    //• Size of the actual data for this value in bytes 
    //(the maximum size is 232-1 bytes)
    uint32_t size;
    //• The encryption key
    uint32_t key[4];
    //• The “nonce” value used for TEA-CTR
    uint64_t nonce;
    //• The actual encrypted data for this value
    void * data;
};

//THREAD ARG
struct thread_decrypt{
    uint64_t * cypher;
    uint32_t key[4];
    uint64_t nonce;
    uint64_t *plain ;
    uint32_t num_blocks;
};

//BTREE NODE
struct tree_node {
    uint8_t leaf;
    //number of keys IN THE NODE
    uint16_t num_keys;
    //how many keys CAN BE STORED
    uint16_t key_count;
    uint16_t min_keys;
    //pointer to keys
    uint32_t * keys;
    // pointer to value pointers
    struct info ** values;
    //pointer to array of children pointers
   struct tree_node ** children;
};

//EXPORT NODE
struct node {
    uint16_t num_keys; // The number of keys in this node
    uint32_t * keys; // The keys in this node, ordered
};

//SEARCH RESULT STRUCT
struct search_result{
   struct tree_node* node_ptr;
   struct tree_node * parent;
    uint32_t key;
    struct info* val;
    bool success;
};

/*############################################################################

    FUNCTIONS

#############################################################################*/

/*############################################################################

    INIT FUNCTIONS

#############################################################################*/

struct info * create_val();

struct tree_node * create_node(int num_keys, struct btree* btree);

struct btree* create_btree(size_t branching, size_t threads);

void * init_store(uint16_t branching, uint8_t n_processors);

/*############################################################################

    Free/EXIT Functions

#############################################################################*/

void close_store(void * helper);

void free_node(struct tree_node * node);

void free_children(struct tree_node * root, struct btree * btree);

/*############################################################################

    Search Functions

#############################################################################*/


struct search_result* create_result();

struct search_result* btsearch(uint32_t key_to_find,struct tree_node * node_search, struct tree_node * parent);

int btree_retrieve(uint32_t key, struct info * found, void * helper);

/*############################################################################

    INSERT FUNCTIONS

#############################################################################*/


void split_node(struct tree_node * parent,struct tree_node * child, int index, struct btree* btree);

void insert_into_leaf(struct tree_node * node, uint32_t key, 
    struct info * value, struct btree * btree);

int insert(uint32_t key, struct info * value, struct btree * btree);

int btree_insert(uint32_t key, void * plaintext, size_t count, uint32_t encryption_key[4], uint64_t nonce, void * helper);

int recursive_size_check(struct tree_node * parent, 
    struct btree* btree, int is_root);

/*############################################################################

    DELETE FUNCTIONS

#############################################################################*/

void merge_child_left(struct tree_node * node, struct tree_node * child_1, struct tree_node* child_2, int i, struct btree* btree);
void merge_child_right(struct tree_node * node, struct tree_node * child_1, struct tree_node* child_2, int i, struct btree* btree);

void merge_empty_child_right(struct tree_node * node, struct tree_node * child_1, 
    struct tree_node* child_2, int i, struct btree* btree);

void merge_empty_child_left(struct tree_node * node, 
    struct tree_node * child_1, struct tree_node* child_2, int i, struct btree* btree);

void steal_from_thy_left_neighbour(struct tree_node* root, 
    struct tree_node *left, struct tree_node * target, int i);

void steal_from_thy_right_neighbour(struct tree_node* root, 
    struct tree_node *right, struct tree_node * target, int i);

int steal_max_from_left(struct tree_node *node);

int steal_min_from_right(struct tree_node *node);

int recursive_rebalance(struct tree_node * root, 
    struct tree_node * parent, int index, struct btree* btree);

int deletion_algorithm(int key, struct tree_node * root, struct btree * btree);

int btree_delete(uint32_t key, void * helper);

/*############################################################################

    EXPORT FUNCTIONS
    
#############################################################################*/

uint64_t btree_export(void * helper,struct node ** list);

uint64_t recursive_export(struct tree_node * node, struct node * list, 
    uint64_t *index, uint64_t * count);

/*############################################################################

    Encrypt/Decrypt Functions

#############################################################################*/

void * thread_decrypt(void * decrypt_info);

void encrypt_tea(uint32_t plain[2], uint32_t cipher[2], uint32_t key[4]);

void decrypt_tea(uint32_t cipher[2], uint32_t plain[2], uint32_t key[4]);

uint64_t encrypt_tea_64(uint64_t *plain,  uint32_t key[4]);

void encrypt_tea_ctr(uint64_t * plain, uint32_t key[4], uint64_t nonce, uint64_t * cipher, uint32_t num_blocks);

void decrypt_tea_ctr(uint64_t * cipher, uint32_t key[4], uint64_t nonce, uint64_t * plain, uint32_t num_blocks);

int btree_decrypt(uint32_t key, void * output, void * helper);



#endif
