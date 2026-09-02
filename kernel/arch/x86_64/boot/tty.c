#include <stdbool.h>
#include <kernel/io.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <kernel/tty.h>

#include "vga.h"

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static uint16_t* terminal_buffer = VGA_MEMORY;

static void enable_cursor(uint8_t cursor_start, uint8_t cursor_end) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | cursor_start);

    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | cursor_end);
}

static void disable_cursor() {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}

static void update_cursor(int x, int y) {
    uint16_t pos = y * VGA_WIDTH + x;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

static uint16_t get_cursor_position(void) {
    uint16_t pos = 0;
    outb(0x3D4, 0x0F);
    pos |= inb(0x3D5);
    outb(0x3D4, 0x0E);
    pos |= ((uint16_t)inb(0x3D5));
    return pos;
}

void terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    for (size_t y = 0; y < VGA_HEIGHT; ++y) {
        for (size_t x = 0; x < VGA_WIDTH; ++x) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
}

void terminal_setcolor(uint8_t color) {
    terminal_color = color;
}

void terminal_putentryat(unsigned char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA_WIDTH + x;
    terminal_buffer[index] = vga_entry(c, color);
}

void terminal_scroll(uint64_t line) {
    uint64_t loop;
    char c;

    for (loop = line * (VGA_WIDTH * 2) + VGA_MEMORY; loop < (line + 1) * (VGA_WIDTH * 2) + VGA_MEMORY; ++loop) {
        c = *(char *)loop;
        *(char*)(loop - (VGA_WIDTH * 2)) = c;
    }
}

void terminal_delete_last_line() {
    uint64_t x, *ptr;

    for (x = 0; x < VGA_WIDTH * 2; ++x) {
        ptr = VGA_MEMORY + (VGA_WIDTH * 2) * (VGA_HEIGHT -1) + x;
        *ptr = 0;
    }
}

static int serial_ready = 0;
static void serial_out(char c) {
    if (!serial_ready) {
        outb(0x3F8 + 1, 0x00);
        outb(0x3F8 + 3, 0x80);
        outb(0x3F8 + 0, 0x01);
        outb(0x3F8 + 1, 0x00);
        outb(0x3F8 + 3, 0x03);
        outb(0x3F8 + 2, 0xC7);
        outb(0x3F8 + 4, 0x0B);
        serial_ready = 1;
    }
    while ((inb(0x3F8 + 5) & 0x20) == 0);
    outb(0x3F8, c);
}

void terminal_putchar(char c) {
    serial_out(c);
    uint64_t line;
    unsigned char uc = c;

    if (c == '\n')
        terminal_column = VGA_WIDTH - 1;
    else
        terminal_putentryat(uc, terminal_color, terminal_column, terminal_row);
    if (++terminal_column == VGA_WIDTH) {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            for (line = 1; line <= VGA_HEIGHT - 1; ++line) {
                terminal_scroll(line);
            }
            terminal_delete_last_line();
            terminal_row = VGA_HEIGHT - 1;
        }
    }
    update_cursor(terminal_column, terminal_row);
}

void terminal_write(const char *data, size_t size) {
    for (size_t i = 0; i < size; ++i)
        terminal_putchar(data[i]);
}

void terminal_writestring(const char*data) {
    terminal_write(data,strlen(data));
}