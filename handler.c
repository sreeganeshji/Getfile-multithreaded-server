#include "gfserver-student.h"
#include "gfserver.h"
#include "content.h"

#include "workload.h"

//
//  The purpose of this function is to handle a get request
//
//  The ctx is a pointer to the "context" operation and it contains connection state
//  The path is the path being retrieved
//  The arg allows the registration of context that is passed into this routine.
//  Note: you don't need to use arg. The test code uses it in some cases, but
//        not in others.
//
gfh_error_t gfs_handler(gfcontext_t **ctx, const char *path, void* arg){
    /*
     the queue node should contain the path and the context.
     
     */
    clientRequest *thisRequest;
    gfcontext_t* thisContext = *ctx;
    *ctx = NULL;
    thisRequest = malloc(sizeof(clientRequest));
    thisRequest->ctx = thisContext;
    thisRequest->path = (char*)path;
    pthread_mutex_lock(&queueMutex);
    steque_enqueue(clientRequestQueue, (void*)thisRequest);
    pthread_mutex_unlock(&queueMutex);
//    pthread_cond_broadcast(&requestEnqued);
    pthread_cond_signal(&requestEnqued);
	return gfh_success;
}

