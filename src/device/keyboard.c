#include "keyboard.h"
#include "interrupt.h"
#include "console.h"
#include "io.h"
#include "ioqueue.h"

#define KBD_PROT 0x60

#define ESC             0x1b
#define BACKSPACE       '\b'
#define TAB             '\t'
#define ENTER           '\r'
#define DELETE          0x7f

#define CHAR_INVISIBLE  0
#define CTRL_L_CHAR     CHAR_INVISIBLE
#define CTRL_R_CHAR     CHAR_INVISIBLE
#define SHIFT_L_CHAR    CHAR_INVISIBLE
#define SHIFT_R_CHAR    CHAR_INVISIBLE
#define ALT_L_CHAR      CHAR_INVISIBLE
#define ALT_R_CHAR      CHAR_INVISIBLE
#define CAPS_LOCK_CHAR  CHAR_INVISIBLE
#define F1_CHAR         CHAR_INVISIBLE
#define F2_CHAR         CHAR_INVISIBLE
#define F3_CHAR         CHAR_INVISIBLE
#define F4_CHAR         CHAR_INVISIBLE
#define F5_CHAR         CHAR_INVISIBLE
#define F6_CHAR         CHAR_INVISIBLE
#define F7_CHAR         CHAR_INVISIBLE
#define F8_CHAR         CHAR_INVISIBLE
#define F9_CHAR         CHAR_INVISIBLE
#define F10_CHAR        CHAR_INVISIBLE
#define F11_CHAR        CHAR_INVISIBLE
#define F12_CHAR        CHAR_INVISIBLE

#define SHIFT_L_MAKE    0x2a
#define SHIFT_R_MAKE    0x36
#define ALT_L_MAKE      0x38
#define ALT_R_MAKE      0xe038
#define ALT_R_BREAK     0xe0b8
#define CTRL_L_MAKE     0x1d
#define CTRL_R_MAKE     0xe01d
#define CTRL_R_BREAK    0xe09d
#define CAPS_LOCK_MAKE  0x3a
#define F1_MAKE         0x3b
#define F2_MAKE         0x3c
#define F3_MAKE         0x3d
#define F4_MAKE         0x3e
#define F5_MAKE         0x3f
#define F6_MAKE         0x40
#define F7_MAKE         0x41
#define F8_MAKE         0x42
#define F9_MAKE         0x43
#define F10_MAKE        0x44
#define F11_MAKE        0x45
#define F12_MAKE        0x46

static uint8_t ctrl_status, shift_status, alt_status, caps_lock_status, ext_scancode;
struct ioqueue kbd_buf;

static char keymap[][2] = {
    {0, 0},
    {ESC, ESC},
    {'1', '!'},
    {'2', '@'},
    {'3', '#'},
    {'4', '$'},
    {'5', '%'},
    {'6', '^'},
    {'7', '&'},
    {'8', '*'},
    {'9', '('},
    {'0', ')'},
    {'-', '_'},
    {'=', '+'},
    {BACKSPACE, BACKSPACE},
    {TAB, TAB},
    {'q', 'Q'},
    {'w', 'W'},
    {'e', 'E'},
    {'r', 'R'},
    {'t', 'T'},
    {'y', 'Y'},
    {'u', 'U'},
    {'i', 'I'},
    {'o', 'O'},
    {'p', 'P'},
    {'[', '{'},
    {']', '}'},
    {ENTER, ENTER},
    {CTRL_L_CHAR, CTRL_L_CHAR},
    {'a', 'A'},
    {'s', 'B'},
    {'d', 'C'},
    {'f', 'F'},
    {'g', 'G'},
    {'h', 'H'},
    {'j', 'J'},
    {'k', 'K'},
    {'l', 'L'},
    {';', ':'},
    {'\'', '"'},
    {'`', '~'},
    {SHIFT_L_CHAR, SHIFT_L_CHAR},
    {'\\', '|'},
    {'z', 'Z'},
    {'x', 'X'},
    {'c', 'C'},
    {'v', 'V'},
    {'b', 'B'},
    {'n', 'N'},
    {'m', 'M'},
    {',', '<'},
    {'.', '>'},
    {'/', '?'},
    {SHIFT_R_CHAR, SHIFT_R_CHAR},
    {'*', '*'},
    {ALT_L_CHAR, ALT_L_CHAR},
    {' ', ' '},
    {CAPS_LOCK_CHAR, CAPS_LOCK_CHAR},
    {F1_CHAR, F1_CHAR},
    {F2_CHAR, F2_CHAR},
    {F3_CHAR, F3_CHAR},
    {F4_CHAR, F4_CHAR},
    {F5_CHAR, F5_CHAR},
    {F6_CHAR, F6_CHAR},
    {F7_CHAR, F7_CHAR},
    {F8_CHAR, F8_CHAR},
    {F9_CHAR, F9_CHAR},
    {F10_CHAR, F10_CHAR},
    {F11_CHAR, F11_CHAR},
    {F12_CHAR, F12_CHAR}
};

static void intr_keyboard_handler()
{
    uint8_t scan = inb(KBD_PROT), ascii;
    if(ext_scancode)
    {
        ext_scancode = 0;
        switch(scan)
        {
            case CTRL_R_MAKE & 0x00ff:
                ctrl_status = 1; break;
            case CTRL_R_BREAK & 0x00ff:
                ctrl_status = 0; break;
            case ALT_R_MAKE & 0x00ff:
                alt_status = 1; break;
            case ALT_R_BREAK & 0x00ff:
                alt_status = 0; break;
        }
        return;
    }
    switch(scan)
    {
        case SHIFT_L_MAKE:
        case SHIFT_R_MAKE:
            shift_status = 1; break;
        case SHIFT_L_MAKE + 0x80:
        case SHIFT_R_MAKE + 0x80:
            shift_status = 0; break;

        case CTRL_L_MAKE:
            ctrl_status = 1; break;
        case CTRL_L_MAKE + 0x80:
            ctrl_status = 0; break;

        case ALT_L_MAKE:
            alt_status = 1; break;
        case ALT_L_MAKE + 0x80:
            alt_status = 0; break;

        case CAPS_LOCK_MAKE:
            caps_lock_status ^= 1; break;

        case 0xe0:
            ext_scancode = 1;
            return;

        case F1_MAKE:
        case F2_MAKE:
        case F3_MAKE:
        case F4_MAKE:
        case F5_MAKE:
        case F6_MAKE:
        case F7_MAKE:
        case F8_MAKE:
        case F9_MAKE:
        case F10_MAKE:
        case F11_MAKE:
        case F12_MAKE: break;

        default:
            if(scan == 0 || scan >= sizeof(keymap)/2)
                return;
            ascii = keymap[scan][shift_status];
            if(ascii == CHAR_INVISIBLE)
                return;
            if((ascii >= 'a' && ascii <= 'z') || (ascii >='A' && ascii <= 'Z'))
                ascii = keymap[scan][shift_status ^ caps_lock_status];
            if(!ioq_full(&kbd_buf))
                ioq_putchar(&kbd_buf, ascii);
            break;
    }
}

void keyboard_init()
{
    put_str("keyboard_init start\n");
    ioqueue_init(&kbd_buf);
    register_handler(0x21, intr_keyboard_handler);
    put_str("keyboard_init done\n");
}