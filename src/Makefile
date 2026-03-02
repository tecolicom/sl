all: sl-1985 sl-2010 sl-2023

sl-1985: sl.c
	$(CC) -std=c89 \
		-Wno-implicit-int \
		-Wno-implicit-function-declaration \
		-Wno-incompatible-function-pointer-types \
		-include term.h -DCM=cursor_address \
		-o $@ $< -lcurses

%: %.c
	$(CC) -o $@ $< -lcurses

clean:
	rm -f sl-{1985,2010,2023}
