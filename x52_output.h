#ifndef X52_OUTPUT_H
#define X52_OUTPUT_H

#include <stdint.h>
#include <windows.h>
#include <qvector.h>

class x52_output
{
public:
    void init(void* hDevice, DWORD dwPage, const wchar_t * name);
    void deInit(void);
    void intro_msg(void* hDevice, DWORD dwPage);
    void writeLine(void* hDevice, DWORD dwPage, uint8_t line, const wchar_t * txt);
    void clear_led(void * hDevice, DWORD dwPage);
    void setOneColor_led(void * hDevice, DWORD dwPage, bool green, bool red);
    void color_led(void * hDevice, DWORD dwPage, uint8_t nr, int8_t param);
    void color_led_flash(void * hDevice, DWORD dwPage, uint8_t nr, int8_t param);
    void x52_flash(void * hDevice, DWORD dwPage);
    void x52Flash(uint8_t nr, uint8_t intervall, uint8_t colour);
    void setString(void * hDevice, DWORD dwPage, DWORD dwIndex, DWORD cchValue, const wchar_t* wszValue);
private:
    QVector<QStringList> flash_intervall{{"0", "0", "0", "0", "0", "0", "0", "0", "0", "0", "0", "0"},  //flash intervals
                                         {"1", "0", "1", "0", "1", "0", "1", "0", "1", "0", "1", "0"},
                                         {"1", "1", "0", "0", "1", "1", "0", "0", "1", "1", "0", "0"},
                                         {"1", "1", "1", "1", "1", "1", "1", "1", "0", "0", "0", "0"},};
    QVector<int8_t> LEDflash {0,0,0,0,0,0,0,0,0,0,0}; //flashFlag + flash interval number
    QVector<int8_t> LEDflashColour {0,0,0,0,0,0,0,0,0,0,0}; //flash color
    uint8_t flash_step = 0;
};

#endif // X52_OUTPUT_H
