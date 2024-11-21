SRC = os_linux.c os_msgLinux.c os_log.c 
FOBJ = libos.so 
 
OBJS = $(SRC:.c=.o) 
CFLAGS = -Wall -Werror -D__LINUX_OS_FC__ -fPIC -I./include/ $(DF_FLAGS)

$(FOBJ).so: $(OBJS)
	$(CC) $(CFLAGS) -shared $(OBJS) -o $@
	$(STRIP) $@

sinclude $(SRC:.c=.d) 

%.o: ./src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.d: ./src/%.c
	@$(CC) -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

.PHONY : clean
clean :
	$(RM) $(FOBJ) $(SRC:.c=.d) $(OBJS) 
