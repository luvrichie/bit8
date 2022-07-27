# bit8
> 👾 bit8 is a chip-8 interpreter that allows you to play chip-8 games with no hassle.

![a GIF of breakout by carmelo cortez being played on bit8](https://i.imgur.com/UDyfB1u.gif)

this tiny chip-8 interpreter (binary being 53 KB!) should handle everything you throw at it from test roms , to games and any other binaries/ROMs/whatever-you-call-it, that satisfies you.

## running ROMs
to run a ROM, you just run the interpreter's binary in the command line, followed by the path to a valid chip-8 ROM

```bash
./path/to/bit8/executable /path/to/rom
```
that's it! easy-peasy, lemon squeezy! 🍋

## building source
building the interpreter is just as straightforward as running it itself, since it requires only one framework. you can compile it with your compiler of choice, of course, but i build it with `clang` and i do it like so:
```bash
clang -o /path/to/output -lSDL2 main.c
```
that's about it!

## contributions
i LOVE contributions! if you have an idea, an issue or literally anything you want to tell me about this, feel free to make an issue. the more, the merrier!

## thanks, credits
below is documentaiton and guides that aided me through creating this, aswell any repositories that i snipped some code out. thank you!
https://github.com/thacks42/chip-8-emulator
https://github.com/dmatlack/chip8
https://tobiasvl.github.io/blog/write-a-chip-8-emulator/
https://multigesture.net/articles/how-to-write-an-emulator-chip-8-interpreter/
also, big thanks to the members of r/EmuDev's and The Coding Den's discord servers, for being such great help, even when i had really dumb-o questions. :P
