#include <iostream>
#include <QDebug>
#include "x52_output.h"
#include "DirectOutput.h"
#include "leds.h"

std::vector<void*> devices;

void __stdcall DirectOutput_Device_Callback(void* hDevice, bool bAdded, void* pvContext) {
    if (bAdded) {
        devices.push_back(hDevice);
        qDebug() << "DeviceCallback: " << pvContext;
    }
    else {
        qDebug() << "DeviceCallback: " << pvContext;
    }
}

void __stdcall DirectOutput_Enumerate_Callback(void* hDevice, void* pvContext) {
    devices.push_back(hDevice);
    qDebug() << "Enumerate: " << pvContext;
}

/*void __stdcall DirectOutput_SoftButton_Callback(void* hDevice, DWORD dwButtons, void* pvContext){  //przyciski joysticka - NIE DZIALA
    if (dwButtons == SoftButton_Up) {
        qDebug() << "ble1";
    }
    else if (dwButtons == SoftButton_Down) {
        qDebug() << "ble2";
    }
    else if (dwButtons == SoftButton_Select) {
        qDebug() << "ble1";
    }
}*/

void x52_output::init(const wchar_t * name){
    DirectOutput_Initialize(name);
    DirectOutput_RegisterDeviceCallback(*DirectOutput_Device_Callback, nullptr);
    DirectOutput_Enumerate(*DirectOutput_Enumerate_Callback, nullptr);
}

void x52_output::deInit(void){
    DirectOutput_Deinitialize();
}

void x52_output::addPage(void* hDevice, DWORD dwPage){
    const wchar_t * pageDebugName = L"X52_page";
    DirectOutput_AddPage(hDevice, dwPage, pageDebugName, FLAG_SET_AS_ACTIVE);
    intro_msg(hDevice, dwPage);
}

void x52_output::intro_msg(void* hDevice, DWORD dwPage)
{
    const wchar_t * txt = L"Saitek X52 plugin";
    int txtsize = std::wcslen (txt);
    DirectOutput_SetString(hDevice, dwPage, 0, txtsize, txt);
    txt = L"  created by";
    txtsize = std::wcslen (txt);
    DirectOutput_SetString(hDevice, dwPage, 1, txtsize, txt);
    txt = L"     KoSik";
    txtsize = std::wcslen (txt);
    DirectOutput_SetString(hDevice, dwPage, 2, txtsize, txt);
}

void x52_output::writeLine(void* hDevice, DWORD dwPage, uint8_t line, const wchar_t * txt)
{
    int txtsize = std::wcslen (txt);
    DirectOutput_SetString(hDevice, dwPage, line, txtsize, txt);
}

void x52_output::clear_led(void * hDevice, DWORD dwPage){
    DirectOutput_SetLed(hDevice, dwPage, LED_FIRE, 0);
    DirectOutput_SetLed(hDevice, dwPage, LED_FIRE_A_GREEN, 0);
    DirectOutput_SetLed(hDevice, dwPage, LED_FIRE_A_RED, 0);
    DirectOutput_SetLed(hDevice, dwPage, LED_FIRE_B_GREEN, 0);
    DirectOutput_SetLed(hDevice, dwPage, LED_FIRE_B_RED, 0);
    DirectOutput_SetLed(hDevice, dwPage, LED_FIRE_D_GREEN, 0);
    DirectOutput_SetLed(hDevice, dwPage, LED_FIRE_D_RED, 0);
    DirectOutput_SetLed(hDevice, dwPage, LED_FIRE_E_GREEN, 0);
    DirectOutput_SetLed(hDevice, dwPage, LED_FIRE_E_RED, 0);
    DirectOutput_SetLed(hDevice, dwPage, LED_TOGGLE_1_2_RED, 0);
    DirectOutput_SetLed(hDevice, dwPage, LED_TOGGLE_1_2_GREEN, 0);
    DirectOutput_SetLed(hDevice, dwPage, LED_TOGGLE_3_4_RED, 0);
    DirectOutput_SetLed(hDevice, dwPage, LED_TOGGLE_3_4_GREEN, 0);
    DirectOutput_SetLed(hDevice, dwPage, LED_TOGGLE_5_6_GREEN, 0);
    DirectOutput_SetLed(hDevice, dwPage, LED_TOGGLE_5_6_RED, 0);
    DirectOutput_SetLed(hDevice, dwPage, LED_CLUTCH_GREEN, 0);
    DirectOutput_SetLed(hDevice, dwPage, LED_CLUTCH_RED, 0);
    DirectOutput_SetLed(hDevice, dwPage, LED_POV_2_RED, 0);
    DirectOutput_SetLed(hDevice, dwPage, LED_POV_2_GREEN, 0);
    DirectOutput_SetLed(hDevice, dwPage, LED_THROTTLE, 0);
}

void x52_output::setOneColor_led(void * hDevice, DWORD dwPage, bool green, bool red){
    DirectOutput_SetLed(hDevice, dwPage, LED_FIRE, green);
    DirectOutput_SetLed(hDevice, dwPage, LED_FIRE_A_GREEN, green);
    DirectOutput_SetLed(hDevice, dwPage, LED_FIRE_A_RED, red);
    DirectOutput_SetLed(hDevice, dwPage, LED_FIRE_B_GREEN, green);
    DirectOutput_SetLed(hDevice, dwPage, LED_FIRE_B_RED, red);
    DirectOutput_SetLed(hDevice, dwPage, LED_FIRE_D_GREEN, green);
    DirectOutput_SetLed(hDevice, dwPage, LED_FIRE_D_RED, red);
    DirectOutput_SetLed(hDevice, dwPage, LED_FIRE_E_GREEN, green);
    DirectOutput_SetLed(hDevice, dwPage, LED_FIRE_E_RED, red);
    DirectOutput_SetLed(hDevice, dwPage, LED_TOGGLE_1_2_RED, red);
    DirectOutput_SetLed(hDevice, dwPage, LED_TOGGLE_1_2_GREEN, green);
    DirectOutput_SetLed(hDevice, dwPage, LED_TOGGLE_3_4_RED, red);
    DirectOutput_SetLed(hDevice, dwPage, LED_TOGGLE_3_4_GREEN, green);
    DirectOutput_SetLed(hDevice, dwPage, LED_TOGGLE_5_6_GREEN, green);
    DirectOutput_SetLed(hDevice, dwPage, LED_TOGGLE_5_6_RED, red);
    DirectOutput_SetLed(hDevice, dwPage, LED_CLUTCH_GREEN, green);
    DirectOutput_SetLed(hDevice, dwPage, LED_CLUTCH_RED, red);
    DirectOutput_SetLed(hDevice, dwPage, LED_POV_2_RED, red);
    DirectOutput_SetLed(hDevice, dwPage, LED_POV_2_GREEN, green);
    DirectOutput_SetLed(hDevice, dwPage, LED_THROTTLE, green);
}

void x52_output::color_led(void * hDevice, DWORD dwPage, uint8_t nr, int8_t param){  // kolor 0 - czerwony, 1-zielony, 2-pomaranczowy, -1 - wylaczona
    if(nr==LED_FIRE || nr==LED_THROTTLE){
        if(param<=0){ //wylaczony
            DirectOutput_SetLed(hDevice, dwPage, nr, 0); //green
        } else {
            DirectOutput_SetLed(hDevice, dwPage, nr, 1); //green
        }
    } else {
        if(param==0){ //wylaczony
            DirectOutput_SetLed(hDevice, dwPage, nr, 1);  //red
            DirectOutput_SetLed(hDevice, dwPage, nr+1, 0); //green
        } else if (param==1){ //wlaczony
            DirectOutput_SetLed(hDevice, dwPage, nr, 0);  //red
            DirectOutput_SetLed(hDevice, dwPage, nr+1, 1); //green
        } else if (param==-1){ //diody wylaczone
            DirectOutput_SetLed(hDevice, dwPage, nr, 0);  //red
            DirectOutput_SetLed(hDevice, dwPage, nr+1, 0); //green
        }else { //stan posredni
            DirectOutput_SetLed(hDevice, dwPage, nr, 1);  //red
            DirectOutput_SetLed(hDevice, dwPage, nr+1, 1); //green
        }
    }

}

void x52_output::color_led_flash(void * hDevice, DWORD dwPage, uint8_t nr, int8_t param){  //kolor 0 - wylaczony, 1 - czerw, 2-zielony, 3-pomaranczowy
    qDebug() << "!: " << flash_intervall[2];
    if(nr==LED_FIRE || nr==LED_THROTTLE){
        if(param<=0){ //wylaczony
            DirectOutput_SetLed(hDevice, dwPage, nr, 0); //green
        } else {
            DirectOutput_SetLed(hDevice, dwPage, nr, 1); //green
        }
    } else {
        if(param==0){ //wylaczony
            DirectOutput_SetLed(hDevice, dwPage, nr, 0);  //red
            DirectOutput_SetLed(hDevice, dwPage, nr+1, 0); //green
        } else if (param==1){ //wlaczony
            DirectOutput_SetLed(hDevice, dwPage, nr, 1);  //red
            DirectOutput_SetLed(hDevice, dwPage, nr+1, 0); //green
        } else if (param==2){ //diody wylaczone
            DirectOutput_SetLed(hDevice, dwPage, nr, 0);  //red
            DirectOutput_SetLed(hDevice, dwPage, nr+1, 1); //green
        }else { //stan posredni
            DirectOutput_SetLed(hDevice, dwPage, nr, 1);  //red
            DirectOutput_SetLed(hDevice, dwPage, nr+1, 1); //green
        }
    }

}


void x52_output::x52_flash(void * hDevice, DWORD dwPage){
    for(uint8_t i=0; i<LEDflash.capacity(); i++)
        {
            if(LEDflash.at(i) > 0)
            {
                if(flash_intervall[LEDflash[i]].at(flash_step) == "1"){
                    color_led_flash(hDevice, dwPage, i, LEDflashColour.at(i));
                } else {
                     color_led_flash(hDevice, dwPage, i, 0);
                }
            }
        }
    flash_step++;
    if(flash_step > 11)
        {
        flash_step = 0;
        }
}

void x52_output::x52Flash(uint8_t nr, uint8_t intervall, uint8_t colour){
    if(nr == 0 || nr == 19){ //for one color buttons
        if(colour != 0){
            LEDflashColour[nr] = 1;
        } else {
            LEDflashColour[nr] = 0;
        }
    } else {
        LEDflashColour[nr] = colour;
    }
    LEDflash[nr] = intervall;
}

void x52_output::setString(void * hDevice, DWORD dwPage, DWORD dwIndex, DWORD cchValue, const wchar_t* wszValue){
    DirectOutput_SetString(hDevice, dwPage, dwIndex, cchValue, wszValue);
}
//------------------------------------------------------------------------------------------------------------------
/*class LedException : public : std::exception {
public:
    explicit LedException(HRESULT result)
        : mResult{ result }
    {
    }
    virtual const char* what() const noexcept override
    {
        if (mDesc.empty()) {
            makeDesc();
        }
        return mDesc.c_str();
    }

private:
    void makeDesc()
    {
        switch (mResult) {
        case E_HANDLE:
            mDesc = "hDevice is not a valid device handle";
            break;
        case E_NOTIMPL:
            mDesc = "hDevice does not have any leds";
            break;
        case E_INVALIDARG:
            mDesc = "dwPage or dwIndex is not a valid id";
            break;
        case E_PAGENOTACTIVE:
            mDesc = "dwPage is not the active page";
            break;
        default:
            mDesc = std::to_string(mResult);
        }
    }

private:
    mutable std::string mDesc;
    HRESULT mResult;
};

class ILedDevice {
public:
    virtual ~ILedDevice() {}
    virtual void SetLed(DWORD dwPage, DWORD dwIndex, DWORD dwValue) = 0;
};

class LedDevice : public ILedDevice final {
public:
    ~LedDevice();

    LedDevice( ...... ) {
         mDevice = ......;
    }

    static void checkForErrors(HRESULT result) const
    {
        if (result == S_OK) return;
        throw LedException{result};
    }

    void SetLed(DWORD dwPage, DWORD dwIndex, DWORD dwValue) override
    {
        checkForErrors(DirectOutput_SetLed(mDevice, dwPage, dwIndex, dwValue));
    }

private:
    void* mDevice;
};

...
// pole w klasie x52_output
// ILedDevice* mLed;
void x52_output::color_led(void * hDevice, DWORD dwPage, uint8_t nr, int8_t param){
    if(nr==LED_FIRE || nr==LED_THROTTLE){
        if(param<=0){ //wylaczony
            mLed->SetLed(dwPage, nr, 0); //green
        } else {
            mLed->SetLed(dwPage, nr, 1); //green
        }
    } else {
        if(param==0){ //wylaczony
            mLed->SetLed(dwPage, nr, 1);  //red
            mLed->SetLed(dwPage, nr+1, 0); //green
        } else if (param==1){ //wlaczony
            mLed->SetLed(dwPage, nr, 0);  //red
            mLed->SetLed(dwPage, nr+1, 1); //green
        } else if (param==-1){ //diody wylaczone
            mLed->SetLed(dwPage, nr, 0);  //red
            mLed->SetLed(dwPage, nr+1, 0); //green
        }else { //stan posredni
            mLed->SetLed(dwPage, nr, 1);  //red
            mLed->SetLed(dwPage, nr+1, 1); //green
        }
    }
}
*/
