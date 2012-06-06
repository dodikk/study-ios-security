#include <CoreFoundation/CoreFoundation.h> 
#include <IOKit/IOCFPlugIn.h>


void disable_watchdog ( ) 
{
   CFMutableDictionaryRef matching = NULL;
   io_service_t service = 0;
   uint32_t zero = 0; 
   CFNumberRef n = NULL;
   
   
   matching = IOServiceMatching("IOWatchDogTimer");
   service = IOServiceGetMatchingService(kIOMasterPortDefault, matching); 
   n = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &zero);
   IORegistryEntrySetCFProperties(service, n);
   
   
   IOObjectRelease(service); 
   
   //memory leak in original source ???
   CFRelease( n );
   CFRelease( matching );
}
