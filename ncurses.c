/*  ncurses.c
 *
 *  ncurses video driver for Nanox
 */

#define termdef 1

#define _XOPEN_SOURCE_EXTENDED 1
#include <curses.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "estruct.h"
#include "edef.h"
#include "efunc.h"
#include "utf8.h"

#define MARGIN  8
#define SCRSIZ  64
#define NPAUSE  2
#define BEL     0x07

void ncurses_open(void);
void ncurses_close(void);
void ncurses_kopen(void);
void ncurses_kclose(void);
int ncurses_getchar(void);
int ncurses_putchar(int c);
void ncurses_flush(void);
void ncurses_move(int row, int col);
void ncurses_eeol(void);
void ncurses_eeop(void);
void ncurses_beep(void);
void ncurses_rev(int state);
void ncurses_italic(int state);
void ncurses_set_colors(int fg, int bg);
void ncurses_set_attrs(int bold, int underline, int italic);
int ncurses_cres(char *res);

struct terminal term = {
    0, 0, 0, 0,
    MARGIN, SCRSIZ, NPAUSE,
    ncurses_open,
    ncurses_close,
    ncurses_kopen,
    ncurses_kclose,
    ncurses_getchar,
    ncurses_putchar,
    ncurses_flush,
    ncurses_move,
    ncurses_eeol,
    ncurses_eeop,
    ncurses_beep,
    ncurses_rev,
    ncurses_italic,
    ncurses_set_colors,
    ncurses_set_attrs,
    ncurses_cres,
};

static int current_pair = 0;
static attr_t current_attr = A_NORMAL;

/* Map color index to ncurses color */
static int map_color(int c) {
    if (c == -1) return -1;
    if (c < 8) return c;
    if (c < 16) return c; 
    return c; 
}

/* Pair management */
#define MAX_PAIRS 1024
static int next_pair = 1;
struct color_pair_entry {
    int fg, bg, pair;
} pair_table[MAX_PAIRS];

static int get_pair(int fg, int bg) {
    if (fg == -1 && bg == -1) return 0;
    
    for (int i = 0; i < next_pair - 1; i++) {
        if (pair_table[i].fg == fg && pair_table[i].bg == bg)
            return pair_table[i].pair;
    }
    if (next_pair < COLOR_PAIRS && next_pair < MAX_PAIRS) {
        init_pair(next_pair, map_color(fg), map_color(bg));
        pair_table[next_pair-1].fg = fg;
        pair_table[next_pair-1].bg = bg;
        pair_table[next_pair-1].pair = next_pair;
        return next_pair++;
    }
    return 0;
}

void ncurses_open(void) {
    setlocale(LC_ALL, "");
    initscr();
    raw();
    noecho();
    nonl();
    keypad(stdscr, TRUE);
    if (has_colors()) {
        start_color();
        use_default_colors();
    }
    term.t_nrow = LINES - 1;
    term.t_ncol = COLS;
    term.t_mrow = MAXROW;
    term.t_mcol = MAXCOL;
}

void ncurses_close(void) {
    endwin();
}

void ncurses_kopen(void) {
}

void ncurses_kclose(void) {
}

int ncurses_getchar(void) {
    wint_t wc;
    int result = wget_wch(stdscr, &wc);

    if (result == ERR)
        return 0;

    if (result == KEY_CODE_YES) {
        switch (wc) {
        case KEY_UP: return SPEC | 'P';
        case KEY_DOWN: return SPEC | 'N';
        case KEY_LEFT: return SPEC | 'B';
        case KEY_RIGHT: return SPEC | 'F';
        case KEY_PPAGE: return SPEC | 'Z';
        case KEY_NPAGE: return SPEC | 'V';
        case KEY_HOME: return SPEC | '<';
        case KEY_END: return SPEC | '>';
        case KEY_DC: return SPEC | 'D';
        case KEY_BACKSPACE: return 0x08;
        default:
            if (wc >= KEY_F(1) && wc <= KEY_F(12)) {
                int fn = (int)(wc - KEY_F(1));
                static const int spec_map[12] = {
                    '1','2','3','4','5','6','7','8','9','0','-','='
                };
                return SPEC | spec_map[fn];
            }
            return 0;
        }
    }

    if (wc == 127)
        return 0x08;

    return (int)wc;
}

int ncurses_putchar(int c) {
    char buf[8];
    int len = unicode_to_utf8(c, buf);
    buf[len] = 0;
    addstr(buf);
    return 0;
}

void ncurses_flush(void) {
    refresh();
}

void ncurses_move(int row, int col) {
    move(row, col);
}

void ncurses_eeol(void) {
    clrtoeol();
}

void ncurses_eeop(void) {
    clrtobot();
}

void ncurses_beep(void) {
    beep();
}

void ncurses_rev(int state) {
    if (state) current_attr |= A_REVERSE;
    else current_attr &= ~A_REVERSE;
    attr_set(current_attr, current_pair, NULL);
}

void ncurses_italic(int state) {
#ifdef A_ITALIC
    if (state) current_attr |= A_ITALIC;
    else current_attr &= ~A_ITALIC;
    attr_set(current_attr, current_pair, NULL);
#endif
}

void ncurses_set_colors(int fg, int bg) {
    if (!has_colors()) return;
    
    if (fg & 0x01000000) fg = -1;
    if (bg & 0x01000000) bg = -1;

    current_pair = get_pair(fg, bg);
    attr_set(current_attr, current_pair, NULL);
}

void ncurses_set_attrs(int bold, int underline, int italic) {
    if (bold) current_attr |= A_BOLD; else current_attr &= ~A_BOLD;
    if (underline) current_attr |= A_UNDERLINE; else current_attr &= ~A_UNDERLINE;
#ifdef A_ITALIC
    if (italic) current_attr |= A_ITALIC; else current_attr &= ~A_ITALIC;
#endif
    attr_set(current_attr, current_pair, NULL);
}

int ncurses_cres(char *res) {
    return TRUE;
}
