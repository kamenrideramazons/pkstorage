#ifndef HW_H
#define HW_H

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

#ifdef HOST
/* Desktop test build: hardware is replaced by plain memory. */
static u16 host_keyinput = 0x03FF;
static u8  host_sram[32768];
static u16 host_pal[512];
static u32 host_tiles[2048];
static u32 host_map[512];
static u16 host_dummy;

#define REG_KEYINPUT host_keyinput
#define REG_DISPCNT  host_dummy
#define REG_BG0CNT   host_dummy
#define SRAM         host_sram
#define PALRAM       host_pal
#define TILEMEM      host_tiles
#define MAPMEM       host_map
#define WAIT_VBLANK() ((void)0)
#else
#define REG_DISPCNT  (*(volatile u16 *)0x04000000)
#define REG_VCOUNT   (*(volatile u16 *)0x04000006)
#define REG_BG0CNT   (*(volatile u16 *)0x04000008)
#define REG_KEYINPUT (*(volatile u16 *)0x04000130)

#define PALRAM  ((volatile u16 *)0x05000000)
#define TILEMEM ((volatile u32 *)0x06000000)   /* char base 0   */
#define MAPMEM  ((volatile u32 *)0x0600F800)   /* screen base 31 */
/* SRAM is on an 8-bit bus: ONLY byte accesses are valid. */
#define SRAM    ((volatile u8 *)0x0E000000)

#define WAIT_VBLANK() do { while (REG_VCOUNT >= 160) {} \
                           while (REG_VCOUNT < 160) {} } while (0)
#endif

#define K_A      0x001
#define K_B      0x002
#define K_SELECT 0x004
#define K_START  0x008
#define K_RIGHT  0x010
#define K_LEFT   0x020
#define K_UP     0x040
#define K_DOWN   0x080
#define K_R      0x100
#define K_L      0x200

#endif
