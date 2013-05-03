CFLAGS = -O2 -Wall

OBJS = asi.o

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

all: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o asi

clean:
	@rm -f *.o asi
