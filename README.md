# Getfile-multithreaded-server

Multithreaded [Getfile](https://github.com/sreeganeshji/GetFile-protocol) system.

<image src="https://raw.githubusercontent.com/sreeganeshji/socket-echo-client-server/master/Illustrations/mtgf.png" height=150>

this project provides multithread server client implementation of the get file protocol library. 
 
# Tradeoffs considered
The boss worker thread model was employed in which the threads are created initially and then the requests are queued in the main thread which are accessed by the threads and protected by a mutex. This choice of pre creating the threads before the requests were issued as supposed to creating the threads as the requested are issued was chosen as there would be more time for parallel processing in the latter case than the former. This is because we can avoid the overhead of creating new threads each time a new request came along and destroying them. 
# Flow of control
The main thread of the server creates a pool of worker threads and using the gflibrary, initializes the server and waits to accept the client requests.  
The client creates a pool of worker threads and enqueues the requests onto the shared queue and broadcasts to the threads to dequeue the requests and send them to the server. 
Once the server accepts the clients request, it parses the message and checks for any errors. Once it has successfully parsed the path, it calls the handler function. The handler function enqueues the path onto the shared queue. The worker threads check for the existence of the file and sends the header and the file to the connected client. Once the client receives all the threads and has exhausted all the requests, it cleans up all the memory consumed and exits. 
# Code testing
The library was tested on how it handles multiple requests in parallel, different sized files, and requests with different status values. 
# References
The architecture for the implementation was inspired by the high level diagram listed on the references in the GitHub page. 
To find the length fo the file, the function fstat was used which was referred from https://linux.die.net/man/2/fstat 
The function used to read the file using the file descriptor, pread was referred from http://man7.org/linux/man-pages/man2/pread.2.html
