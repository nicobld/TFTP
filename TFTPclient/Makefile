OBJS=$(subst .c,.o,$(shell find . -name "*.c"))

clientTFTP: $(OBJS)
	gcc $(OBJS) -o $@ $(CFLAGS) $(CPPFLAGS)

%.o : %.c
	gcc -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

clean:
	rm -f clientTFTP
	rm -f $(OBJS)