#ifndef __G_PQUEUE_H__
#define __G_PQUEUE_H__

G_BEGIN_DECLS

/**
 * GPQueue:
 *
 * An opaque structure representing a priority queue.
 **/
typedef struct _GPQueue GPQueue;

/**
 * GPQueueHandle:
 *
 * An opaque value representing one entry in a #GPQueue.
 *
 * DO NOT RELY on the fact that a #GPQueue and a #GPQueueHandle are
 * essentially the same thing at this time, this may change along with the
 * underlying implementation in future releases.
 **/
typedef GPQueue* GPQueueHandle;

GPQueue*	g_pqueue_insert	 (GPQueue *pqueue,
	 gpointer data,
	 gint priority,
	 GPQueueHandle *handle)
	 G_GNUC_WARN_UNUSED_RESULT;

gpointer	g_pqueue_top	 (GPQueue *pqueue);

gboolean	g_pqueue_top_extended	 (GPQueue *pqueue,
	 gpointer *data,
	 gint *priority);

GPQueue*	g_pqueue_delete_top	 (GPQueue *pqueue)
	 G_GNUC_WARN_UNUSED_RESULT;

gpointer	g_pqueue_pop	 (GPQueue **pqueue);

gboolean	g_pqueue_pop_extended	 (GPQueue **pqueue,
	 gpointer *data,
	 gint *priority);

GPQueue*	g_pqueue_delete	 (GPQueue* pqueue,
	 GPQueueHandle entry)
	 G_GNUC_WARN_UNUSED_RESULT;

GPQueue*	g_pqueue_change_priority	(GPQueue* pqueue,
	 GPQueueHandle entry,
	 gint priority)
	 G_GNUC_WARN_UNUSED_RESULT;

void	 g_pqueue_destroy	 (GPQueue* pqueue);

G_END_DECLS

#endif /* __G_PQUEUE_H__ */

