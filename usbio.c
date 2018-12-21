#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <usb.h>

#define VERSION "2.2"

#define FALSE   0
#define TRUE    1

#define	SETPORT			0x54
#define	GETPORT			0x53
#define SETDIR			0x55
#define SETBIT			0x56
#define CLEARBIT		0x57

unsigned int def_vendor = 0x04d8, def_product = 0xf7c0, def_base = 16;
char binary[17];

void print_help(void)
{
     fprintf(stderr, "Usage: usbio [options] <command> [value]\n"
       "Control an USB I/O port\n\n"
       " -d, --device <vendorid>[:productid]\n"
       "    Select only device(s) with USB vendorid[:productid], default=0x%04x:0x%04x\n"
       " -s, --serial <serial number>\n"
       "    Select only the device with the given serial number, default=any\n"
       " -o, --output <base>\n"
       "    Set output format. Base x=hexadecimal (16), b=binary (2), d=decimal (10), default=x\n"
       " -v, --verbose\n"
       "    Verbose mode\n"
       " -V, --version\n"
       "    Show program version\n"
       " -h, --help\n"
       "    Show usage and help\n\n"
       " <command>: getport | setport <n> | setdir <n> | setbit <n> | clearbit <n>\n\n",
       def_vendor, def_product
    );
}

long int convert(char *value)
{
  long int result;
  char *endptr;
  errno = 0;
  if (strncasecmp(value, "0b", 2) == 0)
    result = strtol(&value[2], &endptr, 2);
  else
   result = strtol(value, &endptr, 0);
}

char * itob(unsigned int value)
{
   int i;
   for (i=0; i < 16; i++)
   {
     if (value & 0x8000) binary[i] = '1';
     else binary[i] = '0';

     value <<= 1;
   }

   binary[i] = '\0';
   return binary;
}

int main(int argc, char *argv[])
{
  static const struct option long_options[] =
  {
    { "version", 0, 0, 'V' },
    { "verbose", 0, 0, 'v' },
    { "help", 0, 0, 'h' },
    { "device", 1, 0, 'd' },
    { "serial", 1, 0, 's' },
    { "output", 1, 0, 'o' },
    { 0, 0, 0, 0 }
  };

  int c, err = 0;
  int verbose = 0, help = 0;
  char *device, *serial = "";

  struct usb_bus *bus;
  struct usb_device *dev;
  usb_dev_handle *udev;
  int dev_found, ret, iserial, base = def_base;
  unsigned char buffer[8], *cmd, dev_serial[100];
  unsigned int vendor = def_vendor, product = def_product;
  unsigned int bRequest = 0; // request
  unsigned int wValue = 0; // request data
  unsigned int wIndex = 0; // not used, optional data
  unsigned int wLength = 0; // length of buffer[]
  unsigned int bmRequestType = USB_TYPE_VENDOR | USB_RECIP_DEVICE;


  // process arguments
  while ((c = getopt_long(argc, argv, "Vvhd:s:o:", long_options, NULL)) != EOF)
  {
     switch(c)
     {
       case 'V':
        printf("usbio " VERSION "\n");
        return 0;

      case 'v':
        verbose++;
        break;

      case 'h':
        help=1;
        break;

      case 's':
        serial = optarg;
        break;

      case 'o':
           switch(optarg[0])
           {
             case 'x': base = 16; break;
             case 'b': base = 2; break;
             case 'd': base = 10; break;
             default: err++; break;
           } // switch
        break;

      case 'd':
        vendor = 0; product = 0;
        device = strchr(optarg, ':');
        if (device) *device++ = 0;

        if (*optarg) vendor = convert(optarg);
        if (device) product = convert(device);
        break;

      case '?':
      default:
        err++;
        break;
        
     } // switch
  } // while

  if (err || help || argc <= optind)
  {
    print_help();
    return -1;
  } // if

  if (verbose)
  {
   fprintf(stderr, "Search parameters: usb vendor = 0x%04x, product = 0x%04x, serial = \"%s\"\n", vendor, product, serial);
   // for (c = optind; c < argc; c++) fprintf(stderr, "argv[%d] = \"%s\"\n", c, argv[c]);
  } // if

  // parse command part
  cmd = argv[optind];
  if (strcmp(cmd, "setport") == 0)
  {
     bRequest = SETPORT;
     if (optind+1 >= argc ) { print_help(); return -1; }
     wValue = convert(argv[optind+1]);
  }
   else if (strcmp(cmd, "getport") == 0)
   {
     bRequest = GETPORT;
     bmRequestType = USB_TYPE_VENDOR | USB_ENDPOINT_IN;
     wLength = 8;
   }
   else if (strcmp(cmd, "setdir") == 0)
   {
     bRequest = SETDIR;
     if (optind+1 >= argc) { print_help(); return -1; }
     wValue = convert(argv[optind+1]);
   }
   else if (strcmp(cmd, "setbit") == 0)
   {
     bRequest = SETBIT;
     if (optind+1 >= argc) { print_help(); return -1; }
     wValue = convert(argv[optind+1]);
   }
   else if (strcmp(cmd, "clearbit") == 0)
   {
     bRequest = CLEARBIT;
     if (optind+1 >= argc) { print_help(); return -1; }
     wValue = convert(argv[optind+1]);
   }
   else
   {
       fprintf(stderr, "No such command\n\n");
       print_help();
       return -1;
   }
   if (verbose)
   {
    fprintf(stderr, "bRequest = 0x%02x, wValue = 0x%02x, bmRequestType = 0x%02x, wLength = 0x%02x\n", bRequest, wValue, bmRequestType, wLength);
   }

  // search USB device
   usb_init();

   usb_find_busses();
   usb_find_devices();

   dev_found = FALSE;
   for (bus = usb_get_busses(); bus && !dev_found; bus = bus->next)
   {
       for (dev = bus->devices; dev && !dev_found; dev = dev->next)
       {
           dev_serial[0] = 0;
           errno = 0;
           udev = usb_open(dev);
           if (errno)
           { 
              strerror_r(errno, dev_serial, sizeof(dev_serial)); // may get permission denied
              udev = NULL;
           }
           else
           {
             iserial = dev->descriptor.iSerialNumber;
             if (iserial)
             {
              usb_get_string_simple(udev, iserial, dev_serial, sizeof(dev_serial));
             }
             else
             {
              strcpy(dev_serial, "<none>");
             }
           }

           if (verbose) fprintf(stderr, "Checking to match USB device with vendor = 0x%04x, product = 0x%04x, serial = %s\n", dev->descriptor.idVendor, dev->descriptor.idProduct, dev_serial);

           if  ((!vendor || (vendor == dev->descriptor.idVendor)) &&
                (!product || (product == dev->descriptor.idProduct)))
           {
               if (*serial)
               {
                  if (strcmp(dev_serial, serial) == 0) { dev_found = TRUE; break; }
               }
               else
               { dev_found = TRUE; break; }
           } // if
           if (udev != NULL) { usb_close(udev); }
       } // for dev
   } // for bus

   if (!dev_found)
   {
       fprintf(stderr, "No matching device found...\n");
       return -2;
   }
   else
   {
       if (verbose) { fprintf(stderr, "Matching device found.\n"); }
   } // if
   

   if (usb_claim_interface(udev, 0) < 0)
   {
       perror("Unable to claim interface");
       return -3;
   } // if

   // do usb transaction
   if (udev)
   {
      ret = usb_control_msg(udev, bmRequestType, bRequest, wValue, wIndex, buffer, wLength, 100);
   }

       if (ret < 0)
       {
           perror("Unable to send vendor specific request");
       }
       else
       {
        if (verbose) fprintf(stderr, "USB buffer length = %d\n", ret);

        if ( ret > 0 )
        {
            wValue = ((unsigned int)(buffer[1]) << 8) + buffer[0];
/*
            printf("0x%02hhx%02hhx\n", buffer[1], buffer[0]);
            printf("%hu\n", wValue);
            printf("0b%s\n", itob(wValue));
*/
          switch(base)
          {
            case 16: printf("0x%02hhx%02hhx\n", buffer[1], buffer[0]);
                     break;
            case 10: printf("%u\n", wValue);
                     break;
            case 2:  printf("0b%s\n", itob(wValue));
                     break;
          } // switch

        } // if
       } //else

 usb_close(udev);
 return 0;

} // main
