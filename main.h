#include <stdbool.h>
#include <SDL2/SDL.h>

#define MEMORY_SIZE 4096

#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32

#define WIN_SCALE 10 // this number changes the size of the window

#define WIN_WIDTH DISPLAY_WIDTH *WIN_SCALE
#define WIN_HEIGHT DISPLAY_HEIGHT *WIN_SCALE

#define STACK_SIZE 16

#define PROGRAM_ADDRESS 0x200
#define FONTSET_ADDRESS 0x050

#define REGISTER_SIZE 16

// SETTINGS
#define ON_COLOR SDL_MAX_UINT32  // foreground color in hex
#define OFF_COLOR SDL_MIN_UINT32 // background color in hex
#define WRAP_GFX FALSE // FALSE if clip, TRUE if wrap. original chip-8 clipped, so the default is FALSE

unsigned char fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

typedef struct
{
    uint8_t memory[MEMORY_SIZE];
    uint32_t display[DISPLAY_HEIGHT][DISPLAY_WIDTH];
    uint16_t stack[STACK_SIZE];
    uint8_t sp;
    uint8_t V[REGISTER_SIZE];
    uint8_t keys[16];
    uint16_t pc;
    uint16_t I;
    uint8_t delay_timer;
    uint8_t sound_timer;
    bool drawFlag;
    bool waitForKey;
    uint8_t keyReg;
} chip_8;
