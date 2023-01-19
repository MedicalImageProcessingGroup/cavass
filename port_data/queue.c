/*
  Copyright 1993-2013 Medical Image Processing Group
              Department of Radiology
            University of Pennsylvania

This file is part of CAVASS.

CAVASS is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

CAVASS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with CAVASS.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <assert.h>
 
#ifndef Enqueue

#define QUEUE_CHUNK 1024

#define ClearQueue \
{ \
	queue_exit = queue_entry = 0; \
	if (queue_buffer == NULL) \
	{	queue_buffer = (QueueItem *)malloc(QUEUE_CHUNK*sizeof(QueueItem)); \
		if (queue_buffer == NULL) \
			HandleQueueError; \
		queue_limit = QUEUE_CHUNK; \
	} \
}

#define QueueIsEmpty (queue_entry == 0)

#define Dequeue(item) \
{ \
	assert(!QueueIsEmpty); \
	(item) = queue_buffer[queue_exit]; \
	queue_exit++; \
	if (queue_exit == queue_entry) \
		ClearQueue \
	else if (queue_exit == queue_limit) \
		queue_exit = 0; \
}

#define Enqueue(item) \
{ \
	if (queue_entry == (queue_exit? queue_exit: queue_limit)) \
		EnlargeQueue; \
	if (queue_entry == queue_limit) \
		queue_entry = 0; \
	queue_buffer[queue_entry] = item; \
	queue_entry++; \
}

#define EnlargeQueue \
{ \
	QueueItem *t; \
	int j; \
\
	t = (QueueItem *)realloc(queue_buffer, \
		(queue_limit+QUEUE_CHUNK)*sizeof(QueueItem)); \
	if (t == NULL) \
		HandleQueueError; \
	queue_buffer = t; \
	if (queue_exit >= queue_entry) \
	{	for (j=queue_limit-1; j>=queue_exit; j--) \
			queue_buffer[j+QUEUE_CHUNK] = queue_buffer[j]; \
		queue_exit += QUEUE_CHUNK; \
	} \
	queue_limit += QUEUE_CHUNK; \
}

#endif

static QueueItem *queue_buffer;
static int queue_limit, queue_entry, queue_exit;
