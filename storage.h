#ifndef STORAGE_H
#define STORAGE_H

#include "hw.h"
#include "species.h"

#define NBOX      14
#define NSLOT     30
#define MON_BYTES 16
#define HDR_BYTES 12
#define SAVE_SIZE (HDR_BYTES + NBOX * NSLOT * MON_BYTES)
#define SLOT_A    0x0000
#define SLOT_B    0x4000

typedef struct {
    u16  species;      /* National Dex number, 0 = empty slot */
    u8   level;        /* 1..100 */
    u8   shiny;        /* 0 / 1 */
    char nick[11];     /* empty string = use species name */
} Mon;

static Mon boxes[NBOX][NSLOT];
static int curbox;

static int active_slot = -1;   /* which SRAM copy is the newest valid one */
static u16 active_seq;

static void mon_clear(Mon *m)
{
    int i;
    m->species = 0; m->level = 0; m->shiny = 0;
    for (i = 0; i < 11; i++) m->nick[i] = 0;
}

static const char *mon_name(const Mon *m)
{
    return m->nick[0] ? m->nick : species_name[m->species];
}

static void all_clear(void)
{
    int b, s;
    for (b = 0; b < NBOX; b++)
        for (s = 0; s < NSLOT; s++)
            mon_clear(&boxes[b][s]);
    curbox = 0;
}

static u16 calc_sum(u32 base)
{
    u16 s = 0;
    u32 i;
    for (i = HDR_BYTES; i < SAVE_SIZE; i++) {
        s = (u16)((s << 1) | (s >> 15));
        s = (u16)(s + SRAM[base + i]);
    }
    return s;
}

/* Returns 1 and the sequence number if the copy at `base` is intact. */
static int slot_valid(u32 base, u16 *seq)
{
    u16 sum;
    if (SRAM[base + 0] != 'P' || SRAM[base + 1] != 'K' ||
        SRAM[base + 2] != 'S' || SRAM[base + 3] != 'T') return 0;
    if (SRAM[base + 4] != 1) return 0;
    sum = (u16)(SRAM[base + 8] | (SRAM[base + 9] << 8));
    if (sum != calc_sum(base)) return 0;
    *seq = (u16)(SRAM[base + 6] | (SRAM[base + 7] << 8));
    return 1;
}

static void write_slot(u32 base, u16 seq)
{
    u32 o = base + HDR_BYTES;
    u16 sum;
    int b, s, i;

    SRAM[base] = 0;   /* invalidate this copy first (torn write => ignored) */

    for (b = 0; b < NBOX; b++) {
        for (s = 0; s < NSLOT; s++) {
            const Mon *m = &boxes[b][s];
            SRAM[o++] = (u8)(m->species & 0xFF);
            SRAM[o++] = (u8)(m->species >> 8);
            SRAM[o++] = m->level;
            SRAM[o++] = m->shiny;
            for (i = 0; i < 10; i++) SRAM[o++] = (u8)m->nick[i];
            SRAM[o++] = 0;
            SRAM[o++] = 0;
        }
    }
    sum = calc_sum(base);
    SRAM[base + 4] = 1;
    SRAM[base + 5] = (u8)curbox;
    SRAM[base + 6] = (u8)(seq & 0xFF);
    SRAM[base + 7] = (u8)(seq >> 8);
    SRAM[base + 8] = (u8)(sum & 0xFF);
    SRAM[base + 9] = (u8)(sum >> 8);
    SRAM[base + 10] = 0;
    SRAM[base + 11] = 0;
    /* magic last: the copy only becomes valid once everything else is in */
    SRAM[base + 0] = 'P'; SRAM[base + 1] = 'K';
    SRAM[base + 2] = 'S'; SRAM[base + 3] = 'T';
}

/* Writes to the copy that is NOT the newest one, then makes it the newest. */
static void save_all(void)
{
    int target = (active_slot == 0) ? 1 : 0;
    u16 seq = (u16)(active_seq + 1);
    write_slot(target ? SLOT_B : SLOT_A, seq);
    active_slot = target;
    active_seq = seq;
}

static void read_slot(u32 base)
{
    u32 o = base + HDR_BYTES;
    int b, s, i;
    for (b = 0; b < NBOX; b++) {
        for (s = 0; s < NSLOT; s++) {
            Mon *m = &boxes[b][s];
            m->species = (u16)(SRAM[o] | (SRAM[o + 1] << 8));
            m->level = SRAM[o + 2];
            m->shiny = SRAM[o + 3] ? 1 : 0;
            for (i = 0; i < 10; i++) {
                u8 c = SRAM[o + 4 + i];
                m->nick[i] = (c >= 32 && c <= 96) ? (char)c : 0;
            }
            m->nick[10] = 0;
            if (m->species < 1 || m->species > NSPECIES ||
                m->level < 1 || m->level > 100)
                mon_clear(m);
            o += MON_BYTES;
        }
    }
    curbox = SRAM[base + 5];
    if (curbox >= NBOX) curbox = 0;
}

/* Wraparound-safe "a - b" for 16-bit sequence counters. */
static int s16_diff(u16 a, u16 b)
{
    return (int)(short)(u16)(a - b);
}

static void load_save(void)
{
    u16 sa = 0, sb = 0;
    int va = slot_valid(SLOT_A, &sa);
    int vb = slot_valid(SLOT_B, &sb);
    all_clear();
    active_slot = -1;
    active_seq = 0;
    if (!va && !vb) {
        save_all();                 /* fresh cartridge: create a save */
        return;
    }
    if (va && vb)
        active_slot = ((s16_diff(sb, sa)) > 0) ? 1 : 0;
    else
        active_slot = va ? 0 : 1;
    read_slot(active_slot ? SLOT_B : SLOT_A);
    active_seq = active_slot ? sb : sa;
}

#endif
