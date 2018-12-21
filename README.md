USBIO
=====

This is a copy of Peter Jakab USBIO tool:


http://jap.hu/electronic/usbio.html

Goal
====

Port it to other USB-GPIO chips, such as the CH341A, where an example of a GPIO toggle is here:

https://github.com/sarim/ch341a-bitbang-userland

Compile
=======

You need to install libusb-devel, and then run ```make```, you should end up with a ```usbio``` binary.

Screenshot
==========

```
$ ./usbio
Usage: usbio [options] <command> [value]
Control an USB I/O port

 -d, --device <vendorid>[:productid]
    Select only device(s) with USB vendorid[:productid], default=0x04d8:0xf7c0
 -s, --serial <serial number>
    Select only the device with the given serial number, default=any
 -o, --output <base>
    Set output format. Base x=hexadecimal (16), b=binary (2), d=decimal (10), default=x
 -v, --verbose
    Verbose mode
 -V, --version
    Show program version
 -h, --help
    Show usage and help

 <command>: getport | setport <n> | setdir <n> | setbit <n> | clearbit <n>
```

Todo
====

* Add a Dockerfile
* Rewrite it in GO, or python
* Add more examples
