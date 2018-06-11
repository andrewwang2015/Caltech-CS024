/*============================================================================
 * Implementation of the FIFO page replacement policy.
 *
 * We don't mind if paging policies use malloc() and free(), just because it
 * keeps things simpler.  In real life, the pager would use the kernel memory
 * allocator to manage data structures, and it would endeavor to allocate and
 * release as little memory as possible to avoid making the kernel slow and
 * prone to memory fragmentation issues.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "vmpolicy.h"

/*============================================================================
 * "Node" Data Structure
 *
 * This data structure represents a single page in the FIFO queue.
 */

typedef struct node_t {
    page_t page;
    struct node_t *next; 
} node_t;


/*============================================================================
 * "Loaded Pages" Data Structure
 *
 * This data structure records all pages that are currently loaded in the
 * virtual memory, so that we can use our FIFO policy.
 */

typedef struct loaded_pages_t {
    /* The maximum number of pages that can be resident in memory at once. */
    int max_resident;
    
    /* The number of pages that are currently loaded.  This can initially be
     * less than max_resident.
     */
    int num_loaded;

    /* Our queue will take from the head and add to the tail. */
    node_t *head;
    node_t *tail;

} loaded_pages_t;



/*============================================================================
 * Policy Implementation
 */

/* The queue of pages that are currently resident. */
static loaded_pages_t *loaded;


/* Initialize the policy.  Return nonzero for success, 0 for failure. */
int policy_init(int max_resident) {
    fprintf(stderr, "Using FIFO eviction policy.\n\n");

    loaded = malloc(sizeof(loaded_pages_t));
    if (loaded == NULL) {
        fprintf(stderr, "Fail to allocate loaded_pages_t \n");
        return 0;
    }
    /* Initialize head and tail of our FIFO queue. */
    loaded->head = NULL;
    loaded->tail = NULL;

    /* Initialize pages counts (max and current). */
    loaded->num_loaded = 0;
    loaded->max_resident = max_resident;

    /* Return nonzero if initialization succeeded. */
    return 1;
}


/* Clean up the data used by the page replacement policy. */
void policy_cleanup(void) {
    /* Iterate through our queue and free each node. */
    node_t *temp = loaded->head;
    node_t *next;
    while (temp != NULL) {
        next = temp->next;
        free(temp);
        temp = next;
    }
    loaded->head = NULL;
    loaded->tail = NULL;
    free(loaded);
    loaded = NULL;
}


/* This function is called when the virtual memory system maps a page into the
 * virtual address space.  Record that the page is now resident.
 */
void policy_page_mapped(page_t page) {
    assert(loaded->num_loaded < loaded->max_resident);
    /* Initialize a new node for new page. */
    node_t *new_node = malloc(sizeof(new_node));
    if (new_node == NULL) {
        fprintf(stderr, "Fail to allocate new node for new page \n");
        exit(1);
    }
    new_node->page = page; 
    new_node->next = NULL;

    /* Handle the case when the queue is empty. */
    if (loaded->head == NULL && loaded->tail == NULL) {
        loaded->head = new_node;
    } else {
        loaded->tail->next = new_node;
    }

    /* Set the tail to now be the new node. */
    loaded->tail = new_node;

    /* Increase the number of pages mapped. */
    loaded->num_loaded++;


    return; 
}


/* This function is called when the virtual memory system has a timer tick. */
void policy_timer_tick(void) {
    /* Do nothing! */
}


/* Choose a page to evict from the collection of mapped pages.  Then, record
 * that it is evicted.  This operates based on the FIFO-replacement policy.
 * All resident pages are recorded in a FIFO queue. When a new queue is 
 * loaded (mapped), it is added to the back of the queue. When a page 
 * must be evicted, the victim is the page at the front of the queue. 
 */
page_t choose_and_evict_victim_page(void) {
    /* Victim is the page at front of queue. */
    page_t victim = loaded->head->page;

    /* Free the previous head node. Set the new head node. */
    node_t *temp = loaded->head; 
    loaded->head = loaded->head->next;
    free(temp);
    temp = NULL;

    /* Decrease number of currently loaded pages. */
    loaded->num_loaded--;

#if VERBOSE
    fprintf(stderr, "Choosing victim page %u to evict.\n", victim);
#endif

    return victim;
}

