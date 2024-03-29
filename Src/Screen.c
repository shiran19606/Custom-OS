#include "Screen.h"

uint16_t* videoMemory = (uint16_t*)(0xB8000 + 0xC0000000);
uint16_t backBuffer[80 * 75] = {(SPACE_ASCII_VALUE | (0x0f << 8))};
uint8_t cur_x = 0;
uint8_t cur_y = 0;
uint8_t bbx = 0;
uint8_t bby = 0;
uint8_t numLinesBb = 0;


void scrollUp()
{
    if(bby >= 25)
    {
        uint8_t attribute = 0x0f;
        uint16_t blankPoint = SPACE_ASCII_VALUE | (attribute << 8);
        for (int i = 0;i<80*25;i++)
            videoMemory[i] = backBuffer[(bby - 25) * 80 + i]; //in each line, we copy the char in the same location in the line above.
        bby--;
    }
}

void scrollDown()
{
    if(numLinesBb >= 25 && bby <= numLinesBb)
    {
        uint8_t attribute = 0x0f;
        uint16_t blankPoint = SPACE_ASCII_VALUE | (attribute << 8);
        for (int i = 0;i<80*25;i++)
            videoMemory[i] = backBuffer[(bby - cur_y) * 80 + i]; //in each line, we copy the char in the same location in the line below.
        bby++;
    }
}

void setX(uint8_t num) {cur_x = num;}
void setY(uint8_t num) {cur_y = num;}
uint8_t getX(){return cur_x;}
uint8_t getY(){return cur_y;}

//function will set the location of the Mouse Cursor, using VGA ports 0x3D4 with is the VGA's command port, and the 0x3D5 which is the VGA's data port.

void setCursorLocation()
{
    uint16_t location = cur_y * 80 + cur_x;
    port_byte_out(VGA_COMMAND_PORT, 14);                  // Tell the VGA board we are setting the high cursor byte.
    port_byte_out(VGA_DATA_PORT, location >> 8); // Send the high cursor byte.
    port_byte_out(VGA_COMMAND_PORT, 15);                  // Tell the VGA board we are setting the low cursor byte.
    port_byte_out(VGA_DATA_PORT, location);      // Send the low cursor byte.
}

//this function checks if we need to scroll, and if so, it will scroll.
//scrolling will just be done by moving each line one line up, and deleting the top one, we will have no scroll bar or console history.
void scrollIfNeeded()
{
    uint8_t attribute = 0x0f;
    uint16_t blankPoint = SPACE_ASCII_VALUE | (attribute << 8);
    int i = 0;
    if (cur_y == 24)
    {
        for (i = 0;i<80*24;i++)
            videoMemory[i] = videoMemory[i+80]; //in each line, we copy the char in the same location in the line below.
        for (i = 0;i<80;i++)
            videoMemory[24*80 + i] = blankPoint;
        cur_x = 0;
        cur_y = 23;
    }

    if(bby == 74)
    {
        for (i = 0;i<80*49;i++)
            backBuffer[i] = backBuffer[i+80]; //in each line, we copy the char in the same location in the line below.
        for (i = 0;i<80;i++);
            backBuffer[80*49 + i] = blankPoint;
        bbx = 0;
        bby = 49;
    }
}
//clearing the screen is basically just printing spaces all over the screen.
void clearScreen()
{
    uint8_t attribute = 0x0f;
    uint16_t blankPoint = SPACE_ASCII_VALUE | (attribute << 8);

    for (int i = 0;i<80*25;i++)
        videoMemory[i] = blankPoint;

    cur_x = 0;
    cur_y = 0;
    setCursorLocation();
}


void put_char(uint8_t charToPrint)
{   
    uint8_t attributes = 0x0f; //0x0f means black background, white char.
    if (charToPrint == BACKSPACE && cur_x) //cur_x means the cur_x is bigger than 0.
    {
        cur_x--;
        bbx--;
    }
    else if (charToPrint == BACKSPACE && !cur_x && cur_y)
    {
        cur_x = 79;
        cur_y--;
        bbx = 79;
        bby--;
        numLinesBb--;
    }
    else if (charToPrint == TAB)
    {
        cur_x = (cur_x+8) & ~(8-1); //advance x to the next location divisable by 8.
        bbx = (bbx+8) & ~(8-1); //advance x to the next location divisable by 8.
    }
    else if (charToPrint == '\r')
    {
        cur_x = 0;
        bbx = 0;
    }
    else if (charToPrint == '\n')
    {
        cur_x = 0;
        cur_y++;
        bbx = 0;
        bby++;
        numLinesBb++;
    }
    else if (charToPrint >= ' ') //check if the char is printable
    {
        videoMemory[cur_y * 80 + cur_x] = ((charToPrint) | (attributes << 8));
        backBuffer[bby * 80 + bbx] = ((charToPrint) | (attributes << 8));
        cur_x++;
        bbx++;
    }
    if (cur_x >=80)
    {
        cur_x = 0;
        cur_y++;
        bbx = 0;
        bby++;
        numLinesBb++;
    }

    scrollIfNeeded();
    setCursorLocation();
}

void printString(const char* str)
{
    while (*str)
    {
        put_char(*str);
        str++;
    }
}

void printNumber(uint32_t number)
{
    if (number == 0)
    {
        put_char('0');
        return;
    }
    if ((number & (1 << 31)) != 0) //if the leftmost bit is on, it means we have a signed negative number.
        put_char('-');
    
    char c[32] = {0};
    int i = 0;
    int32_t temp = number;
    if (temp<0) //if the number is negative, we printed '-' earlier and we will flip the number and work with it as a positive now.
        temp = -1 * temp;
    while (temp > 0)
    {
        c[i] = '0' + temp % 10;
        temp/=10;
        i++;
    }
    //now we need to reverse the array/
    int j = 0;
    while (j<i)
    {
        char tempC = c[j];
        c[j++] = c[--i];
        c[i] = tempC;
    }
    printString(c);
}

void printNumberHex(uint32_t intNumber)
{
    printString("0x");
    if (intNumber == 0)
        put_char('0');
    char c;
    int significant = 0; 
    int i = 0;
    int32_t temp = intNumber;
    for (i = 7; i >= 0; i--) {
        int hexDigit = (temp >> (i * 4)) & 0xF;
        if(hexDigit != 0 || significant) 
        {
            c = (hexDigit < 10) ? ('0' + hexDigit) : ('A' + hexDigit - 10);
            put_char(c);
            significant = 1;
        }
    }
}

void kprintf(const char* format, ...) {
    // Pointer to traverse the format string
    const char* ptr = format;

    // Pointer to the first argument (after format)
    // Here, we're assuming that arguments start immediately after the format string
    // in memory. You'll need to ensure the correct memory layout in your OS.
    void* arg_ptr = (void*)&format + sizeof(format);

    while (*ptr != '\0') {
        if (*ptr == '%') {
            ptr++; // Move to the character after '%'

            // Check format specifiers
            switch (*ptr) {
                case 'c': {
                    char ch = *((char*)arg_ptr);
                    put_char(ch);
                    arg_ptr += sizeof(char);
                    break;
                }
                case 's': {
                    const char* str = *((const char**)arg_ptr);
                    printString(str);
                    arg_ptr += sizeof(const char*);
                    break;
                }
                case 'd':
                case 'i': {
                    uint32_t num = *((uint32_t*)arg_ptr);
                    printNumber(num);
                    arg_ptr += sizeof(uint32_t);
                    break;
                }
                case 'x': {
                    uint32_t hex_num = *((uint32_t*)arg_ptr);
                    printNumberHex(hex_num);
                    arg_ptr += sizeof(uint32_t);
                    break;
                }
                case '%':
                {
                    put_char('%');
                }
                default:
                    // Unsupported format specifier
                    break;
            }
        } else {
            // If not a format specifier, just print the character
            put_char(*ptr);
        }

        ptr++; // Move to the next character in the format string
    }
}