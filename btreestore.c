/*############################################################################

    COMP2017 USYD 2021 SEMSTER 1
    
    ASSIGNMENT 5 : "BTREE"

    SID: 500611960

    BTREESTORE.C

    "Library of functions used to implement a btree with multithreading capabilities"

#############################################################################*/

#include "btreestore.h"

//Resources
//https://gist.github.com/yorickdewid/d86e14cb2f3929823841
//https://gist.github.com/nlowe/6e2bf999d53b2bb31321d30aafdd510a
//https://github.com/tidwall/btree.c/blob/master/btree.c


/*############################################################################

    FUNCTIONS

#############################################################################*/

/*############################################################################

    INIT FUNCTIONS

#############################################################################*/

/*############################################################################

    Create_VAL()

    Creates an INFO data structure which contains the encryption info, 
    encrypted data, key and nonce.
    This maps to each key as its value.

#############################################################################*/

struct info * create_val(){
    struct info * val;
    val = malloc(sizeof(struct info));
    // printf("VALUE CREATED %p\n",val);
    val->data = NULL;
    for(int i=0; i<4;i++){
        val->key[i] = 0;
    }
    val->nonce = 0;
    val->size = 0;
    return val;
}

/*############################################################################

    Create_NODE()

    Creates an Btree Node for the Btree data structure .
    Max children = branching factor.
    Max keys = branching factor-1
    Min keys = ceil(branching factor/2)-1
    Returns pointer to the newly allocated node.
    
#############################################################################*/

struct tree_node * create_node(int branching_factor, struct btree * btree){

   struct tree_node * new_node;
    new_node = (struct tree_node*)malloc(sizeof(struct tree_node));

    if(!new_node){
        perror("OOM");
        return NULL;
    }

    //printf("NODE CREATED PTR %p \n",new_node);

    new_node->key_count = branching_factor-1;

    double bf = branching_factor;
    new_node->min_keys = 1;

    new_node->keys = malloc(branching_factor*sizeof(uint32_t));

    //init keys to INT MIN
    for(int i=0; i<branching_factor; i++){
        new_node->keys[i] = INT_MIN;
    }

    //init array of val PTRs
    new_node->values = 
        (struct info**)malloc(branching_factor*sizeof(struct info*));

     //init val ptrs to zero
    for(int i=0; i<branching_factor; i++){
        new_node->values[i] = NULL;
    }
    // printf("VALS %p\n",new_node->values);

    //create array of children
    new_node->children = 
        (struct tree_node**)malloc((branching_factor+1)*sizeof(struct tree_node*));

     //init children to NULL
    for(int i=0; i<branching_factor+1; i++){
        new_node->children[i] = NULL;
    }

    //Set key count
    new_node -> num_keys = 0;
    new_node -> leaf = 1;

    //increment btree counter
    btree->counter +=1;

    return new_node;

}

/*############################################################################

    Create_Btree()

    Creates an Btree data structure .
    Max children = branching factor.
    Max keys = branching factor-1
    Min keys = ceil(branching factor/2)-1
    Returns pointer to the newly allocated Btree.
    
#############################################################################*/

struct btree* create_btree(size_t branching, size_t threads){

    //printf("Creating Btree -> BF:%d  THR:%d", branching, threads);
    // fflush(stdout);

    struct btree* tree_root = (struct btree*)(malloc(sizeof(struct btree)));
    tree_root->branching = branching;
    tree_root->counter = 0;
    tree_root->threads = threads;

    if (!tree_root){
        perror("OOM!");
        return NULL;
    }

   struct tree_node * head = create_node(tree_root->branching, tree_root);

    tree_root->root = head;

    //init lock
    pthread_mutex_t Mlock = PTHREAD_MUTEX_INITIALIZER;
    tree_root->Mlock = Mlock;
    return tree_root;

}

/*############################################################################

    INIT_STORE()

    This function will be called exactly once before any other functions.
    Initialises btree data structure and returns a pointer to the btree

#############################################################################*/

void * init_store(uint16_t branching, uint8_t n_processors) {

    if(branching <2 || n_processors<1){
        perror("Branching < 2!");
        return NULL;
    }

    struct btree * btree;
    btree = create_btree(branching, n_processors);
    return (void*)btree;
}


/*############################################################################

    Create_Result()

    Creates a Result data structure.
    Used for btree_recieve()
    If recieve successful, contains the key, value, node and parent pointers.
    
#############################################################################*/

struct search_result* create_result(){

    struct search_result* search_result;
    search_result = malloc(sizeof(struct search_result));

    if(!search_result){
        perror("OOM");
        return NULL;
    }

    search_result->node_ptr = NULL;
    search_result->parent = NULL;
    search_result->success = 0;
    search_result->key = 0;
    search_result->val = NULL; 

    return search_result;
}


/*############################################################################

    INSERT FUNCTIONS

#############################################################################*/

/*############################################################################

    Split node

    Given a parent (new root) and a child (old root as child index of new root), 
    - splits childs right keys into a new node
    - sets new node to right child of parent
    - parent takes childs median key

#############################################################################*/

void split_node(struct tree_node * parent,struct tree_node * child, int index, struct btree * btree){

    //printf("Splitting node: parent %p, child %p\n", parent, child);
    // fflush(stdout);

    //index for median key
    double branch = parent->key_count +1;
    int middle = ceil(branch/2)-1;
    

    //Create new node
    struct tree_node * new_split_node = create_node(btree->branching, btree);
    new_split_node -> leaf = child->leaf;
    new_split_node -> num_keys = 0;

    //assign childrens right keys to new_node
    for(int i = 0;((i<(child->key_count-middle)) && 
        ((middle+1+i) < child->num_keys)); i++){

        new_split_node->keys[i] = child->keys[(middle)+1+i];

        child->keys[(middle)+1+i] = 0;

        //printf("Moved %d to %p\n",new_split_node->keys[i],new_split_node);

        new_split_node->values[i] = child->values[(middle)+1+i];
        child->values[(middle)+1+i] = NULL;
        
        // //printf("Assigned key %d from %x to new root right child %x\n",
        //     child->keys[(middle)+i+1],child, new_split_node);

        //fflush(stdout);

        new_split_node -> num_keys +=1;

    }

    //if child not leaf, move childs children to new node
    if(!child->leaf){
        for(int j = 0;(j < (child->key_count - middle)+1) && 
            ((middle+j+1) < child->num_keys+1)  ; j++){

            new_split_node->children[j] = child->children[middle+j+1];
            
            //printf("Moved child %p to new right child %p of root %p from child %p\n",child->children[middle+j+1], new_split_node, parent, child);

            child->children[middle+j+1] = NULL;
            fflush(stdout);
        }
    }

    //set key count of old root
    child->num_keys = middle;

    //shift parents children right
    for(int i = parent->num_keys;i>=index; i--){
        parent->children[i+1] = parent->children[i];
    }

    //parents child set to new split node
    parent->children[index] = new_split_node;

    //shift parents keys
    
    for(int i = parent->num_keys;(i>=index); i--){

        parent->keys[i] =  INT_MIN;
        parent->keys[i] = parent->keys[i-1];
        parent->values[i] = NULL;
        parent->values[i] = parent->values[i-1];

        //printf("Moved key %d to index %d\n",parent->keys[i-1],i-1);
        // fflush(stdout);
    }

    //parents key set to childs middle key
    parent->keys[index-1] = child->keys[middle];
    parent->values[index-1] = child->values[middle];
    child->keys[middle] = INT_MIN;
    child->values[middle] = NULL;

    //printf("Added median key %d to parent %x in index %d \n",parent->keys[index-1],parent, index-1);
    //add to parents count
    parent->num_keys+=1;

}

/*############################################################################

    Insert_into_leaf()

    Given a key,value and a leaf node, inserts the key&value into the node
    at the appropriate ordered index

#############################################################################*/

void insert_into_leaf(struct tree_node * node, uint32_t key, 
    struct info * value, struct btree * btree){        

    //printf("insert into node\n");
    fflush(stdout);

    int index = node->num_keys;

    if(node->leaf){
        //shift keys right 
        while(index>=1 && key < node->keys[index-1]){
            node->keys[index] = node->keys[index-1];
            node->values[index] = node->values[index-1];
            node->values[index-1] = NULL;
            index-=1;
        } 

        //printf("Inserting key %d into node %x, index %d\n",key,node,index);
        fflush(stdout);

        node->keys[index] = key;
        node->values[index] = value;
        node->num_keys +=1;

    }else{
        perror("Insert into non-leaf!\n");
        return;
    }

}

/*############################################################################

    Recursive_size_check()

    Recursively iterate over the tree in-order, checking each encountered
    node for the correct number of keys. 
    If too many keys, node is split

#############################################################################*/

int recursive_size_check(struct tree_node * parent, 
    struct btree* btree, int is_root){


    //printf("Size check, current root is %x\n",btree->root);
    fflush(stdout);

    //if not a leaf, iterate over children and check if too many keys
    if(!parent->leaf){
        
        //check children sizes
        for(int i=0; i<parent->num_keys+1 && i < parent->key_count+1;i++){

            //printf("index = %d\n",i);
            fflush(stdout);

            if(!parent->children[i]->leaf){

                //printf("Recursing on child %d of non-leaf parent %x\n",i,parent);
                int overflow_found = 0;

                overflow_found = 
                    recursive_size_check(parent->children[i], btree, 0);

                if (overflow_found){
                    return 1;
                }

            }

            //printf("Checking parent %x, node %x child %d\n", parent,parent->children[i],i);
            fflush(stdout);


            //if child i has too many keys, 
            //split it and pass one up to the parent

            if(parent->children[i]->num_keys> parent->key_count){

                //printf("OVERFLOW FOUND CHILD %d\n",i);
                fflush(stdout);

                //send median key to parent, 
                //right half to new node and left half remains.
                //printf("SPLIT NODE!1!\n");
                fflush(stdout);
                split_node(parent, parent->children[i], i+1, btree);

                //return 1 as we have performed a change 
                //and will require another check
                return 1;
            }
            
        }
        
    }

        //if checking the root node, it has no parent, 
        //therefore we will create a new root if we split
        //Every other child will be checked by its parent
        if(is_root){

            if(parent->num_keys > parent->key_count){

                //printf("OVERFLOW FOUND IN ROOT\n");
                fflush(stdout);

                //create a new node
                struct tree_node * new_node1 = create_node(btree->branching, btree);
                    //the new node is the root
                    struct tree_node * oldroot = btree->root;
                    btree->root = new_node1;
                    //thereore it is not a leaf
                    new_node1 -> leaf = 0;
                    //it has no keys yet
                    new_node1 -> num_keys = 0;
                    //its child is the old root
                    new_node1 -> children[0] = oldroot;

                    //we split the old root
                    //put the median into the new root
                    //put the right half into the new right child of root
                    //mode the right children to new right half
                    //leave left half in old root
                    //printf("SPLIT NODE!!!\n ");
                    fflush(stdout);
                    split_node(new_node1, oldroot,1,btree);
                    return 1;
            }
        }
    return 0;
}

/*############################################################################

    Insert()

    Given a key and value, inserts into the appropriate position in the btree
    First searches if k exists,
    then inserts into the leaf node where k would be,
    then recursively resizes the tree until all nodes satisfy branching property
    returns int 1 = non successful, 0 = success

#############################################################################*/

int insert(uint32_t key, struct info * value, struct btree * btree){

    struct search_result *sr;
    sr = btsearch(key,btree->root, NULL);

    if(sr->success){
        
        //printf("FOUND KEY\n");
        fflush(stdout);
        free(sr);
        return 1;

    }

    //SR contains location of leaf
    else{

        //printf("Inserted key %d and val %p into node %p\n",key,value,sr->node_ptr);
        fflush(stdout);
        insert_into_leaf(sr->node_ptr, key, value, btree);
        free(sr);

        int size_check = 1;
        //continue to perform size check on tree until no oversized nodes encountered.
        while(size_check){
            size_check = recursive_size_check(btree->root, btree,1);
        }

        return 0;

    }
}

/*############################################################################

    Btree_Insert()

    TEA Encrypts given data and stores in a value struct
    Given a key and value, inserts into the appropriate position in the btree
    Return 0 if the insertion is successful. Return 1 if the key already exists

#############################################################################*/

int btree_insert(uint32_t key, void * plaintext, size_t count, uint32_t encryption_key[4], uint64_t nonce, void * helper) {
    
    //lock tree
    struct btree * btree = helper;

    struct search_result * sr;


    sr = btsearch(key,btree->root,NULL);

    if(sr->success){
        free(sr);         
        return 1;
    }

    free(sr);

    //encrypt data
    double count1 = count;
    int blocks = ceil(count1/8);

    if(blocks == 0){
        blocks = 1;
    }

    int padding = 8-(count%8);
    if(padding ==8){
        padding = 0;
    }

    //printf("Blocks %d\n",blocks);

    struct info *value = create_val();
    value->data = malloc(blocks *sizeof(uint64_t));

    for(int i=0; i<4; i++){
        value->key[i] = encryption_key[i];
    }
    
    value->size = count;
    value->nonce = nonce;

    uint8_t * plaintextIn = plaintext;
    uint8_t * plain = calloc(count+padding, sizeof(uint8_t));

    for(int i=0; i<count; i++){
        plain[i] = 
        plaintextIn[i];
    }

    
    encrypt_tea_ctr((uint64_t*) plain, encryption_key, nonce, value->data, blocks);
    free(plain);

    //insert
    pthread_mutex_lock(&btree->Mlock);
    int insert_success=0;
    if((insert_success = insert(key, value, helper))==1){
        pthread_mutex_unlock(&btree->Mlock);
        return 1;
    }
    
    //printf("Key inserted: %d\n " , key);
    
    fflush(stdout);

    pthread_mutex_unlock(&btree->Mlock);
    
    return 0;

}

/*############################################################################

    Free/EXIT Functions

#############################################################################*/

/*############################################################################

    Free_node()

    given a tree_node, frees its allocated memory

#############################################################################*/

void free_node(struct tree_node * node){
        //free unused
    
    for(int i=0; i<node->num_keys;i++){
        //printf("FREEING VALUE %d %p\n",i,node->values[i]);
        fflush(stdout);
        if(node->values[i] !=NULL){
        free(node->values[i]->data);
        free(node->values[i]);
        node->values[i] = NULL;
        }
    }
    for(int i=node->num_keys; i<node->key_count + 1;i++){
        //printf("FREEING VALUE %d %p\n",i,node->values[i]);
        fflush(stdout);
        if(node->values[i] !=NULL){
        free(node->values[i]);
        }
        
    }
    free(node->keys);
    free(node->children);
    free(node->values);
    //printf("Freeing node %p values %p\n",node,node->values);
    fflush(stdout);
    free(node);

}

/*############################################################################

    Free_children()

    given a tree and node, recursively frees each nodes allocated memory

#############################################################################*/

void free_children(struct tree_node * root, struct btree * btree){

    //printf("Freeing %x, LEAF=%d\n",root, root->leaf);
    fflush(stdout);

    //no nodes to free
    if(btree->counter == 0){
        return;
    }

    //not a leaf, traverse over children recursively
    if(!root->leaf){

        //actual number of children        
        int num_children = root->num_keys+1;
        
        //printf("Freeing children of %x\n",root);
        fflush(stdout);

        //free all children
        for(int i = 0; i<num_children;i++){
            free_children(root->children[i],btree);
        }

        //printf("Freeing remaining children pointers of %x\n",root);
        fflush(stdout);

        //free pointers of unused children
        for(int i=num_children; i<root->key_count+2; i++){
            //printf("Freeing %p\n",root->children[i]);
            fflush(stdout);
            free(root->children[i]);
        }

        //printf("Freeing remaining pointer to children pointers of %x\n",root);
        fflush(stdout);

        free(root->children);

        //free all values

        //printf("Freeing used info pointers/structs and data of %x\n",root);
        fflush(stdout);

        //free used values
        for(int i=0; i<root->num_keys;i++){
            //printf("Freeing key %d value %p %p\n",root->keys[i],root->values[i], &(root->values[i]));
            fflush(stdout);
            free((root->values[i]->data));
            free(root->values[i]);
            root->values[j] = NULL;
        }

        //printf("Freeing unused info struct pointers of %x\n",root);
        fflush(stdout);

        //free value pointers unused
        for(int i=root->num_keys; i<root->key_count+1;i++){
            //printf("Freeing key %d value %p %p\n",root->keys[i],root->values[i]);

            free(root->values[i]);
            }
    
        //printf("Freeing pointer to info struct pointers %p of %x\n",root->values,root);
        fflush(stdout);
        free(root->values);

        // //printf("Freeing keys pointer of %x\n",root);
        fflush(stdout);

        free(root->keys);

        //printf("Freeing %p\n",root);
        fflush(stdout);

        free(root);

    }else{

        //node is leaf
        //free root children

        //actual number of children        
        int num_children = root->num_keys+1;

        //printf("Freeing unused children pointers of leaf %x\n",root);

        //free unused child pointers and pointer to pointers
        for(int i=0; i<root->key_count+1;i++){
            //printf("Freeing %p\n",root->children[i]);
            fflush(stdout);
            free(root->children[i]);
        }

        //printf("Freeing remaining pointer to children pointers of %x\n",root);
        fflush(stdout);
        free(root->children);

        //free used values

        //printf("Freeing used info pointers/structs and data of %x\n",root);
        fflush(stdout);

        for(int i=0; i<root->num_keys;i++){
            //printf("Freeing index %d Freeing leaf key %d value %p %p\n",i,root->keys[i],root->values[i], &(root->values[i]));
            fflush(stdout);
            free(root->values[i]->data);
            free(root->values[i]);
        }

        //free value pointers unused

        //printf("Freeing unused info struct pointers of leaf %x\n",root);
        fflush(stdout);
        for(int i=root->num_keys; i<root->key_count+1;i++){
            //printf("Freeing index %d key %d value %p %p\n",i,root->keys[i],root->values[i], &(root->values[i]));
            fflush(stdout);
            free(root->values[i]);
            }
    
        // //printf("Freeing pointer to info struct pointers %p of %x\n",root->values,root);
        fflush(stdout);

        free(root->values);

        //printf("Freeing keys pointer of %x\n",root);
        free(root->keys);

        //printf("Freeing %p\n",root);
        fflush(stdout);
        free(root);

    }

}

/*############################################################################

    Close_Store()

    Calls free functions on all allocated memory
    Used when closing Btree

#############################################################################*/

void close_store(void * helper) {
    // Your code here

    struct btree * tree = (struct btree*)helper;
   struct tree_node * root = tree->root;
     pthread_mutex_destroy(&tree->Mlock); 
    free_children(root, tree);
    free(tree);

    return;
}

/*############################################################################

    Search Functions

#############################################################################*/

/*############################################################################

    btsearch()

    Searches down tree to find key
    returns a search_result struct containing the found key, its index,
    node_ptr, parent and value.
    search_result->success = 1 if found, 0 if not found.

#############################################################################*/

struct search_result* btsearch(uint32_t key_to_find,
    struct tree_node * node_search, struct tree_node * parent){

    int index = 0;

    //iterate over node keys while  key > key index
    while((index < node_search->num_keys) && 
        (key_to_find > node_search->keys[index])){
        index+=1;
    }

    //if we iterated too far, set index back to prevent segfault
    if(index == node_search->key_count+1){
        index -=1;
    }

    //if we found the key, 
    // construct the result containing the node and index within the node
    if((index <= node_search->num_keys)&& key_to_find == node_search->keys[index]){

        struct search_result * sr = create_result();
        sr->key = index;
        sr->val = node_search->values[index];
        sr->success = 1;
        sr->node_ptr = node_search;
        sr->parent = parent;

        return sr;

    }

    //else, if we are searching a leaf
    //there are no more children, search failed

    if(node_search->leaf){

        struct search_result * sr = create_result();
        sr->success = 0;
        sr->node_ptr = node_search;
        sr->parent = parent;
        sr->key = index;

        return sr;  

    }else{

        //if we have children to search, 
        //search the child which may contain the key
        return btsearch(key_to_find, node_search->children[index], node_search);
    }

}

/*############################################################################

    btree_retrieve()

    retrieves requested key and value from the tree, writing the value info
    to the struct * found.
    Data pointer is returned encrypted.
    If successful, returns 0, otherwise, returns 1 For error
    Does not modify the data structure

#############################################################################*/

int btree_retrieve(uint32_t key, struct info * found, void * helper) {


    struct search_result * sr;
    struct btree * btree = helper;

    sr = btsearch(key,btree->root,NULL);
    
    if(sr->success){

        found->data = sr->val->data;

        for(int i=0; i<4; i++){
            found->key[i] = sr->val->key[i];
        }

        found->nonce = sr->val->nonce;
        found->size = sr->val->size;

        free(sr);

        return 0;

    }else{

        free(sr);
        return 1;

    }
}

/*############################################################################

    DELETE FUNCTIONS

#############################################################################*/

/*############################################################################

    merge_child()

    Takes a parent and its two children,
    Moves child2 keys and median of parent to child_1
    Moves child2 children to child1

#############################################################################*/

void merge_child_left(struct tree_node * node,
     struct tree_node * child_1, struct tree_node* child_2, int i, struct btree* btree){

         //printf("MC");
    double numkeys = child_1->key_count;
    int middle = ceil((numkeys)/2)-1;

    int merge_root = 0;
    if(node == btree->root){
        merge_root = 1;
    }

    //move non-target keys to target 
    for(int j=middle+1; j<child_2->num_keys+middle+1; j++){
        child_1->keys[j] = child_2->keys[j-(middle+1)];
        child_2->keys[j-(middle+1)] = INT_MIN;
        child_1->values[j] = child_2->values[j-(middle+1)];
        child_2->values[j-(middle+1)] = NULL;
        child_1->num_keys +=1;
    }

    //Assign median of root to child1
    child_1->keys[middle] = node->keys[i];
    node->keys[i]= INT_MIN;
    child_1->values[middle] = node->values[i];
    node->values[i] = NULL;
    child_1->num_keys +=1;

    //if merging nonleaf, copy children
    if(!child_2->leaf){
        for(int x=middle+1; x<numkeys+1; x++){
            child_1->children[x] = child_2->children[x-(middle+1)];
        }
    }

    //set parent keys
    for(int z = i+1; z<node->num_keys;z++){
        node->keys[z-1] = INT_MIN;
        node->keys[z-1] = node->keys[z];
        node->values[z-1] = NULL;
        node->values[z-1] = node->values[z];
        node->children[z] = node->children[z+1];
    }

    for(int z = i+1; z<node->num_keys+1;z++){
        node->children[z] = node->children[z+1];
    }

    node->values[node->num_keys-1] = NULL;
    node->num_keys -=1;

    if(merge_root && btree->root->num_keys == 0){
        //printf("BTREEEE");
        btree->root = child_1;
        child_1->leaf=0;
        free_node(node);
        btree->counter-=1;
    }

    for(int i=0; i< child_2->key_count+1; i++){
        child_2->values[i] = NULL;
        child_2->keys[i] = INT_MIN;
    }
    child_2->num_keys = 0;
    free_node(child_2);

    btree->counter-=1;
  
}


void merge_child_right(struct tree_node * node,
     struct tree_node * child_1, struct tree_node* child_2, int i, struct btree* btree){

         //printf("MC");
    double numkeys = child_1->key_count;
    int middle = ceil((numkeys)/2)-1;

    int merge_root = 0;
    if(node == btree->root){
        merge_root = 1;
    }

    //move non-target keys to target 
    for(int j=middle+1; j<child_2->num_keys+middle+1; j++){
        child_1->keys[j] = child_2->keys[j-(middle+1)];
        child_2->keys[j-(middle+1)] = INT_MIN;
        child_1->values[j] = child_2->values[j-(middle+1)];
        child_2->values[j-(middle+1)] = NULL;
        child_1->num_keys +=1;
    }

    //Assign median of root to child1
    child_1->keys[middle] = node->keys[i];
    node->keys[i]= INT_MIN;
    child_1->values[middle] = node->values[i];
    node->values[i] = NULL;
    child_1->num_keys +=1;

    //if merging nonleaf, copy children
    if(!child_2->leaf){
        for(int x=middle+1; x<numkeys+1; x++){
            child_1->children[x] = child_2->children[x-(middle+1)];
        }
    }

    //set parent keys
    for(int z = i+1; z<node->num_keys;z++){
        node->keys[z-1] = INT_MIN;
        node->keys[z-1] = node->keys[z];
        node->values[z-1] = NULL;
        node->values[z-1] = node->values[z];
        node->children[z] = node->children[z+1];
    }

    for(int z = i+1; z<node->num_keys+1;z++){
        node->children[z] = node->children[z+1];
    }

    node->values[node->num_keys-1] = NULL;
    node->num_keys -=1;

    if(merge_root && btree->root->num_keys == 0){
        btree->root = child_1;
        child_1->leaf=0;
        free_node(node);
        btree->counter-=1;
    }

    for(int i=0; i< child_2->key_count+1; i++){
        child_2->values[i] = NULL;
        child_2->keys[i] = INT_MIN;
    }
    child_2->num_keys = 0;
    free_node(child_2);

    btree->counter-=1;
  
}

/*############################################################################

    merge_empty_child()

    Takes a parent and its two children,
    Moves child2 keys and median of parent to child_1
    Moves child2 children to child1
    Assumes child1 is empty, writing to key index 0
    Copies indexes in reference to merging with right child

#############################################################################*/

void merge_empty_child_right(struct tree_node * node, struct tree_node * child_1, 
    struct tree_node* child_2, int i, struct btree* btree){
    
    //printf("MEC");
    //printf("Merging node %p,  child1: %p,  child_2:%p\n",node,child_1,child_2);
    fflush(stdout);
    // child_1->num_keys = child_1->key_count-1;

    double numkeys = child_1->key_count;
    int middle = ceil((numkeys)/2)-1;

    int merge_root = 0;

    //printf("node:%p and root%p\n",node,btree->root);
    fflush(stdout);
    if(node == btree->root){
        fflush(stdout);
        merge_root = 1;
    }


    //move child 2 keys to target 
    for(int j=middle; j<child_2->num_keys+1; j++){
        child_1->keys[j] = child_2->keys[j-(middle)];
        child_2->keys[j-(middle)] = INT_MIN;
        if(child_1->values[j] !=NULL){
        //printf("VALUE FREED CHILD1 FOR %p\n",child_1->values[j]);

        free(child_1->values[j]);
        child_1->values[j] = NULL;
        }

        child_1->values[j] = child_2->values[j-(middle)];
        child_2->values[j-(middle)] = NULL;
        child_1->num_keys +=1;
        //printf("Moved key %d to child1\n",child_1->keys[j]);

    }

    //take median from parent
    //middle-1 should be 0?
    child_1->keys[0] = node->keys[i];
    node->keys[i]= INT_MIN;
    if(child_1->values[0] != NULL){
        //printf("VALUE FREED CHILD1 FOR %p\n",child_1->values[0]);
        free(child_1->values[0]);
    }
    child_1->values[0] = node->values[i];
    node->values[i] = NULL;
    child_1->num_keys +=1;

    //if merging nonleaf, copy children
    if(!child_2->leaf){
        for(int x=middle+1; x<child_2->num_keys+(middle+1)+1; x++){
            child_1->children[x-1] = child_2->children[x-(middle+1)];
            //printf("MOVED CHILD NODE %p TO CHILD 1 %p\n",child_2->children[x-(middle+1)],child_1);
            fflush(stdout);
            child_2->children[x-(middle+1)] = NULL;
        }
    }

    child_2->num_keys =0;

    //set parent keys
    for(int z = i+1; z<node->num_keys;z++){
        node->keys[z-1] = INT_MIN;
        node->keys[z-1] = node->keys[z];
        node->values[z-1] = NULL;
        node->values[z-1] = node->values[z];
        node->children[z] = node->children[z+1];
        node->children[z+1] = NULL;
    }
    node->values[node->num_keys-1] = NULL;
    node->num_keys -=1;
    

    if(merge_root && btree->root->num_keys == 0){
        //printf("FREEING ROOT\n");
        fflush(stdout);
        btree->root = child_1;
        child_1->leaf=0;
        free_node(node);
        btree->counter -=1;
    }

    for(int i=0; i< child_2->key_count+1; i++){
        // free(child_2->values[i]);
        child_2->values[i] = NULL;
        child_2->keys[i] = INT_MIN;
    }
    child_2->num_keys = 0;

    //printf("FREEING CHILD2 %p\n",child_2);
    fflush(stdout);
    free_node(child_2);
    btree->counter-=1;
  
}

/*############################################################################

    merge_empty_child_left()

    Takes a parent and its two children,
    Moves child2 keys and median of parent to child_1
    Moves child2 children to child1
    Assumes child1 is empty, writing to key index 0
    Copies indexes in reference to merging with left child

#############################################################################*/

void merge_empty_child_left(struct tree_node * node, 
    struct tree_node * child_1, struct tree_node* child_2, int i, struct btree* btree){
    
    //printf("MECL");

    //printf("Merging node %p,  child1: %p,  child_2:%p index %d\n",node,child_1,child_2,i);
    fflush(stdout);
    double numkeys = child_1->key_count;
    int middle = ceil((numkeys)/2)-1;

    int merge_root = 0;
    if(node == btree->root){
        merge_root = 1;
    }

    //move non-target keys to target 
    for(int j=middle+1; j<child_2->num_keys+1; j++){
        child_1->keys[j] = child_2->keys[j-(middle)];
        child_2->keys[j-(middle)]= INT_MIN;
        child_1->values[j] = child_2->values[j-(middle)];
        child_2->values[j-(middle)] = NULL;
                child_1->num_keys +=1;
    }

    child_1->keys[middle] = node->keys[i];
    child_1->values[middle] = node->values[i];
    node->keys[i]= INT_MIN;
    node->values[i] = NULL;
    child_1->num_keys +=1;

    //if merging nonleaf, copy children
    if(!child_2->leaf){
        for(int x=middle+1; x<numkeys+1; x++){
            child_1->children[x] = child_2->children[x-(middle+1)];
            child_2->children[x-(middle+1)] = NULL;
        }
    }

    //set parent keys
    for(int z = i+1; z<node->num_keys;z++){
        node->keys[z-1] = INT_MIN;
        node->keys[z-1] = node->keys[z];
        node->values[z-1] = NULL;
        node->values[z-1] = node->values[z];
        node->children[z] = node->children[z+1];
        node->children[z+1] = NULL;
    }
    node->values[node->num_keys-1] = NULL;
    node->num_keys -=1;

    if(merge_root && node->num_keys == 0){
        btree->root = child_1;
        child_1->leaf=1;
        //printf("FREEING ROOT\n");
        fflush(stdout);
        free_node(node);
        btree->counter-=1;
    }

    for(int i=0; i< child_2->key_count+1; i++){
        child_2->values[i] = NULL;
        child_2->keys[i]= INT_MIN;
    }
    child_2->num_keys = 0;

    //printf("FREEING CHIDL@2\n");
    fflush(stdout);
    free_node(child_2);

    btree->counter-=1;
  
}

/*############################################################################

    steal_from_thy_left_neighbour()

    Takes a left neighbour and target
    Takes largest key from left neighbour,
    puts it into the parent
    and takes a parent key into the target.

#############################################################################*/

void steal_from_thy_left_neighbour(struct tree_node* root, 
    struct tree_node *left, struct tree_node * target, int i){

    target->num_keys+=1;

    for(int j=target->num_keys-1; j>0;j--){
        target->keys[j] = INT_MIN;
        target->keys[j] = target->keys[j-1];
        // free(target->values[j]->data);
        if(target->values[j] != NULL){
            //printf("FREED VAL %p",target->values[j]);
            free(target->values[j]);
        }
        target->values[j] = NULL;
        target->values[j] = target->values[j-1];
    }

    target->keys[0] = root->keys[i];
    // free(target->values[0]->data);
    // target->values[0] = NULL;
    if(target->values[0]!=NULL){
        //printf("FREED VAL %p",target->values[0]);
        // free(target->values[0]);
    }
    target->values[0] = root->values[i];
    root->keys[i] = INT_MIN;
    root->keys[i] = left->keys[left->num_keys-1];
    // free(root->values[i]->data);
    // root->values[i] = NULL;
    root->values[i] = left->values[left->num_keys-1];
    // free(left->values[left->num_keys-1]->data);
    // left->values[left->num_keys-1] = NULL;

    if(!left->leaf){
        for(int x=target->num_keys; x>0; x--){
            target->children[x] = target->children[x-1];
        }
        target->children[0] = left->children[left->num_keys];
        left->children[left->num_keys] = NULL;
    }
    
    left->values[left->num_keys-1] = NULL;
    left->num_keys-=1;
}

/*############################################################################

    steal_from_thy_right_neighbour()

    Takes a right neighbour and target
    Takes smallest key from right neighbour,
    puts it into the parent
    and takes a parent key into the target.

#############################################################################*/

void steal_from_thy_right_neighbour(struct tree_node* root, 
    struct tree_node *right, struct tree_node * target, int i){
    
    //printf("Stealing: root%p, right:%p, target:%p\n",root,right,target);
    fflush(stdout);



    target->num_keys+=1;
    target->keys[target->num_keys-1] = root->keys[i];
    root->keys[i] = INT_MAX;
    if(target->values[target->num_keys-1] != NULL){
        //printf("FREED VAL %p",target->values[target->num_keys-1]);
        free(target->values[target->num_keys-1]);
    }
    target->values[target->num_keys-1] = root->values[i];
    // free(root->values[i]->data);
    //printf("root val = %p\n",root->values[i]);
    // root->values[i] = NULL;

    //printf("Root took %d\n",right->keys[0]);
    fflush(stdout);

    root->keys[i] = right->keys[0];
    right->keys[0] = INT_MAX;
    root->values[i] = right->values[0];


    for(int j=0; j<right->num_keys-1; j++){
        right->keys[j] = NULL;
        right->keys[j] = right->keys[j+1];
        // free(right->values[j]->data);
        // right->values[j] = NULL;
        right->values[j] = right->values[j+1];
    }

    if(!right->leaf){
        target->children[target->num_keys] = right->children[0];
        for(int x=0; x<right->num_keys; x++){
            right->children[x] = right->children[x+1];
        }
    }

    right->values[right->num_keys-1] = NULL;
    right->num_keys-=1;
    // free(right->values[right->num_keys]->data);
    // right->values[right->num_keys] = NULL;

}

/*############################################################################

    steal_max_from_left()

    Traverses given left node and 
    returns the largest key held within the subtree

#############################################################################*/

int steal_max_from_left(struct tree_node *node){
    
	if(!node->leaf){
		return steal_max_from_left(node->children[node->num_keys]);
	}else{
		return node->keys[node->num_keys-1];
	}
}

/*############################################################################

    steal_min_from_right()

    Traverses given right node and 
    returns the smallest key held within the subtree
    
#############################################################################*/

int steal_min_from_right(struct tree_node *node){
	if(!node->leaf){
		return steal_min_from_right(node->children[0]);
	}else{
		return node->keys[0];
	}
}

/*############################################################################

    recursive_rebalance()

    Traverses tree inorder and rebalances any nodes with < min_keys
    first attempts to take from neighbours, before merging with siblings
    
#############################################################################*/

int recursive_rebalance(struct tree_node * root, 
    struct tree_node * parent, int index, struct btree* btree){
    
    //printf("Curr root %p\n",btree->root);
    fflush(stdout);

    //check if only root
    if(root == btree->root){
        struct tree_node * c1 = NULL;
        struct tree_node * c2 = NULL;
        if(root->children[0] != NULL){
            c1 = root->children[0];
        }
        if(root->children[1] != NULL){
            c2 = root->children[1];
        }
        if(!c1 || !c2){
            root->leaf = 1;
            return 0;
        }
    }

    //if not a leaf check self then children
    if(!root->leaf){
        
        //we may end up with an internal with missing nodes due to merging
        //we want to recurse inorder
        if(root->num_keys < root->min_keys){

            //need to rebalance this node
            //printf("rebalancing internal\n");
            fflush(stdout);

            //Firstly, check if the immediate left sibling 
            //has more than the minimum number of keys
            //if left neighbour sibling has enough keys, steal
            if((index > 0) && (parent->children[index-1]->num_keys > root->min_keys)){

                //printf("Stealing from left neighbour\n");
                fflush(stdout);

                steal_from_thy_left_neighbour(parent, 
                    parent->children[index-1],parent->children[index],index-1);

                return 1;

            //try right
            }else if ((parent != NULL) &&(index < parent->num_keys) && 
                (parent->children[index+1]->num_keys > root->min_keys)){

                //printf("Stealing from right neighbour\n");
                fflush(stdout);

                steal_from_thy_right_neighbour(parent, 
                    parent->children[index+1],parent->children[index],index);

                return 1;

            //merge with left
            }else if ( (index > 0)){
                //printf("Merging with right, index: %d \n",index-1);

                if(parent->children[index]->num_keys > 0){
                    merge_child_left(parent,parent->children[index-1],
                    parent->children[index], index-1, btree);
                    }else{
                merge_empty_child_left(parent,parent->children[index-1], 
                    parent->children[index], index-1, btree);
                    }
                return 1;

            //merge with right
            }else{
                //printf("Merging with right, index: %d \n",index);
                fflush(stdout);

                if(parent->children[index]->num_keys > 0){
                    merge_child_right(parent,parent->children[index],
                    parent->children[index+1], index, btree);
                }else{
                merge_empty_child_right(parent,parent->children[index],
                    parent->children[index+1], index, btree);
                }

                return 1;
            }

            return 1;

        }

        //we checked the current node, now check its children
        for(int i=0; i<root->num_keys+1;i++){

            //printf("checking child %d node : %p",i, root->children[i]);
            fflush(stdout);

            if(recursive_rebalance(root->children[i], root, i, btree) == 1){
                return 1;
            }
        }
        return 0;
    }   
    else{

        //leaf node
        if(root->num_keys < root->min_keys){

            //need to rebalance this node
            //printf("rebalancing leaf!\n");
            fflush(stdout);

            //if no parent, we have the root
            //if the root is a leaf, and doesnt have enough keys, return
            if(parent == NULL){
                return 0;
            }


            //if left neighbour sibling has enough keys, steal
            fflush(stdout);
            if((index > 0) && (parent->children[index-1]->num_keys > root->min_keys)){
                //printf("stealing from sibling left \n");
                fflush(stdout);
                steal_from_thy_left_neighbour(parent, parent->children[index-1],parent->children[index],index-1);
                return 1;

            }else if ((index < parent->num_keys) && 
                (parent->children[index+1]->num_keys > root->min_keys)){

                //printf("stealing from sibling right \n");
                fflush(stdout);
                steal_from_thy_right_neighbour(parent, 
                    parent->children[index+1],parent->children[index],index);

                return 1;

            }else if ( (index > 0)){
                //merge with left
                //printf("merging with left 1 idnex: %d\n",index);
                fflush(stdout);
                if(parent->children[index]->num_keys > 0){
                    merge_child_left(parent,parent->children[index-1],
                    parent->children[index], index-1, btree);
                    }else{
                merge_empty_child_left(parent,parent->children[index-1], 
                    parent->children[index], index-1, btree);
                    }
                    
                return 1;

            }else{
                //merge with right
                //printf("merging with right 1 idnex: %d\n",index);
                fflush(stdout);
                if(parent->children[index]->num_keys > 0){
                    merge_child_right(parent,parent->children[index],
                    parent->children[index+1], index, btree);
                }else{
                merge_empty_child_right(parent,parent->children[index],
                    parent->children[index+1], index, btree);
                }

                return 1;
            }
            return 1;
        }
    }return 0;
}

/*############################################################################

    deletion_algorithm()

    Finds key,
    If in leaf -> delete
    If internal -> replace with maxleft and delete from leaf or merge
    Deletes key and recursively rebalances
    
#############################################################################*/

int deletion_algorithm(int key, struct tree_node * root, struct btree * btree){

    // //printf("**************Delete called KEY: %d ************\n",key);
    fflush(stdout);

    //Case 1: leaf node as root
    if(root->leaf){

        //printf("Delete %d on root\n", key);
        fflush(stdout);

        int j = 0;
        //find the key
        while((j<root->num_keys) && (key>root->keys[j])){
            j+=1;
        }

        if(root->keys[j] == key){

            //if the last key in array, free its data
            if(j == root->num_keys-1 ){
                // key to remove is last key, 
                // just remove it
            if(root->values[j] != NULL){
               free(root->values[j]->data);
                free(root->values[j]);
                root->values[j] = NULL;
            }
                // //printf("VALUE DATA FREED FOR %p\n",root->values[j]);

                if(root->num_keys ==1){

                    // //if freeing last root node, ensure no duplicate vals are free
                    // if(root->values[j] != NULL){
                    // free(root->values[j]);
                    // root->values[j] = NULL;
                    // }

                }
                
                root->keys[j] == INT_MIN;
                root->values[root->num_keys-1] = NULL; 
                root->num_keys -=1;
            }else{

                //shift values to remove key
                for(int x=0; x<root->num_keys-1;x++){
                    root->keys[x] = INT_MIN;
                    root->keys[x] = root->keys[x+1];

                    //printf("FREEING VALUE and DATA %p\n",root->values[x]);
                    free(root->values[x]->data);
                    free(root->values[x]);
                    root->values[x] == NULL;
                    root->values[x] = root->values[x+1];
                    
                }
            // free(root->values[root->num_keys-1]);
            root->values[root->num_keys-1] = NULL;
            root->num_keys-=1;}

        }
        else{
            perror("No node found in root!\n");
            return 1;
        }

    }else{

        //Case 2: Internal node

        int z=0;
        struct tree_node * before;
        struct tree_node * after;
        before = NULL;
        after = NULL;

        while((z<root->num_keys) && (key>root->keys[z])){
            z+=1;
        }

        //CASE 2A: Internal node, key in current node (must be root)
        if((z<root->num_keys) && (root->keys[z] == key)){
            //printf("Match key in root %d, Deleting %d from root\n",root->keys[z], key);
            fflush(stdout);
            before = root->children[z];
            after = root->children[z+1];

            //try steal from left
            if(before->num_keys > before->min_keys || !before->leaf){

                int maxLeft = steal_max_from_left(before);

                //printf("Stole %d from left\n",maxLeft);
                fflush(stdout);

                //get value of stolen key
                struct search_result * sr3 = 
                    btsearch(maxLeft,btree->root,NULL);

                sr3->node_ptr->values[sr3->key] = root->values[z];
                root->values[z] = sr3->val;
            
                //replace key with min

                //printf("swapped maxleft %d and key %d",maxLeft,key);
                fflush(stdout);
                root->keys[z] = maxLeft;

                //replace min with key
                sr3->node_ptr->keys[sr3->key] = key;
                
                //delete key from leaf of min
                deletion_algorithm(key,sr3->node_ptr,btree);

                //rebalance
                int rebalancing = 1;
                while(rebalancing){
                    rebalancing=recursive_rebalance(btree->root, NULL, 0,btree);
                }
                free(sr3);
                
            //Else try steal from right  
            }else if(after->num_keys > after->min_keys || !after->leaf){

                    int minRight = steal_min_from_right(after);

                    //printf("Stole %d from right\n",minRight);
                    fflush(stdout);

                    //replace key with min
                    root->keys[z] = minRight;

                    //get value of stolen key
                    struct search_result * sr3 = btsearch(minRight,btree->root,NULL);
                    
                    //get data of minright
                    sr3->node_ptr->values[sr3->key] = root->values[z];
                    root->values[z] = sr3->val;

                    //printf("swapped minRight %d and key %d",minRight,key);
                    fflush(stdout);

                    sr3->node_ptr->keys[sr3->key] = key;
                    
                    deletion_algorithm(key,sr3->node_ptr,btree);

                    int rebalancing = 1;

                    while(rebalancing){
                        rebalancing=recursive_rebalance(btree->root, NULL, 0,btree);
                    }
                    free(sr3);
                    }
            else{
                //i guess we gotta merge

                //printf("Merging root???\n");
                fflush(stdout);
                merge_child_left(root, before,after,z,btree);
                deletion_algorithm(key,before,btree);
            }

        }else{
            //CASE 2B
            //key located in lower levels
            //find key's node
            //find maxleft child
            //swap the keys
            //call delete on leaf containing k
            //recursively rebalance

            //find k
            struct search_result * sr = btsearch(key,btree->root, NULL);
            if(!sr->success){
                free(sr);
                return 1;
            }

            //CASE 2Ba : LEAF DELETION
            if(sr->node_ptr->leaf){

                //printf("Deleting from leaf ndoe \n");
                fflush(stdout);
                deletion_algorithm(key, sr->node_ptr,btree );
                int rebalancing = 1;

                while(rebalancing){
                    //printf("Rebalancing\n");
                    fflush(stdout);
                    rebalancing = 
                    recursive_rebalance(btree->root, NULL, 0,btree);
                }

                free(sr);
                return 0;

            }else{

                //CASE 2Bb INTERNAL DELETION
                //cant be root, as this would have been picked up in 2A
                //we need to find the max value in the left neighbour child of k
                //printf("Internal deletion!\n");

                fflush(stdout);
                int root_parent = 0;
                
                struct tree_node * parent = sr->parent;

                if(sr->parent == btree->root){
                    root_parent = 1;
                }

                if(sr->parent == NULL){
                    perror("ERROR!!!");
                    return 1;
                }
                    
                //if child index is not first, we can steal from left
                int minmaxKey = 0;
                int keyindexofK = sr->key;

                minmaxKey = steal_max_from_left(sr->node_ptr->children[keyindexofK]);

                //printf("Stole max key %d from left child of internal\n",minmaxKey);

                fflush(stdout);

                //we need to find the node we stole from to put k here and get its val
                struct search_result * sr1 = 
                    btsearch(minmaxKey, root,NULL);

                if(!sr1->success){
                    perror("ERROR1\n");
                }

                struct info * maxVal = sr1->val;

                //printf("Replaced %d with %d\n",key,minmaxKey);

                fflush(stdout);

                //set k to maxLeft
                sr->node_ptr->keys[sr->key] = minmaxKey;
                sr->node_ptr->values[sr->key] = maxVal;

                //set maxleft to k
                sr1->node_ptr->keys[sr1->key] = key;
                sr1->node_ptr->values[sr1->key] = sr->val;

                //printf("Deleting k from its new leaf home\n");
                fflush(stdout);

                //now delete k from its new leaf node home
                deletion_algorithm(key, sr1->node_ptr,btree);

                //rebalance tree
                int rebalancing = 1;

                while(rebalancing){
                    //printf("Rebaolaning\n");
                    fflush(stdout);
                    rebalancing = recursive_rebalance(btree->root, NULL, 0,btree);
                }

                free(sr);
                free(sr1);
                return 0;
            }
        }
    }
}

/*############################################################################

    btree_delete()

    Checks for key in tree
    Calls deletion algorithm
    return 1 if fail, 0 if success
    
#############################################################################*/

int btree_delete(uint32_t key, void * helper) {
    
    struct btree* btree = helper;


    //printf("Deleting key %d",key);
    struct search_result * sr;
    
    sr = btsearch(key,btree->root,NULL);
    if(!sr->success){
        free(sr);
        return 1;
    }

    free(sr);

    fflush(stdout);

    struct tree_node * root = btree->root;

    pthread_mutex_lock(&btree->Mlock);
    deletion_algorithm(key, root,btree);
    pthread_mutex_unlock(&btree->Mlock);
  
    return 0;
}

/*############################################################################

    EXPORT FUNCTIONS
    
#############################################################################*/

/*############################################################################

    recursive_export()

    Performs preorder traversal on nodes and adds to a struct node * list
    Each node * contains the keys and number of keys within each node of the tree
    List is dynamically allocated and will require freeing
    returns a count of the number of nodes within the Btree

#############################################################################*/

uint64_t recursive_export(struct tree_node * node, struct node * list, 
    uint64_t *index, uint64_t * count){


    list[*index].keys = malloc(node->num_keys * sizeof(uint32_t));

    for(int i=0; i<node->num_keys;i++){
        list[*index].keys[i] = node->keys[i];
        fflush(stdout);
    }

    list[*index].num_keys = node->num_keys;

    *index +=1;
    *count +=1;

    //if children, preorder traverse over them
    if(!node->leaf){
        for(int i=0; i<node->num_keys+1;i++){
            recursive_export(node->children[i], list, index, count);
        }
    }
}

/*############################################################################

    Btree Export()

    Calls recursive export function on btree root
    returns a count of the number of nodes within the Btree
    
#############################################################################*/

uint64_t btree_export(void * helper, struct node ** list) {

    uint64_t index = 0;
    uint64_t count = 0;

    struct btree * tree = helper;
    struct tree_node *root = tree->root;

    //printf("Exporting %d nodes\n",tree->counter);
    fflush(stdout);

    *list =  malloc(tree->counter * sizeof(struct node));

    recursive_export(root, *list, &index, &count);
    
    if(count == 0){
        free(list);
        list = NULL;
    }

    return count;

}

/*############################################################################

    Encrypt/Decrypt Functions

#############################################################################*/

/*############################################################################

    thread_decrypt()

    Utilizes threads to decrypt blocks of data

    splits data into nthread blocks and 
    creates n threads to decrypt each section of blocks
    Takes args in form of decrypt_info struct.

#############################################################################*/

void * thread_decrypt(void * decrypt_info){

    struct thread_decrypt * td = decrypt_info;
    //printf("Decrypting in a thread!");
    // pthread_detach(pthread_self());

    decrypt_tea_ctr(td->cypher, td->key, td->nonce, td->plain ,td->num_blocks );
    free(td);
    return 0;

}

/*############################################################################

    btree_decrypt()

    decrypt function utilizing threads 

    splits data into nthread blocks and 
    creates n threads to decrypt each section of blocks
    Takes args in form of decrypt_info struct.
    Threads only used for 1kb + data

#############################################################################*/

// int btree_decrypt(uint32_t key, void * output, void * helper) {

//     struct search_result * sr;
//     struct btree * btree = helper;
//     pthread_mutex_lock(&btree->Mlock);

//     sr = btsearch(key,btree->root,NULL);
    
//     if(sr->success){

//         uint32_t keyIn[4];

//         for(int i=0; i<4; i++){
//             keyIn[i] = sr->val->key[i];
//         }

//         uint64_t nonceIn = sr->val->nonce;
//         double bytesIn = sr->val->size;

//         int blocks = ceil(bytesIn/8);
        // //printf("Decrypting %d blocks\n",blocks);
//             fflush(stdout);

//         if(blocks == 0){
//             blocks = 1;
//         }

//         uint8_t * decrypted = calloc(blocks*8, sizeof(uint8_t) );

//         if(bytesIn > 1000){
//             //divide blocks by threads
//             int nblocks = blocks / btree->threads;
//             //get remainder
//             int remainder = blocks % btree->threads;

//             pthread_t tid;
//             int blocks_written = 0;
//             for(int i=0; i<btree->threads;i++){
//                 //last thread finishes the job
//                 if(i+1 == btree->threads){
//                     nblocks = nblocks+remainder;
//                 }
//                 struct thread_decrypt * td = malloc(sizeof(struct thread_decrypt));
//                 uint64_t * dataIn = sr->val->data;
//                 td->cypher = dataIn[blocks_written];
//                 td->plain = decrypted[blocks_written/8];
//                 for(int i=0; i<4; i++){
//                     td->key[i] = sr->val->key[i];
//                 }
//                 td->num_blocks = nblocks;
//                 blocks_written +=nblocks;
//                 if(pthread_create(&tid,NULL,thread_decrypt,(void*)td)){
//                     free(td);
//                 }
//                 pthread_join(tid, NULL);

//                 pthread_exit(NULL);
//             }
//         }else{

//         decrypt_tea_ctr(sr->val->data, keyIn, nonceIn,decrypted  ,blocks );
//         }

//         uint8_t * outputptr = output;
//         for(int i=0; i<bytesIn;i++){
//             outputptr[i] = decrypted[i];
//         }

//         pthread_mutex_unlock(&(btree->Mlock));

//         free(decrypted);

//         free(sr);
            
//         // btree->lock=0;
//         return 0;

//         }else{
//             free(sr);
//             // btree->lock=0;
//             return 1;
//         }
//         // btree->lock=0;


// }

/*############################################################################

    btree_decrypt()

    Searches for the requested key in the tree and decrypts its data
    copying the plaintext to output*
    If successful, return 0, otherwise return 1.

#############################################################################*/

int btree_decrypt(uint32_t key, void * output, void * helper) {
    
    struct btree * btree = helper;


    struct search_result * sr;

    sr = btsearch(key,btree->root,NULL);
    
    if(sr->success){

        uint32_t keyIn[4];

        for(int i=0; i<4; i++){
            keyIn[i] = sr->val->key[i];
        }

        uint64_t nonceIn = sr->val->nonce;
        double bytesIn = sr->val->size;

        int blocks = ceil(bytesIn/8);

        fflush(stdout);

        if(blocks == 0){
            blocks = 1;
        }

        pthread_mutex_lock(&btree->Mlock);

        uint8_t * decrypted = calloc(blocks*8, sizeof(uint8_t) );


        decrypt_tea_ctr(sr->val->data, keyIn, nonceIn, (uint64_t *)decrypted  ,blocks );

        uint8_t * outputptr = output;
        for(int i=0; i<bytesIn;i++){
            outputptr[i] = decrypted[i];
        }
        pthread_mutex_unlock(&(btree->Mlock));

        free(decrypted);

        free(sr);
        return 0;

    }else{
        free(sr);
        return 1;
    }
}


/*############################################################################

    encrypt_tea()

    Encrypts 8 bytes of data with TEA encryption algorithm
    Outputs to provided cypher *

#############################################################################*/

void encrypt_tea(uint32_t plain[2], uint32_t cipher[2], uint32_t key[4]) {

    int sum = 0;
    uint32_t delta = 0x9E3779B9;
    cipher[0] = plain[0];
    cipher[1] = plain[1];

    for(int i=0; i<1024;i++){
        sum = (sum + delta) % 4294967296;
        uint64_t tmp1 = ((cipher[1] << 4) + key[0]) % 4294967296;
        uint64_t tmp2 = (cipher[1] + sum) % 4294967296;
        uint64_t tmp3 = ((cipher[1] >> 5) + key[1]) % 4294967296;
        cipher[0] = (cipher[0] + (tmp1 ^ tmp2 ^ tmp3)) % 4294967296;
        uint64_t tmp4 = ((cipher[0] << 4) + key[2]) % 4294967296;
        uint64_t tmp5 = (cipher[0] + sum) % 4294967296;
        uint64_t tmp6 = ((cipher[0] >> 5) + key[3]) % 4294967296;
        cipher[1] = (cipher[1] + (tmp4 ^ tmp5 ^ tmp6)) % 4294967296;
        }
        return;
}

/*############################################################################

    encrypt_tea_64()

    Modified encrypt_tea function
    Encrypts 8 bytes of data with TEA encryption algorithm
    Outputs to provided cypher *

#############################################################################*/

uint64_t encrypt_tea_64(uint64_t *plain,  uint32_t key[4]) {

    uint32_t cipher1[2];

    int sum = 0;
    cipher1[0] = (uint32_t)(*plain & 0xFFFFFFFFLL);
    cipher1[1] = (uint32_t)((*plain & 0xFFFFFFFF00000000LL) >> 32);

    for(int i=0; i<1024;i++){

        sum = (sum + 0x9E3779B9) % 4294967296;

        cipher1[0] = (cipher1[0] + ((((cipher1[1] << 4) + 
            key[0]) % 4294967296) ^ ((cipher1[1] + sum) % 4294967296) ^
            (((cipher1[1] >> 5) + key[1]) % 4294967296))) % 4294967296;

        cipher1[1] = (cipher1[1] + ((((cipher1[0] << 4) + 
            key[2]) % 4294967296) ^ ((cipher1[0] + sum) % 4294967296) ^
            (((cipher1[0] >> 5) + key[3]) % 4294967296))) % 4294967296;

    }

    return (uint64_t)cipher1[1] << 32 | cipher1[0];
}

/*############################################################################

    decrypt_tea()

    Decrypts 8 bytes of data with TEA encryption algorithm
    Outputs to provided plain *

#############################################################################*/

void decrypt_tea(uint32_t cipher[2], uint32_t plain[2], uint32_t key[4]) {

    uint32_t  sum = 0xDDE6E400;
    uint32_t  delta = 0x9E3779B9;

    for(int i = 0; i<1024; i++){
        uint64_t  tmp4 = ((cipher[0] << 4) + key[2]) % 4294967296;;
        uint64_t  tmp5 = (cipher[0] + sum) ;
        uint64_t  tmp6 = ((cipher[0] >> 5) + key[3]) % 4294967296;;
        cipher[1] = (cipher[1] - (tmp4 ^ tmp5 ^ tmp6)) % 4294967296;;
        uint64_t  tmp1 = ((cipher[1] << 4) + key[0]) % 4294967296;;
        uint64_t  tmp2 = (cipher[1] + sum) % 4294967296;;
        uint64_t  tmp3 = ((cipher[1] >> 5) + key[1]) % 4294967296;;
        cipher[0] = (cipher[0] - (tmp1 ^ tmp2 ^ tmp3)) % 4294967296;;
        sum = (sum - delta) % 4294967296;;

    }
        plain[0] = cipher[0];
        plain[1] = cipher[1];
    return;
}

/*############################################################################

    encrypt_tea_ctr()

    encrypts num_blocks * 8 bytes of data with TEA encryption algorithm
    Outputs to provided plain *

#############################################################################*/

void encrypt_tea_ctr(uint64_t * plain, uint32_t key[4], 
    uint64_t nonce, uint64_t * cipher, uint32_t num_blocks) {

    for (int i = 0; i< num_blocks; i++){
        uint64_t tmp1 = i ^ nonce;
        cipher[i] = plain[i] ^ encrypt_tea_64(&tmp1, key);
    }
    return;
}

/*############################################################################

    decrypt_tea_ctr()

    Decrypts num_blocks * 8 bytes of data with TEA encryption algorithm
    Outputs to provided plain *

#############################################################################*/

void decrypt_tea_ctr(uint64_t * cipher, uint32_t key[4], 
    uint64_t nonce, uint64_t * plain, uint32_t num_blocks) {

    for (int i = 0; i< num_blocks; i++){
        uint64_t tmp1 = i ^ nonce;
        plain[i] = cipher[i] ^ encrypt_tea_64(&tmp1, key);
    }

    return;
}