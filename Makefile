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

stbar: stbar.o $(COMPONENTS)
	$(CC) $(LDFLAGS) stbar.o $(COMPONENTS) -o stbar

stbar.o: stbar.c stbar.h config.h
	$(CC) $(CFLAGS) -c stbar.c -o stbar.o

config.h: config.def.h
	cp config.def.h config.h

components/%.o: components/%.c components/%.h
	$(CC) $(CFLAGS) -c $< -o $@
