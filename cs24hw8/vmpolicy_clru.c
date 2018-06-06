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
 * Policy Implementation
 */

/* Our queue will take from the head and add to the tail. */
node_t *head;
node_t *tail;

/* Initialize the policy.  Return nonzero for success, 0 for failure. */
int policy_init(int max_resident) {
    (void)(max_resident);
    fprintf(stderr, "Using CLOCK/LRU eviction policy.\n\n");

    /* Initialize head and tail of our queue used for LRU policy. */
    head = NULL;
    tail = NULL;

    /* Return nonzero if initialization succeeded. */
    return 1;
}


/* Clean up the data used by the page replacement policy. */
void policy_cleanup(void) {
    /* Iterate through our queue and free each node. */
    node_t *temp = head;
    node_t *next;
    while (temp != NULL) {
        next = temp->next;
        free(temp);
        temp = next;
    }
    head = NULL;
    tail = NULL;
}


/* This function is called when the virtual memory system maps a page into the
 * virtual address space.  Record that the page is now resident.
 */
void policy_page_mapped(page_t page) {
    /* Initialize a new node for new page. */
    node_t *new_node = malloc(sizeof(new_node));
    new_node->page = page; 
    new_node->next = NULL;

    /* Handle the case when the queue is empty. */
    if (head == NULL && tail == NULL) {
        head = new_node;
        head->prev = NULL;
    } else {
        tail->next = new_node;
        new_node->prev = tail;
    }

    /* Set the tail to now be the new node. */
    tail = new_node;
    tail->next = NULL;
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
    /* Traverse all pages in our queue. */
    node_t *it = head;
    while (it != NULL) {
        /* Check to see if it has been accessed since last interrupt. */
        if (is_page_accessed(it->page)) {
            /* Clear its accessed bit and move to back of queue. */
            clear_page_accessed(it->page);

            /* 
             * Set its permission back to PAGEPERM_NONE so we can detect
             * later accesses.
             */
            set_page_permission(it->page, PAGEPERM_NONE);

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
                tail->next = it;
                it->prev = tail;
                it->next = NULL;
                tail = it;
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
                head = after;
                tail->next = it;
                it->prev = tail;
                it->next = NULL;
                tail = it;
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
    page_t victim = head->page;

    /* Free the previous head node. Set the new head node. */
    node_t *temp = head; 
    head = head->next;
    head->prev = NULL;
    free(temp);

#if VERBOSE
    fprintf(stderr, "Choosing victim page %u to evict.\n", victim);
#endif

    return victim;
}

