NCURSES_CFLAGS := $(shell ncurses6-config --cflags 2>/dev/null)
NCURSES_LIBS   := $(or $(shell ncurses6-config --libs 2>/dev/null),-lcurses)

all: sl-1985 sl-2010 sl-2023

sl-1985: sl.c
	$(CC) -std=c89 -w \
		-Wno-incompatible-function-pointer-types \
		-Wno-incompatible-pointer-types \
		$(NCURSES_CFLAGS) \
		-include term.h -DCM=cursor_address \
		-o $@ $< $(NCURSES_LIBS)

%: %.c
	$(CC) $(NCURSES_CFLAGS) -o $@ $< $(NCURSES_LIBS)

clean:
	rm -f sl-{1985,2010,2023}
