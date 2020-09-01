/*
 *  This file is for use by students to define anything they wish.  It is used by the gf server implementation
 */
#ifndef __GF_SERVER_STUDENT_H__
#define __GF_SERVER_STUDENT_H__

#include "gf-student.h"
#include "gfserver.h"
#include "content.h"

typedef struct clientRequest{
    gfcontext_t * ctx;
    char* path;
}clientRequest;

steque_t *clientRequestQueue;

void init_threads(size_t numthreads);
void cleanup_threads();
int requestsInQueue;
extern pthread_cond_t requestEnqued;
extern pthread_mutex_t queueMutex;

#endif // __GF_SERVER_STUDENT_H__
