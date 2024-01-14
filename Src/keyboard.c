#include "keyboard.h"

char buffer[257] = {0};
int caps = 0;
int buffer_index = 0;
int shift = 0;

void printLetter(uint8_t scancode) {
    char ch = 0;
    switch (scancode) {
        // numbers and symbols
        // checking for shift to change the meaning of the key pressed
        case 0x2:
            if(shift) ch = '!'; else ch = '1'; 
            break;
        case 0x3:
            if(shift) ch = '@'; else ch = '2';
            break;
        case 0x4:
            if(shift) ch = '#'; else ch = '3';
            break;
        case 0x5:
            if(shift) ch = '$'; else ch = '4';
            break;
        case 0x6:
            if(shift) ch = '%'; else ch = '5';
            break;
        case 0x7:
            if(shift) ch = '^'; else ch = '6';
            break;
        case 0x8:
            if(shift) ch = '&'; else ch = '7';
            break;
        case 0x9:
            if(shift) ch = '*'; else ch = '8';
            break;
        case 0xA:
            if(shift) ch = '('; else ch = '9';
            break;
        case 0xB:
            if(shift) ch = ')'; else ch = '0';
            break;

        // symbols
        case 0xC:
            if(shift) ch = '_'; else ch = '-';
            break;
        case 0xD:
            if(shift) ch = '+'; else ch = '=';
            break;
        case 0x1A:
            if(shift) ch = '{'; else ch = '[';
            break;
        case 0x1B:
            if(shift) ch = '}'; else ch = ']';
            break;
        case 0x27:
            if(shift) ch = ':'; else ch = ';';
            break;
        case 0x28:
            if(shift) ch = '"'; else ch = '\'';
            break;
        case 0x29:
            if(shift) ch = '~'; else ch = '`';
            break;
        case 0x2B:
            if(shift) ch = '|'; else ch = '\\';
            break;
        case 0x33:
            if(shift) ch = '<'; else ch = ',';
            break;
        case 0x34:
            if(shift) ch = '>'; else ch = '.';
            break;
        case 0x35:
            if(shift) ch = '?'; else ch = '/';
            break;

        // letters
        // checking for caps or shift press to see if the user wanted capital letters
        case 0x10:
            if(caps || shift) ch = 'Q'; else ch = 'q';
            break;
        case 0x11:
            if(caps || shift) ch = 'W'; else ch = 'w';
            break;
        case 0x12:
            if(caps || shift) ch = 'E'; else ch = 'e';
            break;
        case 0x13:
            if(caps || shift) ch = 'R'; else ch = 'r';
            break;
        case 0x14:
            if(caps || shift) ch = 'T'; else ch = 't';
            break;
        case 0x15:
            if(caps || shift) ch = 'Y'; else ch = 'y';
            break;
        case 0x16:
            if(caps || shift) ch = 'U'; else ch = 'u';
            break;
        case 0x17:
            if(caps || shift) ch = 'I'; else ch = 'i';
            break;
        case 0x18:
            if(caps || shift) ch = 'O'; else ch = 'o';
            break;
        case 0x19:
            if(caps || shift) ch = 'P'; else ch = 'p';
            break;
        case 0x1E:
            if(caps || shift) ch = 'A'; else ch = 'a';
            break;
        case 0x1F:
            if(caps || shift) ch = 'S'; else ch = 's';
            break;
        case 0x20:
            if(caps || shift) ch = 'D'; else ch = 'd';
            break;
        case 0x21:
            if(caps || shift) ch = 'F'; else ch = 'f';
            break;
        case 0x22:
            if(caps || shift) ch = 'G'; else ch = 'g';
            break;
        case 0x23:
            if(caps || shift) ch = 'H'; else ch = 'h';
            break;
        case 0x24:
            if(caps || shift) ch = 'J'; else ch = 'j';
            break;
        case 0x25:
            if(caps || shift) ch = 'K'; else ch = 'k';
            break;
        case 0x26:
            if(caps || shift) ch = 'L'; else ch = 'l';
            break;
        case 0x2C:
            if(caps || shift) ch = 'Z'; else ch = 'z';
            break;
        case 0x2D:
            if(caps || shift) ch = 'X'; else ch = 'x';
            break;
        case 0x2E:
            if(caps || shift) ch = 'C'; else ch = 'c';
            break;
        case 0x2F:
            if(caps || shift) ch = 'V'; else ch = 'v';
            break;
        case 0x30:
            if(caps || shift) ch = 'B'; else ch = 'b';
            break;
        case 0x31:
            if(caps || shift) ch = 'N'; else ch = 'n';
            break;
        case 0x32:
            if(caps || shift) ch = 'M'; else ch = 'm';
            break;

        // special keys
        case 0x1C: // enter
            kprintf("\n%s\n", buffer);
            for(int i = 0; i < 256; i++)
                buffer[i] = 0; // cleaning buffer
            buffer_index = 0;
            break;
        case 0x3A: // capslock
            caps = (caps + 1) % 2;
            break;
        case 0xAA: // left shift key up
            shift = 0;
            break;
        case 0xB6: // right shift key up
            shift = 0;
            break;
        case 0x2A: // left shift
            shift = 1;
            break;
        case 0x36: // right shift
            shift = 1;
            break;
        case 0x39: // space
            ch = ' ';
            break;
        case 0x0E: // backspace
            if(buffer_index)
            {
                put_char(0x08);
                put_char(' ');
                put_char(0x08);
                buffer[--buffer_index] = 0; // removing letter from buffer
            }              
            break;
        case 0x0F: // tab
            put_char(0x09);
            break;
        case 0x48: // arrow up
            printString("Arrow Up");
            break;
        case 0x4B: // arrow left
            printString("Arrow Left");
            break;
        case 0x50: // arrow down
            printString("Arrow Down");
            break;
        case 0x4D: // arrow right
            printString("Arrow Right");
            break;
        case 0x0: // error
            printString("ERROR");
            break;
        case 0x1: // esc
            printString("ESC");
            break;
        
    }
    if (ch) // checking if a key needed to be printed
    {
        buffer[buffer_index] = ch;
        put_char(ch);
        buffer_index++;
    }
}

static void keyboard_callback(registers_t* regs) {
    uint8_t scancode = port_byte_in(0x60);
    printLetter(scancode);
}

void init_keyboard() 
{
    register_handler(IRQ1, keyboard_callback);
}