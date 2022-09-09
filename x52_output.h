#ifndef X52_OUTPUT_H
#define X52_OUTPUT_H

#include <stdint.h>
#include <windows.h>
#include <qvector.h>

class x52_output
{
public:
    void init(void* hDevice, DWORD dwPage);
    void intro_msg(void* hDevice, DWORD dwPage);
    void writeLine(void* hDevice, DWORD dwPage, uint8_t line, const wchar_t * txt);
    void clear_led(void * hDevice, DWORD dwPage);
    void setOneColor_led(void * hDevice, DWORD dwPage, bool green, bool red);
    void color_led(void * hDevice, DWORD dwPage, uint8_t nr, int8_t param);
    void color_led_flash(void * hDevice, DWORD dwPage, uint8_t nr, int8_t param);
};

#endif // X52_OUTPUT_H
