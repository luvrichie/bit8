#include "main.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <SDL2/SDL.h>
#include <inttypes.h>

int load(chip_8 *c, char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp)
    {
        fprintf(stderr, "[!] failed to open the selected file!\n");
        exit(1);
    }
    fseek(fp, 0L, SEEK_END);
    int size = ftell(fp);
    rewind(fp);
    if (size > MEMORY_SIZE - PROGRAM_ADDRESS)
    {
        fprintf(stderr, "[!] file is too large, are you sure you selected the right binary file?\n");
        exit(1);
    }
    fread(c->memory + PROGRAM_ADDRESS, 1, size, fp);
    printf("[*] loaded selected binary.\n");
    fclose(fp);
    return size;
}

int init(chip_8 *c)
{
    memset(c->memory, 0, MEMORY_SIZE);
    memset(c->display, OFF_COLOR, DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(c->display[0][0]));
    memset(c->stack, 0, STACK_SIZE);
    memset(c->V, 0, REGISTER_SIZE);
    memset(c->keys, 0, 16);

    memcpy(c->memory + FONTSET_ADDRESS, fontset, 80);
    printf("[*] loaded fontset into memory.\n");

    c->pc = PROGRAM_ADDRESS;
    c->I = 0;
    c->sp = 0;

    c->delay_timer = 0;
    c->sound_timer = 0;

    c->drawFlag = true;
    c->waitForKey = false;
    printf("[*] init finished.\n");

    return 1;
}

// helper func
void push(chip_8 *c, uint16_t value)
{
    if (c->sp + 1 >= STACK_SIZE - 1)
    {
        fprintf(stderr, "stack overflow!!!");
        return;
    }

    c->sp++;
    c->stack[c->sp] = value;
}

uint16_t pop(chip_8 *c)
{
    if (c->sp == 0)
    {
        fprintf(stderr, "stack underflow!!!");
        return -1;
    }
    uint16_t result = c->stack[c->sp];
    c->sp--;
    return result;
}

// 00E0: clear display
// the title says it all; clears chip-8's display.
void clear_display(chip_8 *c)
{
    memset(c->display, OFF_COLOR, DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(c->display[0][0]));
    c->drawFlag = true;
}

// 00EE: return from subroutine
// pop the last address from stack and set the PC to it
void return_subroutine(chip_8 *c)
{
    c->pc = pop(c);
}

// 1NNN: jump to address
// to say it differently: sets the program counter to the address provided.
void jmp_addr(chip_8 *c, uint16_t addr)
{
    c->pc = addr;
}

// 2NNN: calls subroutine
// In other words, just like 1NNN, you should set PC to NNN. However, the difference between a jump and a call is that this instruction should first push the
// current PC to the stack, so the subroutine can return later.
// (from: https://tobiasvl.github.io/blog/write-a-chip-8-emulator/#00ee-and-2nnn-subroutines)
void call_subroutine(chip_8 *c, uint16_t addr)
{
    push(c, c->pc);
    c->pc = addr;
}

// 3XNN: skip if Vx is equal to nn
void skip_equal(chip_8 *c, uint8_t vx, uint16_t value)
{
    if (c->V[vx] == value)
    {
        c->pc += 2;
    }
}

// 4XNN: skip if Vx is NOT equal to nn
void skip_not_equal(chip_8 *c, uint8_t vx, uint16_t value)
{
    if (c->V[vx] != value)
    {
        c->pc += 2;
    }
}

// 5XY0: skip if Vx is equal to Vy
void skip_equal_reg(chip_8 *c, uint8_t vx, uint8_t vy)
{
    if (c->V[vx] == c->V[vy])
    {
        c->pc += 2;
    }
}

// 6XNN: set Vx to a value
// the title says it all, again. sets one of the regs to a provided value.
void set_reg(chip_8 *c, uint8_t vx, uint8_t value)
{
    c->V[vx] = value;
}

// 7XNN: add value to Vx
// same as 6XNN, but adds instead of asigning.
void add_reg(chip_8 *c, uint8_t vx, uint8_t value)
{
    c->V[vx] += value;
}

// 8XY0: set Vx to Vy
void set_reg_to_reg(chip_8 *c, uint8_t vx, uint8_t vy)
{
    c->V[vx] = c->V[vy];
}

// 8XY1: Vx is set to bitwise OR of Vy
void bit_or_reg(chip_8 *c, uint8_t vx, uint8_t vy)
{
    c->V[vx] |= c->V[vy];
    c->V[0xf] = 0;
}

// 8XY2: Vx is set to bitwise AND of Vy
void bit_and_reg(chip_8 *c, uint8_t vx, uint8_t vy)
{
    c->V[vx] &= c->V[vy];
    c->V[0xf] = 0;
}

// 8XY3: Vx is set to bitwise XOR of Vy
void bit_xor_reg(chip_8 *c, uint8_t vx, uint8_t vy)
{
    c->V[vx] ^= c->V[vy];
    c->V[0xf] = 0;
}

// 8XY4: set Vx to Vx + Vy
// aditionally, if result is larger than 255, VF is set to 1
void add_reg_to_reg(chip_8 *c, uint8_t vx, uint8_t vy)
{
    int result = c->V[vx] + c->V[vy];
    if (result > 0xff)
    {
        c->V[0xf] = 1;
    }
    else
    {
        c->V[0xf] = 0;
    }
    c->V[vx] = result;
}

// 8XY5: set Vx to Vx - Vy
// if Vx is larger than Vy, VF is set to 1, however if Vy is bigger, then VF is set to 0.
void sub_reg(chip_8 *c, uint8_t vx, uint8_t vy)
{
    int result = c->V[vx] - c->V[vy];
    if (c->V[vx] > c->V[vy])
    {
        c->V[0xf] = 1;
    }
    else
    {
        c->V[0xf] = 0;
    }
    c->V[vx] = result & 0xff;
}

// 8XY6: shift Vx one bit to right
void shift_reg_right(chip_8 *c, uint8_t vx, uint8_t vy)
{
    c->V[vx] = c->V[vy];
    uint8_t bit = (c->V[vx]) & 1;
    c->V[vx] >>= 1;
    c->V[0xf] = bit;
}

// 8XY7: set Vx to Vy - Vx
// if Vx is larger than Vy, VF is set to 1, however if Vy is bigger, then VF is set to 0.
void sub_reg_rev(chip_8 *c, uint8_t vx, uint8_t vy)
{
    int result = c->V[vy] - c->V[vx];
    if (c->V[vy] > c->V[vx])
    {
        c->V[0xf] = 1;
    }
    else
    {
        c->V[0xf] = 0;
    }
    c->V[vx] = result & 0xff;
}

// 8XYE: shift Vx one bit to left
void shift_reg_left(chip_8 *c, uint8_t vx, uint8_t vy)
{
    c->V[vx] = c->V[vy];
    c->V[vx] <<= 1;
    c->V[0xF] = (c->V[vx] >> 7) & 1;
}

// 9XY0: skip if Vx is NOT equal to Vy
void skip_not_equal_reg(chip_8 *c, uint8_t vx, uint8_t vy)
{
    if (c->V[vx] != c->V[vy])
    {
        c->pc += 2;
    }
}

// ANNN: set I to a given value
// the title. says. everything. AGAIN!
void set_index_reg(chip_8 *c, uint16_t value)
{
    c->I = value;
}

// BNNN: jump to address plus V0
void jmp_addr_V0(chip_8 *c, uint16_t addr)
{
    c->pc = addr + c->V[0];
}

// CXNN: generate a random number
// generate a random number (probably 0 to 255), binary AND it with nn and set Vx to it.
void num_gen(chip_8 *c, uint8_t vx, uint16_t value)
{
    int rand_int = rand() % 256;
    c->V[vx] = rand_int & value;
}

// DXYN: draw sprite
// it is used to draw a “sprite” on the screen. Each sprite consists of 8-bit bytes, where each bit corresponds to a horizontal pixel; sprites are between 1 and
// 15 bytes tall. They’re drawn to the screen by treating all 0 bits as transparent, and all the 1 bits will “flip” the pixels in the locations of the screen that
// it’s drawn to. (You might recognize this as logical XOR.)
// (from: https://tobiasvl.github.io/blog/write-a-chip-8-emulator/)
void draw_sprite(chip_8 *c, uint8_t vx, uint8_t vy, uint8_t n)
{
    int y_pos = (c->V[vy] & 0x1F);
    int x_pos = (c->V[vx] & 0x3F);
    c->V[0xf] = 0;
    for (int sprite_row = 0; sprite_row < n; sprite_row++)
    {
        if (y_pos + sprite_row >= 32)
        {
            break;
        }
        for (int sprite_bit = 0; sprite_bit < 8; sprite_bit++)
        {
            int screen_y = (y_pos + sprite_row) % DISPLAY_HEIGHT;
            int screen_x = (x_pos + sprite_bit) % DISPLAY_WIDTH;
            if (x_pos + sprite_bit >= 64)
            {
                break;
            }
            uint32_t screen_pixel = c->display[screen_y][screen_x];
            uint8_t sprite_pixel = ((c->memory[c->I + sprite_row]) >> (7 - sprite_bit)) & 1;
            if (screen_pixel == ON_COLOR && sprite_pixel != 0)
            {
                c->display[screen_y][screen_x] = OFF_COLOR;
                c->V[0xf] = 1;
            }
            else if (screen_pixel == OFF_COLOR && sprite_pixel != 0)
            {
                c->display[screen_y][screen_x] = ON_COLOR;
            }
        }
    }
    c->drawFlag = true;
}

// EX9E: skip next instruction if key, that's stored in Vx is pressed
void skip_if_key_pressed(chip_8 *c, uint8_t vx)
{
    if (c->keys[c->V[vx]] == 1)
    {
        c->pc += 2;
    }
}

// EX9E: skip next instruction if key, that's stored in Vx is NOT pressed
void skip_if_key_not_pressed(chip_8 *c, uint8_t vx)
{
    if (c->keys[c->V[vx]] == 0)
    {
        c->pc += 2;
    }
}

// FX07: set Vx to delay timer
void set_reg_timer(chip_8 *c, uint8_t vx)
{
    c->V[vx] = c->delay_timer;
}

// FX0A: halt until key press and store in Vx
void wait_for_key(chip_8 *c, uint8_t reg)
{
    c->waitForKey = true;
    c->keyReg = reg;
}

void key_event(chip_8 *c, uint8_t key)
{
    c->V[c->keyReg] = key;
    c->waitForKey = false;
}

// FX15: set delay timer to Vx
void set_delay_timer(chip_8 *c, uint8_t vx)
{
    c->delay_timer = c->V[vx];
}

// FX18: set sound timer to Vx
void set_sound_timer(chip_8 *c, uint8_t vx)
{
    c->sound_timer = c->V[vx];
}

// FX1E: add Vx to index register
void add_reg_index(chip_8 *c, uint8_t vx)
{
    c->I += c->V[vx];
}

// FX29: get font character
void get_font_char(chip_8 *c, uint8_t vx)
{
    c->I = FONTSET_ADDRESS + c->V[vx] * 5;
}

// FX33: binary-coded decimal conversion
// takes the number from Vx and converts it to 3 decimal digits, storing them
// in memory at the address in the index register.
// for example, if Vx contains 156, (or 9C in hex), it would put the number 1 at the addres in I,
// 5 in address I + 1, and 6 un address I + 2
void bcd(chip_8 *c, uint8_t vx)
{
    int num = c->V[vx];
    int ones = num % 10;
    int tens = (num / 10) % 10;
    int huns = (num / 100) % 10;
    c->memory[c->I + 0] = huns & 0xff;
    c->memory[c->I + 1] = tens & 0xff;
    c->memory[c->I + 2] = ones & 0xff;
}

// FX55: store rgister data in memory
void store_reg(chip_8 *c, uint8_t x)
{
    for (int i = 0; i <= x; i++)
    {
        c->memory[c->I + i] = c->V[i];
    }
    c->I += x + 1;
}

// FX65: load register data from memory
void load_reg(chip_8 *c, uint8_t x)
{
    for (int i = 0; i <= x; i++)
    {
        c->V[i] = c->memory[c->I + i];
    }
    c->I += x + 1;
}

void step(chip_8 *c)
{
    bool continue_exec = false;
    if (c->waitForKey)
    {
        for (int i = 0; i < 16; i++)
        {
            if (c->keys[i] != 0)
            {
                key_event(c, i);
                continue_exec = true;
                break;
            }
        }
    }
    else
    {
        continue_exec = true;
    }
    if (!continue_exec)
        return;
    uint8_t hi = c->memory[c->pc];
    uint8_t lo = c->memory[c->pc + 1];
    uint16_t full = (hi << 8) | lo;
    c->pc += 2;
    switch (hi >> 4)
    {
    case 0x0:
        if (full == 0x00E0)
        {
            clear_display(c);
        }
        else if (full == 0x00EE)
        {
            return_subroutine(c);
        }
        else
        {
            printf("[!] unimplemented opcode! 0x%04x\n", full);
        }
        break;
    case 0x1:
        jmp_addr(c, full & 0xfff);
        break;
    case 0x2:
        call_subroutine(c, full & 0xfff);
        break;
    case 0x3:
        skip_equal(c, hi & 0xf, lo);
        break;
    case 0x4:
        skip_not_equal(c, hi & 0xf, lo);
        break;
    case 0x5:
        skip_equal_reg(c, hi & 0xf, lo >> 4);
        break;
    case 0x6:
        set_reg(c, hi & 0xf, lo);
        break;
    case 0x7:
        add_reg(c, hi & 0xf, lo);
        break;
    case 0x8:
        switch (lo & 0xf)
        {
        case 0x0:
            set_reg_to_reg(c, hi & 0xf, lo >> 4);
            break;
        case 0x1:
            bit_or_reg(c, hi & 0xf, lo >> 4);
            break;
        case 0x2:
            bit_and_reg(c, hi & 0xf, lo >> 4);
            break;
        case 0x3:
            bit_xor_reg(c, hi & 0xf, lo >> 4);
            break;
        case 0x4:
            add_reg_to_reg(c, hi & 0xf, lo >> 4);
            break;
        case 0x5:
            sub_reg(c, hi & 0xf, lo >> 4);
            break;
        case 0x6:
            shift_reg_right(c, hi & 0xf, lo >> 4);
            break;
        case 0x7:
            sub_reg_rev(c, hi & 0xf, lo >> 4);
            break;
        case 0xe:
            shift_reg_left(c, hi & 0xf, lo >> 4);
            break;
        default:
            printf("[!] unimplemented opcode! 0x%04x\n", full);
        }
        break;
    case 0x9:
        skip_not_equal_reg(c, hi & 0xf, lo >> 4);
        break;
    case 0xa:
        set_index_reg(c, full & 0xfff);
        break;
    case 0xb:
        jmp_addr_V0(c, full & 0xfff);
        break;
    case 0xc:
        num_gen(c, hi & 0xf, lo);
        break;
    case 0xd:
        draw_sprite(c, hi & 0xf, lo >> 4, lo & 0xf);
        break;
    case 0xe:
        switch (lo)
        {
        case 0x9e:
            skip_if_key_pressed(c, hi & 0xf);
            break;
        case 0xa1:
            skip_if_key_not_pressed(c, hi & 0xf);
            break;
        default:
            printf("[!] unimplemented opcode! 0x%04x\n", full);
        }
        break;
    case 0xf:
        switch (lo)
        {
        case 0x07:
            set_reg_timer(c, hi & 0xf);
            break;
        case 0x0a:
            wait_for_key(c, hi & 0xf);
            break;
        case 0x15:
            set_delay_timer(c, hi & 0xf);
            break;
        case 0x18:
            set_sound_timer(c, hi & 0xf);
            break;
        case 0x1e:
            add_reg_index(c, hi & 0xf);
            break;
        case 0x29:
            get_font_char(c, hi & 0xf);
            break;
        case 0x33:
            bcd(c, hi & 0xf);
            break;
        case 0x55:
            store_reg(c, hi & 0xf);
            break;
        case 0x65:
            load_reg(c, hi & 0xf);
            break;
        default:
            printf("[!] unimplemented opcode! 0x%04x\n", full);
        }
        break;
    default:
        printf("[!] unimplemented opcode! 0x%04x\n", full);
    }
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "[!] error! no ROM file specified in the argument, are you sure you ran the program like this:\n ./program [path/to/ROM]\n");
        exit(1);
    }
    chip_8 c;
    init(&c);
    int fsize = load(&c, argv[1]);

    const float target_frametime = 1.0f/60.0f;

    SDL_Window *mainWin = NULL;
    SDL_Renderer *winRenderer = NULL;
    SDL_Texture *winTexture = NULL;
    SDL_Rect rect = {0, 0, WIN_WIDTH, WIN_HEIGHT};
    // initializing SDL
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        fprintf(stderr, "[!] error! SDL failed to initialize. error: %s\n", SDL_GetError());
        exit(1);
    }
    else
    {
        // creating a window
        mainWin = SDL_CreateWindow("CHIP-8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIN_WIDTH, WIN_HEIGHT, SDL_WINDOW_SHOWN);
        if (mainWin == NULL)
        {
            fprintf(stderr, "[!] error! SDL failed to create a window, error: %s\n", SDL_GetError());
            exit(1);
        }
        else
        {
            winRenderer = SDL_CreateRenderer(mainWin, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
            if (winRenderer == NULL)
            {
                fprintf(stderr, "[!] error! SDL failed to make a renderer, error: %s\n", SDL_GetError());
                exit(1);
            }
            else
            {
                winTexture = SDL_CreateTexture(winRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, DISPLAY_WIDTH, DISPLAY_HEIGHT);
                if (winTexture == NULL)
                {
                    fprintf(stderr, "[!] error! SDL failed to create a texture, error: %s\n", SDL_GetError());
                    exit(1);
                }
            }
        }
    }

    uint64_t last_time = SDL_GetPerformanceCounter();
    float dt = 0.0f;
    bool running = true;
    while (running)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e) != 0)
        {
            switch (e.type)
            {
            case SDL_QUIT:
                running = false;
                break;
            default:
                break;
            }
        }

        uint64_t new_time = SDL_GetPerformanceCounter();
        float diff = new_time - last_time;
        diff = diff / (float)(SDL_GetPerformanceFrequency());
        float diff_ms = diff * 1000.0f;
        dt += diff_ms;
        if (dt > 16.666f)
        {
            dt -= 16.666f;
            if (c.delay_timer > 0)
                c.delay_timer--;
            if (c.sound_timer > 0)
                c.sound_timer--;
        }
        last_time = new_time;

        const uint8_t *key_state = SDL_GetKeyboardState(NULL);
        c.keys[0x0] = (bool)key_state[SDL_SCANCODE_X];
        c.keys[0x1] = (bool)key_state[SDL_SCANCODE_1];
        c.keys[0x2] = (bool)key_state[SDL_SCANCODE_2] || (bool)key_state[SDL_SCANCODE_UP];
        c.keys[0x3] = (bool)key_state[SDL_SCANCODE_3];
        c.keys[0xc] = (bool)key_state[SDL_SCANCODE_4];
        c.keys[0x4] = (bool)key_state[SDL_SCANCODE_Q] || (bool)key_state[SDL_SCANCODE_LEFT];
        c.keys[0x5] = (bool)key_state[SDL_SCANCODE_W];
        c.keys[0x6] = (bool)key_state[SDL_SCANCODE_E] || (bool)key_state[SDL_SCANCODE_RIGHT];
        c.keys[0xd] = (bool)key_state[SDL_SCANCODE_R];
        c.keys[0x7] = (bool)key_state[SDL_SCANCODE_A];
        c.keys[0x8] = (bool)key_state[SDL_SCANCODE_S] || (bool)key_state[SDL_SCANCODE_DOWN];
        c.keys[0x9] = (bool)key_state[SDL_SCANCODE_D];
        c.keys[0xe] = (bool)key_state[SDL_SCANCODE_F];
        c.keys[0xa] = (bool)key_state[SDL_SCANCODE_Z];
        c.keys[0x0] = (bool)key_state[SDL_SCANCODE_X];
        c.keys[0xb] = (bool)key_state[SDL_SCANCODE_C];
        c.keys[0xf] = (bool)key_state[SDL_SCANCODE_V];

        step(&c);
        if (c.drawFlag)
        {
            SDL_SetRenderDrawColor(winRenderer, 0, 0, 0, 0);
            SDL_RenderClear(winRenderer);
            SDL_UpdateTexture(winTexture, &rect, c.display, DISPLAY_WIDTH * sizeof(Uint32));
            SDL_RenderCopy(winRenderer, winTexture, NULL, &rect);
            SDL_RenderPresent(winRenderer);
            c.drawFlag = false;
        }
        if (diff_ms < target_frametime)
        {

            SDL_Delay(target_frametime - diff_ms);
        }
    }
}
