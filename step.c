#include "main.h"
#include <stdint.h>

void step(chip_8 *c)
{
    uint8_t firstNib = c->memory[c->pc];
    uint8_t secondNib = c->memory[c->pc + 1];
    uint16_t opcode = (firstNib << 8) | secondNib;
    c->pc += 2;
    switch (firstNib >> 4)
    {
    case 0x0:
        if (opcode == 0x00E0)
        {
            memset(c->display, 0, DISPLAY_WIDTH * DISPLAY_HEIGHT);
            c->drawFlag = true;
        }
    case 0x1:
        c->pc = opcode & 0x0FFF;
    case 0x6:
        c->V[(opcode >> 8) & 0x000F] = secondNib;
    case 0x7:
        c->V[(opcode >> 8) & 0x000F] += secondNib;
    case 0xa:
        c->I = opcode & 0x0FFF;
    }
}