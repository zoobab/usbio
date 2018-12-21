usbio: usbio.o
	$(CC) $(LDFLAGS) -o $@ $^ -l usb
%.o: %.c
	$(CC) $(CFLAGS) -c $<
clean:
	rm *.o usbio
