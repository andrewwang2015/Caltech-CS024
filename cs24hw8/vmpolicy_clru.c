/*============================================================================
 * Implementation of the CLOCK/LRU page replacement policy.
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
 * This data structure represents a single page in our CLOCK/LRU policy.
 * We need it to be a doubly linked list for easy removal from the 
 * interior of the linked list. 
 */

typedef struct node_t {
    page_t page;
    struct node_t *prev;
    struct node_t *next; 
} node_t;



/*============================================================================
 * "Loaded Pages" Data Structure
 *
 * This data structure records all pages that are currently loaded in the
 * virtual memory, so that we can use our CLOCK/LRU policy.
 */

typedef struct loaded_pages_t {
    /* The maximum number of pages that can be resident in memory at once. */
    int max_resident;
    
    /* The number of pages that are currently loaded.  This can initially be
     * less than max_resident.
     */
    int num_loaded;

    /* 
     * Our queue will take from the head and add recently accessed pages to 
     * tail.
     */
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
    fprintf(stderr, "Using CLOCK/LRU eviction policy.\n\n");

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
        loaded->head->prev = NULL;
    } else {
        loaded->tail->next = new_node;
        new_node->prev = loaded->tail;
    }

    /* Set the tail to now be the new node. */
    loaded->tail = new_node;
    loaded->tail->next = NULL;    

    /* Increase the number of pages mapped. */
    loaded->num_loaded++;
    return; 
}


/* 
 * This function is called when the virtual memory system has a timer tick.
 * In the CLOCK/LRU policy, we traverse all resident pages in the queue.
 * If a page has been accessed since the last timer interrupt, its "accessed"
 * bit is cleared, and the page is moved to the back of the queue. Nothing
 * is done if the page has not been accessed. 
 */
void policy_timer_tick(void) {

    node_t *it = loaded->head;

    /* 
     * Keep track of the tail node before adjusting of our queue, so we
     * only loop through each node once. 
     */
    node_t *last = loaded->tail;

    /* 
     * Boolean flag that serves as the condition for our while loop. This is
     * to ensure that we only iterate through each page once. This flag
     * becomes false when our iterator reaches the original tail node. 
     * Without this and using our old approach of iteration until our 
     * iterator reaches NULL, we are at risk of scanning over pages more than
     * once because accessed pages are being added to the tail. 
     */
    int to_continue = 1;
    /* Traverse all pages in our queue. */
    while (to_continue) {
        /* 
         * Once we reach the original tail node, we will have iterated 
         * through all pages and so we want to make this iteration the last.
         */
        if (it == last) {
            to_continue = 0;
        }
        /* Check to see if it has been accessed since last interrupt. */
        if (is_page_accessed(it->page)) {
            /* Clear its accessed bit. */
            clear_page_accessed(it->page);

            /* 
             * Set its permission back to PAGEPERM_NONE so we can detect
             * later accesses.
             */
            set_page_permission(it->page, PAGEPERM_NONE);

            /* Move accessed page to end of queue. */
            node_t *after = it->next;
            node_t *prev = it->prev;
            if (prev != NULL && after != NULL) {

                /*
                 * When our iterator is sandwiched between two nodes, we
                 * simply have to move the prev and next pointers as to 
                 * "remove" the iterator node from the sandwich. We then 
                 * add the iterator onto the back of the queue (onto the
                 * tail). 
                 */
                prev->next = after;
                after->prev = prev;
                loaded->tail->next = it;
                it->prev = loaded->tail;
                it->next = NULL;
                loaded->tail = it;
                it = after;
            } else if (prev != NULL) {

                /*
                 * This means after == NULL in which case our iterator "it" 
                 * will be at the tail. Thus, because it is already at 
                 * the back of the queue, there is nothing to be done besides
                 * moving the iterator along.
                 */
                it = after;
            } else if (after != NULL) {

                /*
                 * This means prev == NULL in which case our iterator "it" 
                 * is at the head. In this case, we simply set the new head
                 * to be the node after and move the iterator node to the 
                 * back of the queue (onto the tail).
                 */
                after->prev = NULL;
                loaded->head = after;
                loaded->tail->next = it;
                it->prev = loaded->tail;
                it->next = NULL;
                loaded->tail = it;
                it = after;
            }

        } else {
            /* If page has not been accessed, move iterator node along. */
            it = it->next;
        }
    }

}


/* 
 * Choose a page to evict from the collection of mapped pages.  Then, record
 * that it is evicted.  This operates based on the CLOCK/LRU policy. When a
 * page must be chosen for eviction, it is taken from the front of the queue.
 * However, given the behavior of the timer interrupt, pages that have been
 * accessed "recently" will be towards the back of the queue, so the front
 * of the queue will [probably] be pages that haven't been accessed very
 * recently. 
 */
page_t choose_and_evict_victim_page(void) {
    /* Victim is the page at front of queue. */
    page_t victim = loaded->head->page;
    /* 
     * Shrink the collection of loaded pages now, by evicting the head 
     * page which is supposed to be the page that is [probably] a page
     * that has not been accessed recently. We do so by freeing the previous
     * head node and setting the new head node. 
     */
    node_t *temp = loaded->head; 
    loaded->head = loaded->head->next;
    loaded->head->prev = NULL;
    free(temp);
    loaded->num_loaded--;

#if VERBOSE
    fprintf(stderr, "Choosing victim page %u to evict.\n", victim);
#endif

    return victim;
}

