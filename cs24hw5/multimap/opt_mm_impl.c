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
    multimap_node *root;
};


/*============================================================================
 * HELPER FUNCTION DECLARATIONS
 *
 *   Declarations of helper functions that are local to this module.  Again,
 *   these are not visible outside of this module.
 *============================================================================*/

multimap_node * alloc_mm_node();

multimap_node * find_mm_node(multimap_node *root, int key,
                             int create_if_not_found);

int * resize_array(int *values_arr, int new_size);

void free_multimap_node(multimap_node *node);

void alloc_slabs(void);

void resize_slabs(void);

/* Global variables for slab allocation. */

/* Array of slabs where each slab is an array of multimap_node structs. */
multimap_node **slabs = NULL;
unsigned int num_slabs;
unsigned int max_num_slabs;

/* 
 * These variables track the current state of each slab. We let each slab 
 * contain up to 1000000 multimap_nodes. num_nodes_in_slab corresponds to
 * how many multimap_nodes are in the slab, so we know when to reallocate
 * once num_nodes_in_slab == max_nodes_in_slab.
 */

static const unsigned int max_nodes_in_slab = 1000000;
unsigned int num_nodes_in_slab;
/*============================================================================
 * FUNCTION IMPLEMENTATIONS
 *============================================================================*/

/* 
 * Allocates the initial memory pool of slabs. Initially, there is only room
 * for one slab.
 */
void alloc_slabs() {
    slabs = malloc(sizeof(multimap_node *));
    slabs[0] = malloc(max_nodes_in_slab * sizeof(multimap_node));
    max_num_slabs = 1;
    num_slabs = 1;
    num_nodes_in_slab = 0;
}

/* 
 * Uses realloc to double the amount of slabs the slabs array can hold. 
 */
void resize_slabs(void) {
    max_num_slabs *= 2;
    slabs = realloc(slabs, max_num_slabs * sizeof(multimap_node *));
}

/* Allocates a multimap node, and zeros out its contents so that we know what
 * the initial value of everything will be.
 */
multimap_node * alloc_mm_node(void) {
    /* Check to see if we need to move on over to a new slab. */
    if (num_nodes_in_slab == max_nodes_in_slab) {
        /* Check to see if we need to resize our slabs array. */
        if (max_num_slabs == num_slabs) {
            resize_slabs();
        }
        /* Allocate a new slab. */
        slabs[num_slabs] = malloc(max_nodes_in_slab * sizeof(multimap_node));
        num_nodes_in_slab = 0;
        num_slabs++;
    }

    /* Allocate node into slab. */
    multimap_node *node = slabs[num_slabs - 1] + num_nodes_in_slab;
    num_nodes_in_slab++;
    bzero(node, sizeof(multimap_node));

    /* Initialize the values array to be of size 1 upon creation. */
    node->values = (int *) malloc(sizeof(int));
    node->size = 1;
    node->last_index = 0;

    return node;
}


/* This helper function searches for the multimap node that contains the
 * specified key.  If such a node doesn't exist, the function can initialize
 * a new node and add this into the structure, or it will simply return NULL.
 * The one exception is the root - if the root is NULL then the function will
 * return a new root node.
 */
multimap_node * find_mm_node(multimap_node *root, int key,
                             int create_if_not_found) {
    multimap_node *node;

    /* If the entire multimap is empty, the root will be NULL. */
    if (root == NULL) {
        if (create_if_not_found) {
            root = alloc_mm_node();
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
            if (node->left_child == NULL && create_if_not_found) {
                /* No left child, but caller wants us to create a new node. */
                multimap_node *new = alloc_mm_node();
                new->key = key;

                node->left_child = new;
            }
            node = node->left_child;
        }
        else {                   /* Follow right child */
            if (node->right_child == NULL && create_if_not_found) {
                /* No right child, but caller wants us to create a new node. */
                multimap_node *new = alloc_mm_node();
                new->key = key;

                node->right_child = new;
            }
            node = node->right_child;
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

}


/* Initialize a multimap data structure. */
multimap * init_multimap() {
    multimap *mm = malloc(sizeof(multimap));
    mm->root = NULL;
    alloc_slabs();
    return mm;
}


/* Release all dynamically allocated memory associated with the multimap
 * data structure.
 */
void clear_multimap(multimap *mm) {
    assert(mm != NULL);
    free_multimap_node(mm->root);
    mm->root = NULL;

    /* Free each slab. Implicitly frees each multimap_node. */
    for (unsigned int i = 0; i < num_slabs; i++) {
        free(slabs[i]);
    }

    /* Reset global variables. */
    free(slabs);
    slabs = NULL;
    num_slabs = 0;
    max_num_slabs = 0;
    num_nodes_in_slab = 0;
}


/* Adds the specified (key, value) pair to the multimap. */
void mm_add_value(multimap *mm, int key, int value) {
    multimap_node *node;

    assert(mm != NULL);

    /* Look up the node with the specified key.  Create if not found. */
    node = find_mm_node(mm->root, key, /* create */ 1);
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
        node->size *= 2;
        node->values = resize_array(node->values, node->size);
    }

    /* Add in the value to the array. */
    node->values[last_index] = value;
    node->last_index++;
}

/* 
 * Resizes values array for a multimap_node using realloc. Takes in the 
 * current array and an integer argument corresponding to the desired new 
 * size. Copies the current array into a new array of size new_size and 
 * returns the new, bigger array.
 */

int * resize_array(int *values_arr, int new_size) {
    int *new_values_arr = realloc(values_arr, sizeof(int) * new_size);
    return new_values_arr;
}

/* Returns nonzero if the multimap contains the specified key-value, zero
 * otherwise.
 */
int mm_contains_key(multimap *mm, int key) {
    return find_mm_node(mm->root, key, /* create */ 0) != NULL;
}


/* Returns nonzero if the multimap contains the specified (key, value) pair,
 * zero otherwise.
 */
int mm_contains_pair(multimap *mm, int key, int value) {
    multimap_node *node;
    int *curr;

    node = find_mm_node(mm->root, key, /* create */ 0);
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

