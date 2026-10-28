/* PKMN STORAGE - a Pokemon box storage homebrew for the Game Boy Advance.
 * Bare-metal C, no libraries; needs devkitARM only for crt0/linker/toolchain. */
#include "hw.h"
#include "storage.h"
#include "ui.h"

/* Tells emulators / flash carts to use plain 32KB SRAM for saving. */
const char sram_id[] __attribute__((used)) = "SRAM_V113";

enum { ST_BOX, ST_EDIT, ST_CONFIRM };
static int state;
static Mon *conf_m;

/* ---- Input with auto-repeat on D-pad and shoulder buttons -------------- */
static u16 k_held, k_down, k_rep;
static u8  k_cnt[10];

static void poll(void)
{
    int i;
    u16 k = (u16)(~REG_KEYINPUT) & 0x03FF;
    k_down = (u16)(k & ~k_held);
    k_held = k;
    k_rep = k_down;
    for (i = 4; i < 10; i++) {
        if (k & (1 << i)) {
            if (k_cnt[i] < 255) k_cnt[i]++;
            if (k_cnt[i] >= 18 && ((k_cnt[i] - 18) % 3) == 0)
                k_rep |= (u16)(1 << i);
        } else {
            k_cnt[i] = 0;
        }
    }
}

static void notify(const char *m) { msg = m; msg_t = 90; }

/* ---- Box screen logic --------------------------------------------------- */
static void swap_mons(Mon *a, Mon *b)
{
    Mon t = *a; *a = *b; *b = t;
}

static void start_edit(void)
{
    const Mon *m = &boxes[curbox][cur];
    int i;
    if (m->species) {
        edit = *m;
    } else {
        mon_clear(&edit);
        edit.species = 1;
        edit.level = 5;
    }
    ef = 0;
    typing = 0;
    npos = 0;
    for (i = 0; i < 10; i++) nickbuf[i] = ' ';
    state = ST_EDIT;
}

static void update_box(void)
{
    Mon *m;

    if (k_rep & K_UP)    cur = (cur / 15) * 15 + (cur % 15 + 14) % 15;
    if (k_rep & K_DOWN)  cur = (cur / 15) * 15 + (cur % 15 + 1) % 15;
    if (k_rep & (K_LEFT | K_RIGHT)) cur = (cur + 15) % 30;
    if (k_down & K_L) { curbox = (curbox + NBOX - 1) % NBOX; save_all(); }
    if (k_down & K_R) { curbox = (curbox + 1) % NBOX; save_all(); }

    m = &boxes[curbox][cur];

    if (k_down & K_A) {
        if (!holding) {
            /* Pick up: the Pokemon stays in its slot (and in the save) until
             * it is placed, so a power cut can never lose it. */
            if (m->species) { holding = 1; hold_box = curbox; hold_slot = cur; }
        } else {
            Mon *h = &boxes[hold_box][hold_slot];
            if (h != m) {            /* swap (or move, if target is empty) */
                swap_mons(h, m);
                save_all();
                notify("SAVED");
            }
            holding = 0;
        }
    }
    if ((k_down & K_B) && holding) holding = 0;

    if (k_down & K_SELECT) {
        Mon *t = holding ? &boxes[hold_box][hold_slot] : m;
        if (t->species) { conf_m = t; state = ST_CONFIRM; }
    }
    if (k_down & K_START) start_edit();
}

static void update_confirm(void)
{
    if (k_down & K_A) {
        if (holding && conf_m == &boxes[hold_box][hold_slot]) holding = 0;
        mon_clear(conf_m);
        save_all();
        notify("RELEASED");
        state = ST_BOX;
    } else if (k_down & K_B) {
        state = ST_BOX;
    }
}

/* ---- Edit screen logic -------------------------------------------------- */
static const char charset[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-.'!?";

static int cs_index(char c)
{
    int i;
    for (i = 0; charset[i]; i++) if (charset[i] == c) return i;
    return 0;
}

static void nick_commit(void)
{
    int i, last = -1;
    for (i = 0; i < 10; i++) if (nickbuf[i] != ' ') last = i;
    for (i = 0; i < 11; i++) edit.nick[i] = 0;
    for (i = 0; i <= last; i++) edit.nick[i] = nickbuf[i];
}

static void nick_begin(void)
{
    const char *src = edit.nick[0] ? edit.nick : species_name[edit.species];
    int i;
    for (i = 0; i < 10; i++) nickbuf[i] = ' ';
    for (i = 0; i < 10 && src[i]; i++) nickbuf[i] = src[i];
    npos = 0;
    typing = 1;
}

static int wrap(int v, int lo, int hi)
{
    int n = hi - lo + 1;
    return lo + ((v - lo) % n + n) % n;
}

static void update_edit(void)
{
    int d = 0, big;

    if (typing) {
        int n = (int)sizeof(charset) - 1;
        int ci = cs_index(nickbuf[npos]);
        if (k_rep & K_UP)    nickbuf[npos] = charset[(ci + 1) % n];
        if (k_rep & K_DOWN)  nickbuf[npos] = charset[(ci + n - 1) % n];
        if ((k_rep & K_RIGHT) && npos < 9) npos++;
        if ((k_rep & K_LEFT)  && npos > 0) npos--;
        if (k_down & (K_A | K_B)) { nick_commit(); typing = 0; }
        return;
    }

    if (k_rep & K_UP)   ef = (ef + 3) % 4;
    if (k_rep & K_DOWN) ef = (ef + 1) % 4;

    if (k_rep & (K_LEFT | K_R | K_L | K_RIGHT)) {
        big = (k_rep & (K_L | K_R)) ? 1 : 0;
        if (k_rep & (K_RIGHT | K_R)) d = 1;
        if (k_rep & (K_LEFT | K_L))  d = -1;
        if (ef == 0) {
            edit.species = (u16)wrap(edit.species + d * (big ? 25 : 1), 1, NSPECIES);
        } else if (ef == 1) {
            edit.level = (u8)wrap(edit.level + d * (big ? 10 : 1), 1, 100);
        } else if (ef == 2) {
            edit.shiny ^= 1;
        }
    }

    if ((k_down & K_A) && ef == 3) nick_begin();
    if (k_down & K_SELECT) {
        int i;
        for (i = 0; i < 11; i++) edit.nick[i] = 0;
        for (i = 0; i < 10; i++) nickbuf[i] = ' ';
    }
    if (k_down & K_START) {
        boxes[curbox][cur] = edit;
        save_all();
        notify("SAVED");
        state = ST_BOX;
    }
    if (k_down & K_B) state = ST_BOX;
}

/* ---- Frame / init ------------------------------------------------------- */
static void game_frame(void)
{
    poll();
    if (msg_t > 0) msg_t--;
    if (state == ST_BOX)          update_box();
    else if (state == ST_CONFIRM) update_confirm();
    else                          update_edit();

    if (state == ST_EDIT) render_edit();
    else                  render_box(state == ST_CONFIRM);
}

static void game_init(void)
{
    int b, i;
    REG_DISPCNT = 0x0080;                       /* forced blank while loading */
    for (b = 0; b < 8; b++) {
        PALRAM[b * 16 + 0] = 0;
        PALRAM[b * 16 + 1] = pals[b][0];        /* ink        */
        PALRAM[b * 16 + 2] = pals[b][1];        /* background */
    }
    PALRAM[0] = NAVY;                           /* backdrop */
    for (i = 0; i < FONT_WORDS; i++) TILEMEM[i] = font_tiles[i];
    REG_BG0CNT = 0x1F00;                        /* screen base 31, char base 0, 4bpp */

    load_save();
    state = ST_BOX;
    cur = 0;
    holding = 0;
    render_box(0);
    present();
    REG_DISPCNT = 0x0100;                       /* mode 0, BG0 on */
}

#ifndef HOST
int main(void)
{
    game_init();
    for (;;) {
        WAIT_VBLANK();
        present();
        game_frame();
    }
    return 0;
}
#endif
