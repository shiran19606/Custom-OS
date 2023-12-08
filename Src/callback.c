#include "callback.h"
#include "Screen.h"
#include "ports.h"
#include "isr.h"

char buffer[257] = {0};

void printLetter(uint8_t scancode) {
    int caps = 0;
    int buffer_index = 0;
    char ch = 0;
    switch (scancode) {
        case 0x0:
            printString("ERROR");
            break;
        case 0x1:
            printString("ESC");
            break;
        case 0x2:
            ch = '1';
            break;
        case 0x3:
            ch = '2';
            break;
        case 0x4:
            ch = '3';
            break;
        case 0x5:
            ch = '4';
            break;
        case 0x6:
            ch = '5';
            break;
        case 0x7:
            ch = '6';
            break;
        case 0x8:
            ch = '7';
            break;
        case 0x9:
            ch = '8';
            break;
        case 0xA:
            ch = '9';
            break;
        case 0xB:
            ch = '0';
            break;
        case 0xC:
            ch = '-';
            break;
        case 0xD:
            ch = '=';
            break;


        case 0x10:
            if(caps) ch = 'Q'; else ch = 'q';
            break;
        case 0x11:
            if(caps) ch = 'W'; else ch = 'w';
            break;
        case 0x12:
            if(caps) ch = 'E' ; else ch = 'e';
            break;
        case 0x13:
            if(caps) ch = 'R'; else ch = 'r';
            break;
        case 0x14:
            if(caps) ch = 'T'; else ch = 't';
            break;
        case 0x15:
            if(caps) ch = 'Y'; else ch = 'y';
            break;
        case 0x16:
            if(caps) ch = 'U'; else ch = 'u';
            break;
        case 0x17:
            if(caps) ch = 'I'; else ch = 'i';
            break;
        case 0x18:
            if(caps) ch = 'O'; else ch = 'o';
            break;
        case 0x19:
            if(caps) ch = 'P'; else ch = 'p';
            break;
        case 0x1E:
            if(caps) ch = 'A'; else ch = 'a';
            break;
        case 0x1F:
            if(caps) ch = 'S'; else ch = 's';
            break;
        case 0x20:
            if(caps) ch = 'D'; else ch = 'd';
            break;
        case 0x21:
            if(caps) ch = 'F'; else ch = 'f';
            break;
        case 0x22:
            if(caps) ch = 'G'; else ch = 'g';
            break;
        case 0x23:
            if(caps) ch = 'H'; else ch = 'h';
            break;
        case 0x24:
            if(caps) ch = 'J'; else ch = 'j';
            break;
        case 0x25:
            if(caps) ch = 'K'; else ch = 'k';
            break;
        case 0x26:
            if(caps) ch = 'L'; else ch = 'l';
            break;
        case 0x2C:
            if(caps) ch = 'Z'; else ch = 'z';
            break;
        case 0x2D:
            if(caps) ch = 'X'; else ch = 'x';
            break;
        case 0x2E:
            if(caps) ch = 'C'; else ch = 'c';
            break;
        case 0x2F:
            if(caps) ch = 'V'; else ch = 'v';
            break;
        case 0x30:
            if(caps) ch = 'B'; else ch = 'b';
            break;
        case 0x31:
            if(caps) ch = 'N'; else ch = 'n';
            break;
        case 0x32:
            if(caps) ch = 'M'; else ch = 'm';
            break;

        case 0x1C:
            printString(buffer);
            for(int i = 0; i < 256; i++)
                buffer[i] = '\0';
            break;
        case 0x3A:
            printString("Caps Lock");
            caps = (caps + 1) % 2;
            break;
        
        case 0x39:
            printString("Space");
            ch = ' ';
            break;

        default:
            if (scancode <= 0x7f) {
                printString("Unknown key down");
            } else if (scancode <= 0x39 + 0x80) {
                printString("key up ");
                printLetter(scancode - 0x80);
            } else {
                printString("Unknown key up");
            }
            break;
    }
    if (ch)
    {
        buffer[buffer_index] = ch;
        put_char(ch);
        buffer_index++;
    }
}

static void keyboard_callback(struct registers *regs) {
    uint8_t scancode = port_byte_in(0x60);
    printLetter(scancode);
}

void init_keyboard() 
{
    register_handler(IRQ1, keyboard_callback);
}
