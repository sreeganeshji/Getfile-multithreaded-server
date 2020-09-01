
#include "gfserver-student.h"

#define USAGE                                                                 \
"usage:\n"                                                                    \
"  gfserver_main [options]\n"                                                 \
"options:\n"                                                                  \
"  -t [nthreads]       Number of threads (Default: 5)\n"                      \
"  -p [listen_port]    Listen port (Default: 19121)\n"                         \
"  -m [content_file]   Content file mapping keys to content files\n"          \
"  -h                  Show this help message.\n"                             \

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
  {"port",          required_argument,      NULL,           'p'},
  {"nthreads",      required_argument,      NULL,           't'},
  {"content",       required_argument,      NULL,           'm'},
  {"help",          no_argument,            NULL,           'h'},
  {NULL,            0,                      NULL,             0}
};


extern gfh_error_t gfs_handler(gfcontext_t **ctx, const char *path, void* arg);

static void _sig_handler(int signo){
  if ((SIGINT == signo) || (SIGTERM == signo)) {
    exit(signo);
  }
}
gfserver_t *gfs = NULL;
/*
 condition variable for when there is new request
 */
pthread_cond_t requestEnqued = PTHREAD_COND_INITIALIZER;
pthread_mutex_t queueMutex = PTHREAD_MUTEX_INITIALIZER;
void* serverThread(){
    while(1)
    {
        //if the queue is empty, wait, otherwise dequeue and continue.
        pthread_mutex_lock(&queueMutex);
        while(steque_isempty(clientRequestQueue))
        {
            pthread_cond_wait(&requestEnqued,&queueMutex);
//            printf("came out of the wait\n");
        }
        clientRequest *thisRequest;
        thisRequest =(clientRequest*) steque_pop(clientRequestQueue);
        pthread_mutex_unlock(&queueMutex);
        
        int fildes = content_get(thisRequest->path);
        //if invalid, send status.
        if(fildes < 0){
        gfs_sendheader(&thisRequest->ctx, GF_FILE_NOT_FOUND, 0);
        }
        else{
            struct stat fileStat;
            fstat(fildes,&fileStat);
            int fileSize = fileStat.st_size;
            gfs_sendheader(&thisRequest->ctx, GF_OK, fileSize);
            
            char buffer[1000];
            memset(buffer,0,sizeof(buffer));
            int bufSize = sizeof(buffer);
            int offset = 0;
            while(1){
                int readSize = pread(fildes, buffer, bufSize, offset);
                offset += readSize;
                gfs_send(&thisRequest->ctx, buffer, readSize);
                
                if(offset == fileSize)
                {
                    free(thisRequest);
                    break;
                    
                };
            }
        }
    }
    
}

/* Main ========================================================= */
int main(int argc, char **argv) {
  int option_char = 0;
  unsigned short port = 19121;
  char *content_map = "content.txt";
  int nthreads = 5;

  setbuf(stdout, NULL);

  if (SIG_ERR == signal(SIGINT, _sig_handler)){
    fprintf(stderr,"Can't catch SIGINT...exiting.\n");
    exit(EXIT_FAILURE);
  }

  if (SIG_ERR == signal(SIGTERM, _sig_handler)){
    fprintf(stderr,"Can't catch SIGTERM...exiting.\n");
    exit(EXIT_FAILURE);
  }

  // Parse and set command line arguments
  while ((option_char = getopt_long(argc, argv, "t:rhm:p:", gLongOptions, NULL)) != -1) {
    switch (option_char) {
      case 't': // nthreads
        nthreads = atoi(optarg);
        break;
      case 'p': // listen-port
        port = atoi(optarg);
        break;
      case 'h': // help
        fprintf(stdout, "%s", USAGE);
        exit(0);
        break;       
      default:
        fprintf(stderr, "%s", USAGE);
        exit(1);
      case 'm': // file-path
        content_map = optarg;
        break;                                          
    }
  }

  /* not useful, but it ensures the initial code builds without warnings */
  if (nthreads < 1) {
    nthreads = 1;
  }
    
    clientRequestQueue = malloc(sizeof(steque_t));
    steque_init(clientRequestQueue);

  content_init(content_map);
    


  /* Initialize thread management */
    
    pthread_t servers[nthreads];
    for(int i = 0 ; i < nthreads; i++)
    {
        if(pthread_create(&(servers[i]), NULL, serverThread,NULL) != 0 )
        {
            printf("couldn't create thread\n");
            return 1;
        }
    }

  /*Initializing server*/
  gfs = gfserver_create();

  /*Setting options*/
  gfserver_set_port(&gfs, port);
  gfserver_set_maxpending(&gfs, 16);
  gfserver_set_handler(&gfs, gfs_handler);
  gfserver_set_handlerarg(&gfs, NULL); // doesn't have to be NULL!

  /*Loops forever*/
  gfserver_serve(&gfs);
}
