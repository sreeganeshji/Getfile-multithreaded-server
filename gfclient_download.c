#include "gfclient-student.h"

#define MAX_THREADS (32767)

#define USAGE                                                                 \
"usage:\n"                                                                    \
"  webclient [options]\n"                                                     \
"options:\n"                                                                  \
"  -h                  Show this help message\n"                              \
"  -r [num_requests]   Request download total (Default: 5)\n"           \
"  -p [server_port]    Server port (Default: 19121)\n"                         \
"  -s [server_addr]    Server address (Default: 127.0.0.1)\n"                   \
"  -t [nthreads]       Number of threads (Default 7)\n"                       \
"  -w [workload_path]  Path to workload file (Default: workload.txt)\n"       \

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
  {"help",          no_argument,            NULL,           'h'},
  {"nthreads",      required_argument,      NULL,           't'},
  {"nrequests",     required_argument,      NULL,           'r'},
  {"server",        required_argument,      NULL,           's'},
  {"port",          required_argument,      NULL,           'p'},
  {"workload-path", required_argument,      NULL,           'w'},
  {NULL,            0,                      NULL,             0}
};

static void Usage() {
	fprintf(stderr, "%s", USAGE);
}

static void localPath(char *req_path, char *local_path){
  static int counter = 0;

  sprintf(local_path, "%s-%06d", &req_path[1], counter++);
}

static FILE* openFile(char *path){
  char *cur, *prev;
  FILE *ans;

  /* Make the directory if it isn't there */
  prev = path;
  while(NULL != (cur = strchr(prev+1, '/'))){
    *cur = '\0';

    if (0 > mkdir(&path[0], S_IRWXU)){
      if (errno != EEXIST){
        perror("Unable to create directory");
        exit(EXIT_FAILURE);
      }
    }

    *cur = '/';
    prev = cur;
  }

  if( NULL == (ans = fopen(&path[0], "w"))){
    perror("Unable to open file");
    exit(EXIT_FAILURE);
  }

  return ans;
}

/* Callbacks ========================================================= */
static void writecb(void* data, size_t data_len, void *arg){
  FILE *file = (FILE*) arg;

  fwrite(data, 1, data_len, file);
}


int requestCount = 0;
pthread_mutex_t countMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t requestAdded = PTHREAD_COND_INITIALIZER;
int nrequests = 5;
 steque_t* requestQueue;
char *server = "localhost";
unsigned short port = 19121;

/* Note that when you have a worker thread pool, you will need to move this logic into the worker threads */

/*
 differnt threads will be at different stages of execution. htere should be a way to tell the workers that once all the requests have been satisfied, they don't need to wait for a new request.
 one of the concers is that if when this information is broadcasetd, some thread was executing hte receive operation and then when it is done, will go to the wait state at whcih time no one will be able to let it know.
 hence there needs to be a check on the number of requests completed before waiting for hte next element in the queue.
 another concern is that when this thread is checking the value of the number of requests, another thread is processing that last request and has not incremented the number of theads yet. hence the first thread will assume there are more requests to come and will be waiting. the second thread should broadcast after it is done processing or even just incrementing the count.
 */

void * clientWork(void *j){
  
    int i = *(int*)j;
    while(1){
        pthread_mutex_lock(&countMutex);
    if(requestCount == nrequests)
                 {
//                     pthread_cond_broadcast(&requestAdded);
                     printf("thread %d unlocking mutex\n",i);
                     pthread_mutex_unlock(&countMutex);
                     printf("thread %d exiting\n",i);
                     pthread_exit(0);
                 }
        
            while (steque_isempty(requestQueue))
            {
                printf("thread %d waiting\n",i);
                pthread_cond_wait(&requestAdded,&countMutex);
                   printf("thread %d obtained the signal\n",i);
                       if(requestCount == nrequests)
                              {
                //                  pthread_cond_broadcast(&requestAdded);
                                  printf("thread %d unlocking mutex\n",i);
                                  pthread_mutex_unlock(&countMutex);
                                  printf("thread %d exiting\n",i);
                                  pthread_exit(0);
                              }
             
            }
        
 
        requestCount++;
        
        if(requestCount == nrequests)
        {
            printf("thread %d broadcasting signal\n",i);
            pthread_cond_broadcast(&requestAdded);
        }
        
     
        char* req_path = (char*) steque_pop(requestQueue);
           pthread_mutex_unlock(&countMutex);
    
    /*
     see if the queue has data, if not, then we could wait for main function to enqueue new requests, but we also need to ensure that all the requests are satisfied. we need to keep track of the number of requests processed and then compare it wit hthe total number ofrequests, and if only if it is less do we wait for the next enqued request. hence we'll need to create a global shared variable called the count whcih is protected by the mutex and is incremented everytime a request is initiatteid by the threads.
     *
     */
        
    char local_path[1066] = "";
         int returncode = 0;

        FILE *file = NULL;
 localPath(req_path, local_path);
 gfcrequest_t * gfr = NULL;

file = openFile(local_path);

gfr = gfc_create();
gfc_set_server(&gfr, server);
gfc_set_path(&gfr, req_path);
gfc_set_port(&gfr, port);
gfc_set_writefunc(&gfr, writecb);
gfc_set_writearg(&gfr, file);
fprintf(stdout, "thread %d Requesting %s%s\n", i,server, req_path);
//fprintf(stdout, "Requesting %s%s\n", server, req_path);

if ( 0 > (returncode = gfc_perform(&gfr))){
  fprintf(stdout, "gfc_perform returned an error %d\n", returncode);
  fclose(file);
  if ( 0 > unlink(local_path))
    fprintf(stderr, "warning: unlink failed on %s\n", local_path);
}
else {
    fclose(file);
}

if ( gfc_get_status(&gfr) != GF_OK){
  if ( 0 > unlink(local_path))
    fprintf(stderr, "warning: unlink failed on %s\n", local_path);
}

fprintf(stdout, "Status: %s\n", gfc_strstatus(gfc_get_status(&gfr)));
fprintf(stdout, "Received %zu of %zu bytes\n", gfc_get_bytesreceived(&gfr), gfc_get_filelen(&gfr));

gfc_cleanup(&gfr);
    
    /*
     lock countMutex
     if(count == maxLen):
        signal(free,countMutex)
     count --;
     unlock countMutex
     */
    }
}





/* Main ========================================================= */
int main(int argc, char **argv) {
/* COMMAND LINE OPTIONS ============================================= */
  
  char *workload_path = "workload.txt";

  int i = 0;
  int option_char = 0;
  int nthreads = 7;
    char* req_path = NULL;

  

  setbuf(stdout, NULL); // disable caching

  // Parse and set command line arguments
  while ((option_char = getopt_long(argc, argv, "t:n:r:s:w:hp:", gLongOptions, NULL)) != -1) {
    switch (option_char) {
      case 'h': // help
        Usage();
        exit(0);
        break;                      
      case 'n': // nrequests
        break;
      case 'r':
        nrequests = atoi(optarg);
        break;
      case 'p': // port
        port = atoi(optarg);
        break;
      default:
        Usage();
        exit(1);
      case 's': // server
        server = optarg;
        break;
      case 't': // nthreads
        nthreads = atoi(optarg);
        break;
      case 'w': // workload-path
        workload_path = optarg;
        break;
    }
  }

  if( EXIT_SUCCESS != workload_init(workload_path)){
    fprintf(stderr, "Unable to load workload file %s.\n", workload_path);
    exit(EXIT_FAILURE);
  }

  if (nthreads < 1) {
    nthreads = 1;
  }
  if (nthreads > MAX_THREADS) {
    nthreads = MAX_THREADS;
  }

  gfc_global_init();
    requestQueue = malloc(sizeof(steque_t));
    steque_init(requestQueue);

  /* add your threadpool creation here */

    pthread_t workers[nthreads];
    int indArr[nthreads];
    for (int i = 0 ; i < nthreads; i ++)
    {
        indArr[i] = i;
    }
    int j=0;
   
    for(  ; j < nthreads; j ++)
    {
        if(pthread_create(&(workers[j]), NULL, clientWork,(void*)&indArr[j]) !=0)
        {
            printf("coulnd't create thread\n");
            return 1;
        }
    }
    
  /* Build your queue of requests here */

    
    
  for(i = 0; i < nrequests; i++){
      req_path = workload_get_path();

      if(strlen(req_path) > 500){
        fprintf(stderr, "Request path exceeded maximum of 500 characters\n.");
        exit(EXIT_FAILURE);
      }

      steque_enqueue(requestQueue, req_path);
  }
    
      pthread_cond_broadcast(&requestAdded);

//    pthread_exit(NULL);
    
    for(int k = 0 ; k < nthreads; k++)
    {
       
        printf("joining thread %d\n",k);
        pthread_join(workers[k],NULL);
        printf("joined thread %d\n",k);
    }
    
    steque_destroy(requestQueue);
  gfc_global_cleanup(); // use for any global cleanup for AFTER your thread pool has terminated.

    pthread_mutex_destroy(&countMutex);
    pthread_cond_destroy(&requestAdded);
    pthread_exit(NULL);
    
}

