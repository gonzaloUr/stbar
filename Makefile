include config.mk

.PHONY: all clean install uninstall

all: stbar

install: all
	mkdir -p $(PREFIX)/bin
	cp stbar $(PREFIX)/bin

uninstall:
	rm -f $(PREFIX)/bin/stbar

clean:
	rm -f stbar.o stbar
	rm -f components/*.o

stbar: stbar.o $(patsubst components/%.c,components/%.o,$(wildcard components/*.c))
	$(CC) $(LDFLAGS) $^ -o $@

stbar.o: stbar.c config.c
	$(CC) $(CFLAGS) -c stbar.c -o stbar.o

components/%.o: components/%.c components/%.h
	$(CC) $(CFLAGS) -c $< -o $@
