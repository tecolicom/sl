#include <curses.h>
#include <signal.h>

#define	NCOLS (COLS-1)
#define	NLINES (LINES)

char *ban[] = {
    "      o \0\0\0\0",
    "     o \0\0\0\0\0",
    "    o  ____ ",
    "   --  |OO| ",
    "  _||__|  | ",
    " |        | ",
    "/-O=O O=O-- "
};

main (argc, argv) 
int argc;
char	*argv[];
{
    int	die();
    int i, j;
    int ceil;
    int width = 11;

    initscr();
    signal(SIGINT,die);
    leaveok(stdscr,TRUE);
    scrollok(stdscr,FALSE);

    ceil = (LINES-7)/2;
    for (i=NCOLS; i ; i--) {
	for (j=0; j < 7; j++) {
	    fputs (tgoto (CM, i, j + ceil), stdout);
	    putns (ban[j], NCOLS - i);
	}
	refresh();
    }
    for (i=0; i <= width ; i++) {
	for (j=0; j < 7; j++) {
	    fputs (tgoto (CM, 0, j + ceil), stdout);
	    putns (ban[j] + i, 100);
	}
	refresh();
    }
    die();
}

putns (s, i)
register char *s;
register int i;
{
    char *cp;

    for (cp = s ; *cp && i; cp++, i--)
	putchar (*cp);
}

die()
{
    signal(SIGINT,SIG_IGN);
    mvcur(0,COLS-1,LINES-1,0);
    endwin();
    exit(0);
}
