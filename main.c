#include "main.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "step.c"

int load(chip_8 *c, char **argv)
{
    FILE *f = fopen(argv[1], "rb");
    if (f == NULL)
    {
        fprintf(stderr, "[!] error! couldn't open %s\n. are you sure you entered the right path?", argv[1]);
        exit(1);
    }

    // Get the file size and read it into a memory buffer
    fseek(f, 0L, SEEK_END);
    int fsize = ftell(f);
    fseek(f, 0L, SEEK_SET);
    if (fsize > MEMORY_SIZE - PROGRAM_ADDRESS)
    {
        fprintf(stderr, "[!] error! file too large to fit into RAM. are you sure you selected the right file?\n");
        exit(1);
    }
    fread(c->memory + PROGRAM_ADDRESS, fsize, 1, f);
    fclose(f);

    printf("[*] loaded file into memory.\n");
    return fsize;
}

void init(chip_8 *c)
{
    memset(c->memory, 0, MEMORY_SIZE);
    memset(c->display, 0, DISPLAY_WIDTH * DISPLAY_HEIGHT);
    memset(c->stack, 0, STACK_SIZE);
    memset(c->V, 0, REGISTER_SIZE);

    memcpy(c->memory + FONTSET_ADDRESS, fontset, 80);
    printf("[*] loaded fontset into memory.\n");

    c->pc = PROGRAM_ADDRESS;
    c->I = 0;

    c->delay_timer = 0;
    c->sound_timer = 0;

    c->drawFlag = false;
    printf("[*] init finished.\n");
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "[!] error! no ROM file specified in the argument, are you sure you ran the program like this:\n ./program [path/to/ROM]\n");
        exit(1);
    }
    chip_8 c;
    int fsize = load(&c, argv);
    init(&c);

    while (true)
    {
        if (c.pc < fsize)
        {
            step(&c);
            //drawing code should be here
            c.drawFlag = false;
        }
        else
        {
            printf("[*] emulation has reached the end of program! exiting...\n");
            exit(1);
        }
    }
}