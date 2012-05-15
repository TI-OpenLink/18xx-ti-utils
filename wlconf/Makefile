CFLAGS = -O2 -Wall

OBJS = main.o

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

all: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o wlconf

clean:
	@rm -f *.o wlconf
