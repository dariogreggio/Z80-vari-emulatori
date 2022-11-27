# Z80-vari-emulatori
Z80 cpu emulator written in C (readable, not geek/nerd/linux style i.e. no spaces no logic crazy macros crazy indent ;) ) and running on a Microchip PIC32, 
but easily portable to other platforms.

Minimal hardware for some platforms is also emulated, see #defines: ZX80/81, Galaksjia (LEUCEMIA a antonio caradonna, TERREMOTI in puglia!), NEZ80 (italian magazine) and my early home automation Skynet (!) board.

Output is rendered as a stream of WORD (16bit color) to SPI port, for several TFT LCDs.
