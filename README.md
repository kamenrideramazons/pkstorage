# PKMN STORAGE — a Pokémon box storage for the Game Boy Advance

A homebrew GBA ROM that works like a PC box system: **14 boxes × 30 slots** (420 Pokémon),
all 386 Gen 1–3 species, saved to cartridge SRAM.

## Build the ROM

**Option A – GitHub (nothing to install):** push this folder to a GitHub repo.
The included workflow builds it; download `pkstorage-gba` from the Actions run.

**Option B – locally:** install [devkitARM](https://devkitpro.org/wiki/Getting_Started), then

    make

This produces `pkstorage.gba` (runs in mGBA, NanoBoyAdvance, real hardware via flash cart).
Saves are plain 32 KB SRAM (the ROM contains the `SRAM_V113` marker so emulators and
flash carts pick the right save type automatically).

## Controls

Box screen

| Button | Action |
|---|---|
| D-pad | Move cursor (hold to repeat) |
| L / R | Previous / next box |
| A | Pick up a Pokémon, then A again on another slot to place or swap it (A on the same slot cancels) |
| B | Cancel pick-up |
| START | Create / edit the Pokémon in the slot |
| SELECT | Release (delete) — asks for confirmation (A = yes, B = no) |

Edit screen

| Button | Action |
|---|---|
| Up / Down | Choose field (species, level, shiny, nickname) |
| Left / Right | Change value by 1 |
| L / R | Change value by 25 (species) or 10 (level) |
| A on NICKNAME | Type: Up/Down changes the letter, Left/Right moves, A or B finishes |
| SELECT | Clear nickname (blank = show species name) |
| START | Save and return |
| B | Cancel |

## Save safety

The save is written to two alternating SRAM copies with a checksum and a sequence number.
A copy is only marked valid after it is completely written, so a power cut mid-save leaves
the previous copy intact. Picking up a Pokémon never removes it from its slot until it is
placed, so nothing can be lost while "holding" one.

## Limitations (please read)

* This is a **standalone storage tool**. It stores species / level / shiny / nickname in its
  own format. It does **not** read or write real Pokémon save files, and it does not import
  Pokémon from a game cartridge or via link cable. Real PK3 data (80 bytes each) would not
  fit 420 entries in 32 KB SRAM anyway, and import would need cart-reading or link-cable code.
* The code was **not compiled for ARM or run on an emulator** by me — only its logic and
  screen layout were tested on a PC (see below). Please run `make` and try it in mGBA first.

## Tests

    make test

Compiles the game logic on your PC with the hardware replaced by plain memory, simulates
button presses, checks picking/placing/swapping/releasing, persistence across a simulated
power cut, corrupted and garbage SRAM, and writes screenshots to `test/out/`.

## Layout

    src/main.c       input + game logic + entry point
    src/ui.h         text renderer and screens
    src/storage.h    data model and SRAM save/load
    src/hw.h         GBA registers (+ desktop test shim)
    src/font.h       GENERATED tile data      (tools/gen.py)
    src/species.h    GENERATED species names  (tools/gen.py)
    tools/           font art, species lists, generator, screenshot renderer
