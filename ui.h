#ifndef UI_H
#define UI_H

#include "hw.h"
#include "font.h"
#include "storage.h"

/* Palette banks: [text colour, background colour] */
#define RGB(r, g, b) ((u16)((r) | ((g) << 5) | ((b) << 10)))
#define NAVY RGB(2, 4, 10)
#define PAL_NORMAL 0
#define PAL_CURSOR 1
#define PAL_DIM    2
#define PAL_TITLE  3
#define PAL_GREEN  4
#define PAL_WARN   5
#define PAL_SHINY  6
#define PAL_TYPING 7

static const u16 pals[8][2] = {
    { RGB(31, 31, 31), NAVY },
    { RGB(1, 2, 6),    RGB(31, 28, 8) },
    { RGB(12, 14, 20), NAVY },
    { RGB(31, 31, 12), RGB(5, 8, 18) },
    { RGB(14, 31, 14), NAVY },
    { RGB(31, 12, 10), NAVY },
    { RGB(31, 26, 6),  NAVY },
    { RGB(31, 31, 31), RGB(4, 18, 8) },
};

static u16 scr[32 * 32];

static void clear_scr(void)
{
    int i;
    for (i = 0; i < 32 * 32; i++) scr[i] = 0;
}

static void put_ch(int x, int y, int pal, char c)
{
    if (x < 0 || x >= 30 || y < 0 || y >= 20) return;
    if (c >= 'a' && c <= 'z') c = (char)(c - 32);
    if (c < 32 || c > 96) c = '?';
    scr[y * 32 + x] = (u16)((c - 32) | (pal << 12));
}

static int put_str(int x, int y, int pal, const char *s)
{
    while (*s) put_ch(x++, y, pal, *s++);
    return x;
}

static void fill(int x, int y, int w, int pal)
{
    int i;
    for (i = 0; i < w; i++) put_ch(x + i, y, pal, ' ');
}

static int put_num(int x, int y, int pal, int n, int w, char pad)
{
    char b[8];
    int i;
    for (i = w - 1; i >= 0; i--) {
        if (n > 0 || i == w - 1) { b[i] = (char)('0' + n % 10); n /= 10; }
        else b[i] = pad;
    }
    for (i = 0; i < w; i++) put_ch(x + i, y, pal, b[i]);
    return x + w;
}

/* name limited to 10 characters */
static int put_name(int x, int y, int pal, const char *s)
{
    int i;
    for (i = 0; i < 10 && s[i]; i++) put_ch(x + i, y, pal, s[i]);
    return x + i;
}

static void present(void)
{
    int i;
    for (i = 0; i < 512; i++)
        MAPMEM[i] = (u32)scr[2 * i] | ((u32)scr[2 * i + 1] << 16);
}

static int box_count(int b)
{
    int s, n = 0;
    for (s = 0; s < NSLOT; s++) if (boxes[b][s].species) n++;
    return n;
}

static void title_bar(const char *left)
{
    fill(0, 0, 30, PAL_TITLE);
    put_str(1, 0, PAL_TITLE, left);
}

/* ---- Box screen ------------------------------------------------------- */
static int cur;                 /* cursor slot 0..29 */
static int holding, hold_box, hold_slot;
static const char *msg;
static int msg_t;

static void render_box(int confirm)
{
    int s;
    const Mon *c = &boxes[curbox][cur];

    clear_scr();
    title_bar("PKMN STORAGE");
    put_str(16, 0, PAL_TITLE, "BOX ");
    put_num(20, 0, PAL_TITLE, curbox + 1, 2, '0');
    put_num(24, 0, PAL_TITLE, box_count(curbox), 2, ' ');
    put_str(26, 0, PAL_TITLE, "/30");

    if (msg_t > 0 && msg) put_str(1, 1, PAL_GREEN, msg);

    for (s = 0; s < NSLOT; s++) {
        const Mon *m = &boxes[curbox][s];
        int x = 1 + (s / 15) * 15;
        int y = 2 + (s % 15);
        int pal = m->species ? (m->shiny ? PAL_SHINY : PAL_NORMAL) : PAL_DIM;
        if (holding && curbox == hold_box && s == hold_slot) pal = PAL_GREEN;
        if (s == cur) pal = PAL_CURSOR;
        fill(x, y, 14, pal);
        put_num(x, y, pal, s + 1, 2, '0');
        if (m->species) {
            put_name(x + 3, y, pal, mon_name(m));
            if (m->shiny) put_ch(x + 13, y, pal, '*');
        } else {
            put_str(x + 3, y, pal, "-----");
        }
    }

    if (holding) {
        const Mon *h = &boxes[hold_box][hold_slot];
        put_str(0, 17, PAL_GREEN, "HOLDING ");
        put_name(8, 17, PAL_GREEN, mon_name(h));
        put_str(19, 17, PAL_GREEN, "FROM BOX ");
        put_num(28, 17, PAL_GREEN, hold_box + 1, 2, '0');
    }

    if (!c->species) {
        put_str(0, 18, PAL_DIM, "EMPTY SLOT");
    } else {
        put_str(0, 18, PAL_NORMAL, "NO.");
        put_num(3, 18, PAL_NORMAL, c->species, 3, '0');
        put_name(7, 18, PAL_NORMAL, species_name[c->species]);
        put_str(18, 18, PAL_NORMAL, " LV");
        put_num(21, 18, PAL_NORMAL, c->level, 3, ' ');
        if (c->shiny) put_str(24, 18, PAL_SHINY, " SHINY");
    }

    if (confirm)
        put_str(0, 19, PAL_WARN, "RELEASE FOR GOOD?  A:YES B:NO");
    else if (holding)
        put_str(0, 19, PAL_DIM, "A:PLACE B:BACK SEL:DEL L/R:BOX");
    else
        put_str(0, 19, PAL_DIM, "A:PICK ST:EDIT SEL:DEL L/R:BOX");
}

/* ---- Edit screen ------------------------------------------------------ */
static Mon edit;
static int ef;                   /* selected field 0..3 */
static int typing, npos;
static char nickbuf[10];

static void render_edit(void)
{
    int i, r;
    clear_scr();
    title_bar("EDIT POKEMON");
    put_str(18, 0, PAL_TITLE, "BOX ");
    put_num(22, 0, PAL_TITLE, curbox + 1, 2, '0');
    put_str(25, 0, PAL_TITLE, "#");
    put_num(26, 0, PAL_TITLE, cur + 1, 2, '0');

    for (i = 0; i < 4; i++) {
        int row = 4 + i * 2;
        int pal = (ef == i && !typing) ? PAL_CURSOR : PAL_NORMAL;
        fill(1, row, 28, pal);
        switch (i) {
        case 0:
            put_str(2, row, pal, "SPECIES");
            put_str(11, row, pal, "<");
            put_num(13, row, pal, edit.species, 3, '0');
            put_name(17, row, pal, species_name[edit.species]);
            put_str(28, row, pal, ">");
            break;
        case 1:
            put_str(2, row, pal, "LEVEL");
            put_str(11, row, pal, "<");
            put_num(13, row, pal, edit.level, 3, ' ');
            put_str(17, row, pal, ">");
            break;
        case 2:
            put_str(2, row, pal, "SHINY");
            put_str(11, row, pal, "<");
            put_str(13, row, pal, edit.shiny ? "YES" : "NO ");
            put_str(17, row, pal, ">");
            break;
        case 3:
            put_str(2, row, pal, "NICKNAME");
            put_str(11, row, pal, "[");
            if (typing) {
                for (r = 0; r < 10; r++)
                    put_ch(12 + r, row, r == npos ? PAL_TYPING : PAL_NORMAL,
                           nickbuf[r]);
            } else if (edit.nick[0]) {
                put_name(12, row, pal, edit.nick);
            } else {
                put_name(12, row, PAL_DIM, species_name[edit.species]);
            }
            put_str(22, row, pal, "]");
            break;
        }
    }

    put_str(1, 13, PAL_DIM, "BLANK NICKNAME = SPECIES NAME");

    if (typing) {
        put_str(0, 17, PAL_DIM, "UP/DN:CHARACTER LEFT/RT:MOVE");
        put_str(0, 19, PAL_DIM, "A OR B: DONE TYPING");
    } else {
        put_str(0, 17, PAL_DIM, "UP/DN:FIELD  LEFT/RIGHT:CHANGE");
        put_str(0, 18, PAL_DIM, "L/R:BIG STEP  SEL:CLEAR NICK");
        put_str(0, 19, PAL_DIM, "START:SAVE  B:CANCEL  A:TYPE");
    }
}

#endif
