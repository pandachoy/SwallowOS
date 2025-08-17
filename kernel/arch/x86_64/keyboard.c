#include <stdint.h>
#include <kernel/io.h>
#include <kernel/pic.h>

#define KEYBOARD_DATA_PORT      0x60
#define KEYBOARD_IRQ_VECTOR     0x21
#define KEYBOARD_IRQ_NUMBER      0x1

#define KEYBOARD_BUFFER_SIZE     256
volatile char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
volatile uint8_t key_buffer_pos = 0;

static char scancode_to_ascii(uint8_t scancode) {
    const char layout[] = "\x00\x1B" "1234567890-="
                         "\x08\tqwertyuiop[]\n"
                         "\x00asdfghjkl;'`"
                         "\x00\\zxcvbnm,./\x00"
                         "*\x00 ";
    return (scancode < sizeof(layout)) ? layout[scancode] : 0;
}

__attribute__((noreturn))
void keyboard_handler(void) {
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);

    /* 处理按键释放事件，忽略高位为1的扫描码 */
    if (scancode & 0x80) goto out;

    /* 扫描码转为ascii*/
    char c = scancode_to_ascii(scancode);

    if (c != 0 && key_buffer_pos < KEYBOARD_BUFFER_SIZE - 1) {
        keyboard_buffer[key_buffer_pos++] = c;
        keyboard_buffer[key_buffer_pos] = '\0'; /* 字符串终止 */
    }
    // keyboard_buffer[key_buffer_pos++] = 'c';
    // keyboard_buffer[key_buffer_pos] = '\0'; /* 字符串终止 */

out:
    PIC_sendEOI(KEYBOARD_IRQ_NUMBER);
}

void keyboard_init() {
    outb(KEYBOARD_IRQ_VECTOR, 0xFD);
}