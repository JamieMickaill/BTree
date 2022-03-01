/*############################################################################

    COMP2017 USYD 2021 SEMSTER 1
    
    ASSIGNMENT 5 : "BTREE"

    SID: 500611960

    TESTS.C

#############################################################################*/


#include "btreestore.h"



/*############################################################################

    TESTS

#############################################################################*/


/*############################################################################

    INIT TESTS

#############################################################################*/

void test_init(){
    void * helper = init_store(3, 4);
    if(helper == NULL){
        return 1;
    }
    close_store(helper);
}

void test_init_big(){
    void * helper = init_store(100000, 4);
    if(helper == NULL){
        return 1;
    }
    close_store(helper);
}

void test_init_small(){
    void * helper = init_store(1, 1);
    if(helper == NULL){
        return 0;
    }
    close_store(helper);  
}

void test_init_zero_b(){
    void * helper = init_store(0, 1);
    if(helper == NULL){
        return 0;
    }
    close_store(helper);   
}

void test_init_zero_thread(){
    void * helper = init_store(2, 0);
    if(helper == NULL){
        return 0;
    }
    close_store(helper);   
}

/*############################################################################

    Decrypt TESTS

#############################################################################*/

void test_decrypt_tea(){

    uint32_t key = 1;
    uint32_t key2 = 2;
    uint32_t plaintext[2];
    plaintext[0] = 0x3B9AC9FF;
    plaintext[1] = 0x3B9AC9FF;
    uint32_t cypher[2] = {0};
    size_t count = 64;
    uint32_t encryption_key[4];
    for(int i=0; i<4;i++){
        encryption_key[i] = i+100;
    }
    uint64_t nonce = 0x3B9AC9FF;

    printf("original data: ");
    for (int i=0; i<2;i++){
        printf("%x ",plaintext[i]);
    }

    encrypt_tea(plaintext, cypher, encryption_key);

    printf("Encrypted data:");
    for (int i=0; i<2;i++){
        printf("%x ",cypher[i]);
    }

    uint32_t plaintext2[2] = {0};
    
    printf("Decrypted data: ")    ;
    decrypt_tea(cypher, plaintext2, encryption_key);
        for (int i=0; i<2;i++){
        printf("%x ",plaintext2[i]);
    }

}

/*############################################################################

    Encrypt TESTS

#############################################################################*/

void test_encrypt_tea(){

    uint32_t key = 1;
    uint32_t key2 = 2;
    uint32_t plaintext[2];
    plaintext[0] = 0x3B9AC9FF;
    plaintext[1] = 0x3B9AC9FF;
    uint32_t cypher[2] = {0};
    size_t count = 64;
    uint32_t encryption_key[4];
    for(int i=0; i<4;i++){
        encryption_key[i] = i+100;
    }
    uint64_t nonce = 0x1111111111111111;
    encrypt_tea(plaintext, cypher, encryption_key);
    printf("Encrypted data:");
    for (int i=0; i<2;i++){
        printf("%x ",cypher[i]);
    }

}

/*############################################################################

    Retrieve TESTS

#############################################################################*/

void test_insert_retrieve(){
    void * helper = init_store(3, 4);

    uint32_t key = 1;
    uint32_t key2 = 2;
    uint32_t plaintext[2];
    plaintext[0] = 0x3B9AC9FF;
    plaintext[1] = 0x3B9AC9FF;
    size_t count = 8;
    uint32_t encryption_key[4];
    for(int i=0; i<4;i++){
        encryption_key[i] = i+100;
    }
    uint64_t nonce = 0x1111111111111111;

    int retrieved = 0;
    struct info * retrieved_val = malloc(sizeof(struct info));
    btree_insert(key, &plaintext, count, encryption_key, nonce, helper);
    retrieved = btree_retrieve(key, retrieved_val, helper);

    //success
    printf("Retrieved: ");
    uint8_t * data = retrieved_val->data;
    if(!retrieved){
        for(int j=0; j<retrieved_val->size; j++){
            printf("%x ",data[j]);
        }
    }

    free(retrieved_val);
    close_store(helper);
}

void test_insert_retrieve_non_existing(){
    void * helper = init_store(3, 4);

    uint32_t key = 1;
    uint32_t key2 = 2;
    uint32_t plaintext[2];
    plaintext[0] = 0x3B9AC9FF;
    plaintext[1] = 0x3B9AC9FF;
    size_t count = 8;
    uint32_t encryption_key[4];
    for(int i=0; i<4;i++){
        encryption_key[i] = i+100;
    }
    uint64_t nonce = 0x1111111111111111;

    int retrieved = 0;
    struct info * retrieved_val = malloc(sizeof(struct info));
    btree_insert(key2, plaintext, count, encryption_key, nonce, helper);
    retrieved = btree_retrieve(key, retrieved_val, helper);

    //success
    if(!retrieved){
        printf("Key shouldn't exist");
    }else{
        printf("Key not found -> success");
    }
    free(retrieved_val);
    close_store(helper);
}

/*############################################################################

    Insert TESTS

#############################################################################*/

void test_insert(){
    void * helper = init_store(3, 4);

    uint32_t key = 1;
    uint32_t key2 = 2;
    uint32_t plaintext[2];
    plaintext[0] = 0x3B9AC9FF;
    plaintext[1] = 0x3B9AC9FF;
    size_t count = 8;
    uint32_t encryption_key[4];
    for(int i=0; i<4;i++){
        encryption_key[i] = i+100;
    }
    uint64_t nonce = 0x1111111111111111;


    btree_insert(key, plaintext, count, encryption_key, nonce, helper);
    btree_insert(key2, plaintext, count, encryption_key, nonce, helper);
    close_store(helper);
}

void test_insert_many(){
    void * helper = init_store(3, 4);
    
    uint32_t plaintext[2];
    plaintext[0] = 0x3B9AC9FF;
    plaintext[1] = 0x3B9AC9FF;
    size_t count = 8;
    uint32_t encryption_key[4];
    for(int i=0; i<4;i++){
        encryption_key[i] = i+100;
    }
    uint64_t nonce = 0x1111111111111111;


    for(int i=0; i<100; i++){
    btree_insert(i, plaintext, count, encryption_key, nonce, helper);
    }
    close_store(helper);
}

void test_insert_many_big_bf(){
    void * helper = init_store(10, 4);
    
    uint32_t plaintext[2];
    plaintext[0] = 0x3B9AC9FF;
    plaintext[1] = 0x3B9AC9FF;
    size_t count = 8;
    uint32_t encryption_key[4];
    for(int i=0; i<4;i++){
        encryption_key[i] = i+100;
    }
    uint64_t nonce = 0x1111111111111111;


    for(int i=0; i<100; i++){
    btree_insert(i, plaintext, count, encryption_key, nonce, helper);
    }
    close_store(helper);
}

void test_insert_many_smol_bf(){
    void * helper = init_store(2, 4);
    
    uint32_t plaintext[2];
    plaintext[0] = 0x3B9AC9FF;
    plaintext[1] = 0x3B9AC9FF;
    size_t count = 8;
    uint32_t encryption_key[4];
    for(int i=0; i<4;i++){
        encryption_key[i] = i+100;
    }
    uint64_t nonce = 0x1111111111111111;


    for(int i=0; i<100; i++){
    btree_insert(i, plaintext, count, encryption_key, nonce, helper);
    }
    close_store(helper);
}

/*############################################################################

    Insert Delete TESTS

#############################################################################*/

void test_insert_delete_single(){
    void * helper = init_store(2, 4);
    
    uint32_t plaintext[2];
    plaintext[0] = 0x3B9AC9FF;
    plaintext[1] = 0x3B9AC9FF;
    size_t count = 8;
    uint32_t encryption_key[4];
    for(int i=0; i<4;i++){
        encryption_key[i] = i+100;
    }
    uint64_t nonce = 0x1111111111111111;


    for(int i=0; i<1; i++){
    btree_insert(i, plaintext, count, encryption_key, nonce, helper);
    }

    for(int i=0; i<1; i++){
    btree_delete(i,helper);
    }
    close_store(helper);
}

//uncomment when delete is fixed
void test_insert_delete_multi(){
    void * helper = init_store(5, 4);
    
    uint32_t plaintext[2];
    plaintext[0] = 0x3B9AC9FF;
    plaintext[1] = 0x3B9AC9FF;
    size_t count = 8;
    uint32_t encryption_key[4];
    for(int i=0; i<4;i++){
        encryption_key[i] = i+100;
    }
    uint64_t nonce = 0x1111111111111111;


    for(int i=0; i<500; i++){
    btree_insert(i, plaintext, count, encryption_key, nonce, helper);
    struct node * list = NULL;

    }

    for(int j=0; j<500; j++){
        btree_delete(j,helper);

        // printf("DELETE %d\n",j);
    }
        // struct node * list = NULL;

            close_store(helper);
        
        }
        
    


void test_insert_delete_many_smol_bf(){
    // void * helper = init_store(3, 4);
    
    // uint32_t plaintext[2];
    // plaintext[0] = 0x3B9AC9FF;
    // plaintext[1] = 0x3B9AC9FF;
    // size_t count = 8;
    // uint32_t encryption_key[4];
    // for(int i=0; i<4;i++){
    //     encryption_key[i] = i+100;
    // }
    // uint64_t nonce = 0x1111111111111111;


    // for(int i=5; i>0; i--){
    // btree_insert(i, plaintext, count, encryption_key, nonce, helper);
    // }

    // for(int i=5; i>0; i--){
    // btree_delete(i,helper);
    // }
    // close_store(helper);
}

void test_insert_delete_many_big_bf(){
//     void * helper = init_store(10, 4);
    
//     uint32_t plaintext[2];
//     plaintext[0] = 0x3B9AC9FF;
//     plaintext[1] = 0x3B9AC9FF;
//     size_t count = 8;
//     uint32_t encryption_key[4];
//     for(int i=0; i<4;i++){
//         encryption_key[i] = i+100;
//     }
//     uint64_t nonce = 0x1111111111111111;


//     for(int i=0; i<50; i++){
//     btree_insert(i, plaintext, count, encryption_key, nonce, helper);
//     }

//     for(int i=0; i<50; i++){
//     btree_delete(i,helper);
//     }
//     close_store(helper);
}

void test_insert_duplicate_error(){
    // void * helper = init_store(100, 4);
    
    // uint32_t plaintext[2];
    // plaintext[0] = 0x3B9AC9FF;
    // plaintext[1] = 0x3B9AC9FF;
    // size_t count = 8;
    // uint32_t encryption_key[4];
    // for(int i=0; i<4;i++){
    //     encryption_key[i] = i+100;
    // }
    // uint64_t nonce = 0x1111111111111111;


    // for(int i=0; i<10; i++){
    // btree_insert(1, plaintext, count, encryption_key, nonce, helper);
    // }

    // for(int i=0; i<10; i++){
    // btree_delete(9,helper);
    // }
    // close_store(helper);
}

void test_insert_delete_nonexistingkey_error(){
    void * helper = init_store(100, 4);
    
    uint32_t plaintext[2];
    plaintext[0] = 0x3B9AC9FF;
    plaintext[1] = 0x3B9AC9FF;
    size_t count = 8;
    uint32_t encryption_key[4];
    for(int i=0; i<4;i++){
        encryption_key[i] = i+100;
    }
    uint64_t nonce = 0x1111111111111111;


    for(int i=0; i<1; i++){
    btree_insert(99, plaintext, count, encryption_key, nonce, helper);
    }

    for(int i=0; i<1; i++){
    btree_delete(95,helper);
    }
    close_store(helper);
}

/*############################################################################

    Insert Decrypt TESTS

#############################################################################*/

void test_insert_decrypt_single(){
    void * helper = init_store(4, 4);
    
    uint32_t plaintext[2];
    plaintext[0] = 0x3B9AC9FF;
    plaintext[1] = 0x3B9AC9FF;
    size_t count = 8;
    uint32_t encryption_key[4];
    for(int i=0; i<4;i++){
        encryption_key[i] = i+100;
    }
    uint64_t nonce = 0x1111111111111111;


    for(int i=0; i<1; i++){
    btree_insert(i, plaintext, count, encryption_key, nonce, helper);
    }

    uint64_t*  decrypted = malloc((count/8) * sizeof(uint64_t));
    for(int i=0; i<1; i++){
    btree_decrypt(i,decrypted, helper);
    }
    free(decrypted);
    close_store(helper);
}

void test_insert_decrypt_multi(){
    void * helper = init_store(4, 4);
    
    uint32_t plaintext[2];
    plaintext[0] = 0x3B9AC9FF;
    plaintext[1] = 0x3B9AC9FF;
    size_t count = 8;
    uint32_t encryption_key[4];
    for(int i=0; i<4;i++){
        encryption_key[i] = i+100;
    }
    uint64_t nonce = 0x1111111111111111;


    for(int i=0; i<10; i++){
    btree_insert(i, plaintext, count, encryption_key, nonce, helper);
    }

    uint64_t *decrypted  = malloc((count/8) * sizeof(uint64_t));

    for(int i=0; i<10; i++){
        btree_decrypt(i,decrypted,helper);
        uint8_t * decrypteddata = decrypted;
        for(int j = 0; j< count;j++){
            printf("%x\n",decrypteddata[j]);
        }
    }
    free(decrypted);

    close_store(helper);
}

void test_insert_decrypt_single_padding(){
    void * helper = init_store(2, 4);
    
    uint32_t plaintext[3];
    plaintext[0] = 0x3B9AC9FF;
    plaintext[1] = 0x3B9AC9FF;
    plaintext[2] = 0x3B9AC9FF;
    double count = 12;
    uint32_t encryption_key[4];
    for(int i=0; i<4;i++){
        encryption_key[i] = i+100;
    }
    uint64_t nonce = 0x1111111111111111;


    for(int i=0; i<1; i++){
    btree_insert(i, plaintext, count, encryption_key, nonce, helper);
    }

    uint64_t *decrypted  = malloc((count/8) * sizeof(uint64_t));

    for(int i=0; i<10; i++){
        btree_decrypt(i,decrypted,helper);
        uint8_t * decrypteddata = decrypted;
        for(int j = 0; j< count;j++){
            printf("%x\n",decrypteddata[j]);
        }
    }
    free(decrypted);
    close_store(helper);
}

void test_insert_decrypt_multi_padding(){
    void * helper = init_store(2, 4);
    
    uint32_t plaintext[3];
    plaintext[0] = 0x3B9AC9FF;
    plaintext[1] = 0x3B9AC9FF;
    plaintext[2] = 0x3B9AC9FF;
    double count = 12;
    uint32_t encryption_key[4];
    for(int i=0; i<4;i++){
        encryption_key[i] = i+100;
    }
    uint64_t nonce = 0x1111111111111111;


    //insert keys
    for(int i=0; i<10; i++){
    btree_insert(i, plaintext, count, encryption_key, nonce, helper);
    }


    uint64_t *decrypted  = malloc((count/8) * sizeof(uint64_t));

    for(int i=0; i<10; i++){
        btree_decrypt(i,decrypted,helper);
        uint8_t * decrypteddata = decrypted;
        for(int j = 0; j< count;j++){
            printf("%x\n",decrypteddata[j]);
        }
    }
    free(decrypted);
    close_store(helper);
}

/*############################################################################

    Export TESTS

#############################################################################*/

void test_insert_export_multi(){
    void * helper = init_store(4, 4);
    
    uint32_t plaintext[2];
    plaintext[0] = 0x3B9AC9FF;
    plaintext[1] = 0x3B9AC9FF;
    size_t count = 8;
    uint32_t encryption_key[4];
    for(int i=0; i<4;i++){
        encryption_key[i] = i+100;
    }
    uint64_t nonce = 0x1111111111111111;


    for(int i=0; i<10; i++){
    btree_insert(i, plaintext, count, encryption_key, nonce, helper);
    }

    struct node * list = NULL;

    btree_export(helper,&list);
    struct btree * btree = helper;
    //print out exported keys
    printf("Keys: ");
    for(int i=0; i<btree->counter; i++){
        printf(" Node %d:",i);
        for(int j=0;j<list[i].num_keys;j++){
        printf("%d ",list[i].keys[j]);
        }
    }

    for(int i=0; i<btree->counter;i++){
        free(list[i].keys);
        
    }
    free(list);
    close_store(helper);
}

//uncomment when delete fixed
void test_insert_delete_export_multi(){
//     void * helper = init_store(4, 4);
    
//     uint32_t plaintext[2];
//     plaintext[0] = 0x3B9AC9FF;
//     plaintext[1] = 0x3B9AC9FF;
//     size_t count = 8;
//     uint32_t encryption_key[4];
//     for(int i=0; i<4;i++){
//         encryption_key[i] = i+100;
//     }
//     uint64_t nonce = 0x1111111111111111;


//     for(int i=0; i<10; i++){
//     btree_insert(i, plaintext, count, encryption_key, nonce, helper);
//     }

//     struct node * list = NULL;

//     btree_export(helper,&list);
// struct btree * btree = helper;
//     //print out exported keys
//     printf("Keys: ");
//     for(int i=0; i<btree->counter; i++){
//         for(int j=0;j<list[i].num_keys;j++){
//         printf("%d ",list[i].keys[j]);
//         }
//     }

//     for(int i=0; i<btree->counter;i++){
//         free(list[i].keys);
        
//     }
//     free(list);


//     for(int i=0; i<5; i++){
//         btree_delete(i,helper);
//     }

//     struct tree_node * list2 = NULL;
//     btree_export(&list2,helper);

   

//     //print out exported keys
//     printf("Keys: ");
//     for(int i=0; i<btree->counter; i++){
//         for(int j=0;j<list2[i].num_keys;j++){
//         printf("%d ",list2[i].keys[j]);
//         }
//     }

//     for(int i=0; i<btree->counter;i++){
//         free(list2[i].keys);
        
//     }
//     free(list2);


//     close_store(helper);
}
/*############################################################################
 
    MAIN

    Takes argv integer input and runs associated test from function
    pointer array.

#############################################################################*/

int main(int argc, char**argv) {

    void (*tests[25])();

    tests[0] = &test_init;
    tests[1] = &test_init_big;
    tests[2] = &test_init_small;
    tests[3] =  &test_init_zero_b;
    tests[4] = &test_init_zero_thread;
    tests[5] =  &test_decrypt_tea;
    tests[6] =  &test_encrypt_tea;
    tests[7] =  &test_insert_retrieve;
    tests[8] =  &test_insert_retrieve_non_existing;
    tests[9] =  &test_insert;
    tests[10] =  &test_insert_many;
    tests[11] =  &test_insert_many_big_bf;
    tests[12] =  &test_insert_many_smol_bf;
    tests[13] =  &test_insert_delete_single;
    tests[14] =  &test_insert_delete_multi;
    tests[15] =  &test_insert_delete_many_smol_bf;
    tests[16] =  &test_insert_delete_many_big_bf;
    tests[17] =  &test_insert_duplicate_error;
    tests[18] =  &test_insert_delete_nonexistingkey_error;
    tests[19] =  &test_insert_decrypt_single;
    tests[20] =  &test_insert_decrypt_multi;
    tests[21] = &test_insert_decrypt_single_padding;
    tests[22] = &test_insert_decrypt_multi_padding;
    tests[23] =  &test_insert_export_multi;
    tests[24] =  &test_insert_delete_export_multi;

    int test_number = atoi(argv[1]);

    (*tests[test_number])();
    
    return 0;
}

