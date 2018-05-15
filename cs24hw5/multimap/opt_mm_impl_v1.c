#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "multimap.h"


/*============================================================================
 * TYPES
 *
 *   These types are defined in the implementation file so that they can
 *   be kept hidden to code outside this source file.  This is not for any
 *   security reason, but rather just so we can enforce that our testing
 *   programs are generic and don't have any access to implementation details.
 *============================================================================*/


/* Represents a key and its associated values in the multimap, as well as
 * pointers to the left and right child nodes in the multimap. */
typedef struct multimap_node {
    /* The key-value that this multimap node represents. */
    int key;

    /* An array of the values associated with this key in the multimap. */
    int *values;

    /* The current size of the array. Doubles once filled up. */
    unsigned int size;

    /* 
     * The next "free" index of the values array. This index is where the 
     * next new value will be added to.
     */
    unsigned int last_index;

    /* The index in the memory pool array that corresponds to this node. */
    unsigned int mem_pool_idx;

    /* The left child of the multimap node.  This will reference nodes that
     * hold keys that are strictly less than this node's key.
     */
    struct multimap_node *left_child;

    /* The right child of the multimap node.  This will reference nodes that
     * hold keys that are strictly greater than this node's key.
     */
    struct multimap_node *right_child;
} multimap_node;


/* The entry-point of the multimap data structure. */
struct multimap {
    /* Dynamically allocated array storing multimap_node structs. */
    multimap_node *mem_pool;

    /* Size of memory pool. Count of how many multimap_nodes it can hold. */
    unsigned int size;

    /* Index of mem_pool that the next multimap_node should be placed into. */
    unsigned int last_index;

    multimap_node *root;

};


/*============================================================================
 * HELPER FUNCTION DECLARATIONS
 *
 *   Declarations of helper functions that are local to this module.  Again,
 *   these are not visible outside of this module.
 *============================================================================*/

multimap_node * alloc_mm_node(multimap *mm);

multimap_node * find_mm_node(multimap_node *root, int key,
                             int create_if_not_found, multimap *mm);

int * resize_array(int *values_arr, int new_size);
multimap_node * resize_mem_pool(multimap_node *curr_pool, int new_size);
void free_multimap_node(multimap_node *node);


/*============================================================================
 * FUNCTION IMPLEMENTATIONS
 *============================================================================*/

/* Allocates a multimap node from the multimap memory pool argument passed
 * into the function. 
 */
multimap_node * alloc_mm_node(multimap *mm) {
    assert(mm != NULL);

    /* 
     * Reallocate more space in our multimap memory pool if necessary for 
     * allocation of new multimap_node. 
     */
    if (mm->last_index == mm->size) {
        mm->mem_pool = resize_mem_pool(mm->mem_pool, mm->size * 2);
        mm->size *= 2;
    }
    multimap_node *node = mm->mem_pool + mm->last_index;
    node->mem_pool_idx = mm->last_index;
    mm->last_index++;

    bzero(node, sizeof(multimap_node));

    /* Initialize the values array to be of size 1 upon creation. */
    node->values = (int *) malloc(sizeof(int));
    node->size = 1;
    node->last_index = 0;
    printf("%i, %i\n", mm->last_index, mm->size);
    return node;
}

int is_in_pool(multimap_node *node, multimap *mm) {
    return mm->mem_pool + node->mem_pool_idx <= 
        mm->mem_pool + mm->last_index; 
}

/* This helper function searches for the multimap node that contains the
 * specified key.  If such a node doesn't exist, the function can initialize
 * a new node and add this into the structure, or it will simply return NULL.
 * The one exception is the root - if the root is NULL then the function will
 * return a new root node.
 */
multimap_node * find_mm_node(multimap_node *root, int key,
                             int create_if_not_found, multimap *mm) {
    multimap_node *node;

    /* If the entire multimap is empty, the root will be NULL. */
    if (root == NULL) {
        if (create_if_not_found) {
            root = alloc_mm_node(mm);
            root->key = key;
        }
        return root;
    }

    /* Now we know the multimap has at least a root node, so start there. */
    node = root;
    while (1) {
        if (node->key == key)
            break;

        if (node->key > key) {   /* Follow left child */
            if (!is_in_pool(node->left_child, mm) && create_if_not_found) {
                /* No left child, but caller wants us to create a new node. */
                multimap_node *new = alloc_mm_node(mm);
                new->key = key;

                node->left_child = new;
                return new;
            }
            else if (!is_in_pool(node->left_child, mm)) {
                return NULL;
            }
            node = mm->mem_pool + node->left_child->mem_pool_idx;
        }
        else {                   /* Follow right child */
            if (!is_in_pool(node->right_child, mm) && create_if_not_found) {
                /* No right child, but caller wants us to create a new node. */
                multimap_node *new = alloc_mm_node(mm);
                new->key = key;

                node->right_child = new;
                return new;
            }
            else if (!is_in_pool(node->right_child, mm)) {
                return NULL;
            }
            node = mm->mem_pool + node->right_child->mem_pool_idx;
        }

        if (node == NULL)
            break;
    }

    return node;
}



/* This helper function frees a multimap node, including its children and
 * value-list.
 */
void free_multimap_node(multimap_node *node) {
    if (node == NULL)
        return;

    /* Free the children first. */
    free_multimap_node(node->left_child);
    free_multimap_node(node->right_child);

    /* Free the array of values. */
    free(node->values);

#ifdef DEBUG_ZERO
    /* Clear out what we are about to free, to expose issues quickly. */
    bzero(node, sizeof(multimap_node));
#endif
    free(node);
}


/* Initialize a multimap data structure. */
multimap * init_multimap() {
    multimap *mm = malloc(sizeof(multimap));
    /* Initially reserve enough space in memory pool for just the root. */
    mm->mem_pool = malloc(sizeof(multimap_node));
    mm->size = 1;
    mm->last_index = 0;
    mm->root = NULL;
    return mm;
}


/* Release all dynamically allocated memory associated with the multimap
 * data structure.
 */
void clear_multimap(multimap *mm) {
    assert(mm != NULL);
    free(mm->mem_pool);
    free_multimap_node(mm->root);
    mm->root = NULL;

}


/* Adds the specified (key, value) pair to the multimap. */
void mm_add_value(multimap *mm, int key, int value) {
    multimap_node *node;

    assert(mm != NULL);

    /* Look up the node with the specified key.  Create if not found. */
    node = find_mm_node(mm->root, key, /* create */ 1, mm);
    if (mm->root == NULL)
        mm->root = node;

    assert(node != NULL);
    assert(node->key == key);

    /* Add the new value to the multimap node's value array. */
    int last_index = node->last_index;
    int curr_size = node->size;

    /* 
     * Test to see if we need to resize the array. If we do need to resize, 
     * we simply double the size. 
     */
    if (last_index == curr_size) {
        node->values = resize_array(node->values, curr_size * 2);
        node->size = curr_size * 2;
    }

    /* Add in the value to the array. */
    node->values[last_index] = value;
    node->last_index++;
}

/* 
 * Resizes the memory pool array of a multimap. Takes in the current memory
 * pool array and an integer argument corresponding to the desired new 
 * size. Copies the current memory pool contents into the  new array of size 
 * new_size and  returns the new, bigger array. 
 */

multimap_node * resize_mem_pool(multimap_node *curr_pool, int new_size) {
    multimap_node *new_mem_pool = (multimap_node *)realloc(curr_pool, 
        sizeof(multimap_node) * new_size);
    return new_mem_pool;
}

/* 
 * Resizes values array for a multimap_node using realloc. Takes in the 
 * current array and an integer argument corresponding to the desired new 
 * size. Copies the current array into a new array of size new_size and 
 * returns the new, bigger array.
 */
int * resize_array(int *values_arr, int new_size) {
    int *new_values_arr = (int *)realloc(values_arr, sizeof(int) * new_size);
    return new_values_arr;
}

/* Returns nonzero if the multimap contains the specified key-value, zero
 * otherwise.
 */
int mm_contains_key(multimap *mm, int key) {
    return find_mm_node(mm->root, key, /* create */ 0, mm) != NULL;
}


/* Returns nonzero if the multimap contains the specified (key, value) pair,
 * zero otherwise.
 */
int mm_contains_pair(multimap *mm, int key, int value) {
    multimap_node *node;
    int *curr;

    node = find_mm_node(mm->root, key, /* create */ 0, mm);
    if (node == NULL)
        return 0;

    curr = node->values;

    /* Iterate through values array, searching for specific value. */
    for (unsigned int i = 0; i < node->last_index; i++) {
        if (curr[i] == value) {
            return 1;
        }
    }

    return 0;
}


/* This helper function is used by mm_traverse() to traverse every pair within
 * the multimap.
 */
void mm_traverse_helper(multimap_node *node, void (*f)(int key, int value)) {
    int *curr;

    if (node == NULL)
        return;

    mm_traverse_helper(node->left_child, f);

    curr = node->values;

    for (unsigned int i = 0; i < node->last_index; i++) {
        f(node->key, curr[i]);
    }

    mm_traverse_helper(node->right_child, f);
}


/* Performs an in-order traversal of the multimap, passing each (key, value)
 * pair to the specified function.
 */
void mm_traverse(multimap *mm, void (*f)(int key, int value)) {
    mm_traverse_helper(mm->root, f);
}

