#include <glib.h>
#include "gpqueue.h"

struct _GPQueue {
    struct _GPQueue *next;
    struct _GPQueue *prev;
    struct _GPQueue *parent;
    struct _GPQueue *child;

    gpointer data;
    gint priority;
    gint degree;
    gboolean marked;
};

static inline void
g_pqueue_remove (GPQueue *src)
{
    src->prev->next = src->next;
    src->next->prev = src->prev;
    src->next = src;
    src->prev = src;
}

static inline void
g_pqueue_insert_before (GPQueue *dest, GPQueue *src)
{
    GPQueue *prev = dest->prev;
    dest->prev = src->prev;
    src->prev->next = dest;
    src->prev = prev;
    prev->next = src;
}

static inline void
g_pqueue_insert_after (GPQueue *dest, GPQueue *src)
{
    GPQueue *next = dest->next;
    dest->next = src;
    src->prev->next = next;
    next->prev = src->prev;
    src->prev = dest;
}

/**
 * g_pqueue_insert:
 * @pqueue: an existing GPQueue or %NULL to begin with an empty queue.
 * @data: a pointer to something associated with this queue entry.
 *   %NULL or the use of GINT_TO_POINTER() is acceptable. The same @data can
 *   be inserted into a GPQueue more than once, with different or identical
 *   priorities.
 * @priority: the priority for this entry. Entries are returned from the queue
 *   in <emphasis>ascending</emphasis> order of priority.
 * @handle: if not %NULL, a handle for the freshly inserted entry
 *   will be returned into this. This handle can be used in calls to
 *   g_pqueue_delete() and g_pqueue_change_priority(). Never make such calls
 *   for entries that have already been removed from the queue.
 *
 * Inserts a new entry into a #GPQueue.
 *
 * Return value: The altered priority queue.
 **/
GPQueue *
g_pqueue_insert (GPQueue *pqueue, gpointer data, gint priority, GPQueueHandle *handle)
{
    GPQueue *e = g_slice_new(GPQueue);

    e->next = e;
    e->prev = e;
    e->parent = NULL;
    e->child = NULL;
    e->data = data;
    e->priority = priority;
    e->degree = 0;
    e->marked = FALSE;

    if (handle != NULL) {
        *handle = e;
    }

    if (pqueue != NULL) {
        g_pqueue_insert_before(pqueue, e);
        if (e->priority < pqueue->priority) {
            return e;
        } else {
            return pqueue;
        }
    } else {
        return e;
    }
}

/**
 * g_pqueue_top:
 * @pqueue: a #GPQueue.
 *
 * Returns the topmost entry's data pointer.
 *
 * If you need to tell the difference between an empty queue and a queue
 * that happens to have a %NULL pointer at the top, use g_pqueue_top_extended()
 * or check if the queue is empty first.
 *
 * Return value: the topmost entry's data pointer.
 **/
gpointer
g_pqueue_top (GPQueue *pqueue)
{
    return (pqueue != NULL) ? (pqueue->data) : (NULL);
}

/**
 * g_pqueue_top_extended:
 * @pqueue: a #GPQueue.
 * @data: where to write the data pointer.
 * @priority: where to write the priority value.
 *
 * If the queue is empty, writes %NULL into *@data if @data is not %NULL
 * and returns %FALSE.
 *
 * If the queue is not empty, writes the topmost entry's data pointer
 * and priority value into *@data and *@priority unless they are %NULL
 * and returns %TRUE.
 *
 * Return value: %TRUE if the queue is not empty, %FALSE if it is.
 **/
gboolean
g_pqueue_top_extended (GPQueue *pqueue, gpointer *data, gint *priority)
{
    if (pqueue == NULL) {
        if (data != NULL) {
            *data = NULL;
        }
        return FALSE;
    } else {
        if (data != NULL) {
            *data = pqueue->data;
        }
        if (priority != NULL) {
            *priority = pqueue->priority;
        }
        return TRUE;
    }
}

static GPQueue *
g_pqueue_make_child (GPQueue *a, GPQueue *b)
{
    g_pqueue_remove(b);
    if (a->child != NULL) {
        g_pqueue_insert_before(a->child, b);
        a->degree += 1;
    } else {
        a->child = b;
        a->degree = 1;
    }
    b->parent = a;
    return a;
}

static inline GPQueue *
g_pqueue_join_trees (GPQueue *a, GPQueue *b)
{
    if (b->priority < a->priority) {
        return g_pqueue_make_child(b, a);
    }
    return g_pqueue_make_child(a, b);
}

static GPQueue *
g_pqueue_fix_rootlist (GPQueue *pqueue)
{
    /* We need to iterate over the circular list we are given and do
     * several things:
     * - Make sure all the elements are unmarked
     * - Make sure to return the element in the list with smallest
     *   priority value
     * - Find elements of identical degree and join them into trees
     * The last point is irrelevant for correctness, but essential
     * for performance. If we did not do this, our data structure would
     * degrade into an unsorted linked list.
     */

    const gsize degnode_size = (8 * sizeof(gpointer) + 1) * sizeof(gpointer);
    GPQueue **degnode = g_slice_alloc0(degnode_size);

    GPQueue sentinel;
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
    g_pqueue_insert_before(pqueue, &sentinel);

    GPQueue *current = pqueue;
    while (current != &sentinel) {
        current->marked = FALSE;
        current->parent = NULL;
        gint d = current->degree;
        if (degnode[d] == NULL) {
            degnode[d] = current;
            current = current->next;
        } else {
            if (degnode[d] != current) {
                current = g_pqueue_join_trees(degnode[d], current);
                degnode[d] = NULL;
            } else {
                current = current->next;
            }
        }
    }

    current = sentinel.next;
    GPQueue *minimum = current;
    while (current != &sentinel) {
        if (current->priority < minimum->priority) {
            minimum = current;
        }
        current = current->next;
    }

    g_pqueue_remove(&sentinel);

    g_slice_free1(degnode_size, degnode);

    return minimum;
}

static GPQueue *
g_pqueue_delete_root (GPQueue *pqueue, GPQueue *root)
{
    /* Step one:
     * If root has any children, pull them up to root level.
     * At this time, we only deal with their next/prev pointers,
     * further changes are made later in g_pqueue_fix_rootlist().
     */
    if (root->child) {
        g_pqueue_insert_after(root, root->child);
        root->child = NULL;
        root->degree = 0;
    }

    /* Step two:
     * Cut root out of the list.
     */
    if (root->next != root) {
        pqueue = root->next;
        g_pqueue_remove(root);
        /* Step three:
         * Clean up the remaining list.
         */
        pqueue = g_pqueue_fix_rootlist(pqueue);
    } else {
        pqueue = NULL;
    }

    g_slice_free(GPQueue, root);
    return pqueue;
}

/**
 * g_pqueue_delete_top:
 * @pqueue: a #GPQueue.
 *
 * Deletes the topmost entry from a #GPQueue and returns the altered queue.
 *
 * Return value: the altered #GPQueue.
 **/
GPQueue *
g_pqueue_delete_top (GPQueue *pqueue)
{
    if (pqueue == NULL) {
        return NULL;
    }
    return g_pqueue_delete_root(pqueue, pqueue);
}

/**
 * g_pqueue_pop:
 * @pqueue: a <emphasis>pointer</emphasis> to a #GPQueue.
 *
 * Deletes the topmost entry from a #GPQueue and returns its data pointer.
 * The altered queue is written back to *@pqueue.
 *
 * If you also need to know the elements priority value or be able to tell
 * the difference between an empty #GPQueue and one that happens to have an
 * entry with a %NULL data pointer at the top, use g_pqueue_pop_extended()
 * instead.
 *
 * Return value: the topmost entry's data pointer.
 **/
gpointer
g_pqueue_pop (GPQueue **pqueue)
{
    if (*pqueue == NULL) {
        return NULL;
    }
    gpointer data = (*pqueue)->data;
    *pqueue = g_pqueue_delete_root(*pqueue, *pqueue);
    return data;
}

/**
 * g_pqueue_pop_extended:
 * @pqueue: a <emphasis>pointer to a</emphasis> #GPQueue.
 * @data: unless this is %NULL, the topmost entry's data pointer or %NULL
 *   will be written here.
 * @priority: if this is not %NULL and the queue is not empty, the topmost
 *   entry's priority value will be written here.
 *
 * Deletes the topmost entry from a #GPQueue and returns (by reference) both
 * its data pointer and its priority value.
 * The altered queue is written back to *@pqueue.
 *
 * Return value: %TRUE if the queue was not empty, %FALSE if it was.
 **/
gboolean
g_pqueue_pop_extended (GPQueue **pqueue, gpointer *data, gint *priority)
{
    if (*pqueue == NULL) {
        if (data != NULL) {
            *data = NULL;
        }
        return FALSE;
    }

    if (data != NULL) {
        *data = (*pqueue)->data;
    }
    if (priority != NULL) {
        *priority = (*pqueue)->priority;
    }
    *pqueue = g_pqueue_delete_top(*pqueue);
    return TRUE;
}

static inline GPQueue *
g_pqueue_make_root (GPQueue *pqueue, GPQueue *entry)
{
    GPQueue *parent = entry->parent;
    entry->parent = NULL;
    entry->marked = FALSE;
    if (parent != NULL) {
        if (entry->next != entry) {
            if (parent->child == entry) {
                parent->child = entry->next;
            }
            g_pqueue_remove(entry);
            parent->degree -= 1;
        } else {
            parent->child = NULL;
            parent->degree = 0;
        }
        g_pqueue_insert_before(pqueue, entry);
    }
    if (entry->priority < pqueue->priority) {
        return entry;
    } else { return pqueue; }
}

static GPQueue *
g_pqueue_cut_tree (GPQueue *pqueue, GPQueue *entry)
{
    GPQueue *current = entry;
    while ((current != NULL) && (current->parent != NULL)) {
        GPQueue *parent = current->parent;
        pqueue = g_pqueue_make_root(pqueue, entry);
        if (parent->marked) {
            current = parent;
        } else {
            parent->marked = TRUE;
            current = NULL;
        }
    }
    if (entry->priority < pqueue->priority) {
        return entry;
    } else { return pqueue; }
}

/**
 * g_pqueue_delete:
 * @pqueue: a #GPQueue.
 * @entry: a #GPQueueHandle for an entry in @pqueue.
 *
 * Deletes one entry in a #GPQueue and returns the altered queue.
 *
 * Make sure that @entry refers to an entry that is actually part of
 * @pqueue at the time, otherwise the behavior of this function is
 * undefined (expect crashes).
 *
 * Return value: the altered #GPQueue.
 **/
GPQueue *
g_pqueue_delete (GPQueue *pqueue, GPQueueHandle entry)
{
    pqueue = g_pqueue_cut_tree(pqueue, entry);
    pqueue = g_pqueue_delete_root(pqueue, entry);
    return pqueue;
}

/**
 * g_pqueue_change_priority:
 * @pqueue: a #GPQueue.
 * @entry: a #GPQueueHandle for an entry in @pqueue.
 * @priority: the new priority value for @entry.
 *
 * Changes the priority of one entry in a #GPQueue
 * and returns the altered queue.
 *
 * Make sure that @entry refers to an entry that is actually part of
 * @pqueue at the time, otherwise the behavior of this function is
 * undefined (expect crashes).
 *
 * Return value: the altered #GPQueue.
 **/
GPQueue *
g_pqueue_change_priority (GPQueue *pqueue, GPQueueHandle entry, gint priority)
{
    if (entry->priority == priority) {
        return pqueue;
    }

    gint oldpriority = entry->priority;
    entry->priority = priority;

    pqueue = g_pqueue_cut_tree(pqueue, entry);

    if (priority > oldpriority) {
        if (entry->child) {
            g_pqueue_insert_after(entry, entry->child);
            entry->child = NULL;
            entry->degree = 0;
        }
        pqueue = g_pqueue_fix_rootlist(pqueue);
    }

    return pqueue;
}

/**
 * g_pqueue_destroy:
 * @pqueue: a #GPQueue.
 *
 * Deallocates the memory used by @pqueue itself, but not any memory pointed
 * to by the entries' data pointers.
 *
 * This is unnecessary for empty queues, which are just a %NULL pointer,
 * but can be used if you ever want to get rid of a #GPQueue before you have
 * removed all the entries.
 **/
void
g_pqueue_destroy (GPQueue *pqueue)
{
    if (pqueue == NULL) {
        return;
    }
    g_pqueue_destroy(pqueue->child);
    pqueue->prev->next = NULL;
    g_pqueue_destroy(pqueue->next);
    g_slice_free(GPQueue, pqueue);
}

#define __G_PQUEUE_C__
