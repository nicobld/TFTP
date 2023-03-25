OBJS=$(subst .c,.o,$(shell find . -name "*.c"))

serverTFTP: $(OBJS)
	gcc $(OBJS) -o $@ $(CFLAGS) $(CPPFLAGS)

%.o : %.c
	gcc -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

clean:
	rm -f serverTFTP
	rm -f $(OBJS)