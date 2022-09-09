#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QList>
#include <QDateTime>
#include <QTime>
#include <QSignalMapper>
#include <QTimer>
extern "C" {
    #include "SimConnect.h"
}

#include "x52_output.h"
#include "DirectOutput.h"

#include "structures_simconnect.h"
#include <QtXml>
#include <QFile>
#include <QPixmap>
#include <QInputDialog>
#include <QMessageBox>
#include <QSpacerItem>
#include <QGridLayout>

QStringList LEDS_FUNCTIONS = {
    "RED",
    "GREEN",
    "ORANGE",
    "GEAR",
    "BRAKE",
    "PARKING_BRAKE",
    "PARK_BRK_COMBO",
    "BRAKES_COMBO",
    "RPM_ENGINE_1",
    "RPM_ENGINE_2",
    "FLAPS",
    "LANDING_LIGHTS",
    "AP_MASTER",
    "REV_THRUST",
    "TAXI_LIGHTS",
    "NAV_LIGHTS",
    "BEACON_LIGHTS",
    "STROBE_LIGHTS",
    "PANEL_LIGHTS",
    "LIGHTS_COMBO",
    "STALL_WARNING",
    "OVERSPEED_WARNING",
    "MARKERS",
    "INNER_MARKER",
    "MIDDLE_MARKER",
    "OUTER_MARKER",
    "SPOILERS_ARMED",
    "SPOILERS",
    "AP_HDG_LOCK",
    "AP_NAV1_LOCK",
    "AP_ALT_LOCK",
    "AP_APP_LOCK",
    "AUTOTHROTTLE_LOCK",
    "AP_COURSE_COMBO",
};

QStringList MFD_FUNCTIONS = {
    "SPD",
    "ALT",
    "V/S",
    "HDG",
    "AP_HDG",
    "AP_ALT",
    "AP_V/S",
    "AP_SPD",
    "AP_CRS1",
    "AP_CRS2"
};


uint8_t autocon_time = 0; //przechwanie sekund do autoconnect
#define MFD_REF_TIME 100 //czas odswiezania MFD

uint8_t flash_step = 0;
QVector<QStringList> flash_intervall{{"0", "0", "0", "0", "0", "0", "0", "0", "0", "0", "0", "0"},  //interwały błysków
                                     {"1", "0", "1", "0", "1", "0", "1", "0", "1", "0", "1", "0"},
                                     {"1", "1", "0", "0", "1", "1", "0", "0", "1", "1", "0", "0"},
                                     {"1", "1", "1", "1", "1", "1", "1", "1", "0", "0", "0", "0"},};
QVector<int8_t> LEDflash {0,0,0,0,0,0,0,0,0,0,0}; //flaga czy miga i jednoczesnie numer interwału
QVector<int8_t> LEDflashColour {0,0,0,0,0,0,0,0,0,0,0}; //kolor migania LED
// ------------ DANE DLA X52 --------------------------------------------------------
x52_output x52output;
DWORD dwPage = 1;
std::vector<void*> devices;
QVector<QString> mfdLine {"AP_CRS1", "AP_HDG", "AP_VS", ""};
QVector<QString> diody {"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""};
std::map<QString, int> dek {{"LED_FIRE", 0}, {"LED_FIRE_A", 1}, {"LED_FIRE_B", 3}, {"LED_FIRE_D", 5}, {"LED_FIRE_E", 7}, {"LED_TOGGLE_1_2", 9}, {"LED_TOGGLE_3_4", 11}, {"LED_TOGGLE_5_6", 13}, {"LED_POV_2", 15}, {"LED_CLUTCH", 17}, {"LED_THROTTLE", 19}};

// -----------  DANE DLA SIMCONNECT --------------------------------------------------
HANDLE hSimConnect = NULL;
HANDLE hSimConnectLVAR = NULL;

SIMCONNECT_CLIENT_DATA_ID ClientDataID = 1; // dla LVAR
SIMCONNECT_OBJECT_ID objectID = SIMCONNECT_OBJECT_ID_USER; // dla LVAR
SIMCONNECT_RECV* pDataLVAR;

HRESULT hr;
SIMCONNECT_RECV* pData = NULL;
DWORD cbData = 0;
SIMCONNECT_RECV_SIMOBJECT_DATA* pObjData = NULL;


//--------- DEKLARACJE FUNKCJI ----------------------------------------------------------
void XMLretrievPlanes(Ui::MainWindow *ui_pointer, QString tag, QString att);
void XMLretrievButtons(Ui::MainWindow *ui_pointer, QString tag, QString arg1);
void XMLsetAttribute(QDomDocument doc, QDomElement btns, QString name, QString func, uint8_t rodzaj);
void SetLed(Ui::MainWindow *ui_pointer);


#define STATUSBAR_TIMEOUT 3000 // x * 1000 = sekundy
//---------------------------------------------------------
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
//----------------------------------------------------------------------------------------------------------------------------
void MainWindow::x52_flash(void)
{
for(uint8_t i=0; i<LEDflash.capacity(); i++)
    {
        if(LEDflash.at(i) > 0) //jesli flaga 1 do blysku
        {
            if(flash_intervall[LEDflash[i]].at(flash_step) == "1"){
                x52output.color_led_flash(devices[0], dwPage, i, LEDflashColour.at(i));
            } else {
                 x52output.color_led_flash(devices[0], dwPage, i, 0);
            }
        }
    }
flash_step++;
if(flash_step > 11){
    flash_step = 0;
}
}

void x52Flash(uint8_t nr, uint8_t intervall, uint8_t colour){
    if(nr == 0 || nr == 19){ //dla przycisków jednokolorowych
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


void MainWindow::SetLed(Ui::MainWindow *ui_pointer){
    SimConnect_DataRefs* SimConnect_Data = NULL;
    SimConnect_Data = (SimConnect_DataRefs*)&pObjData->dwData;
    //  ---- dla LVAR ----------------
    //WASM_FCU_DataRefs* pDataRefs_WASM_FCU = NULL;
    //SIMCONNECT_RECV_CLIENT_DATA* pObjData = (SIMCONNECT_RECV_CLIENT_DATA*)pDataLVAR;
    //pDataRefs_WASM_FCU = (WASM_FCU_DataRefs*)&pObjData->dwData;


    QString bufor = SimConnect_Data->aircraft_title;
    if(bufor.length() > 5){
        if(bufor != ui_pointer->AircraftTitle_label->text()){
            ui_pointer->AircraftTitle_label->setText(bufor);
            ui_pointer->statusbar->showMessage("Airplane has been changed", STATUSBAR_TIMEOUT);
            ui_pointer->profil_comboBox->setCurrentIndex(0);
            for(uint16_t i=0; i<ui_pointer->profil_comboBox->count(); i++){
                if(bufor.contains(ui_pointer->profil_comboBox->itemText(i)))
                {
                    ui->statusbar->showMessage("Profile found", STATUSBAR_TIMEOUT);
                    ui->profil_comboBox->setCurrentIndex(i);
                    QString nazwa = ui->profil_comboBox->currentText();
                    const wchar_t *array = (const wchar_t *)nazwa.utf16();
                    TimerMFDrefresh->stop();
                    x52output.writeLine(devices[0], dwPage, 0, array);
                    x52output.writeLine(devices[0], dwPage, 1, (wchar_t*)L"");
                    x52output.writeLine(devices[0], dwPage, 2, (wchar_t*)L"");
                    delay(1500);
                    TimerMFDrefresh->start(MFD_REF_TIME);
                    break;
                }
            }
            XMLretrievButtons(ui_pointer, "plane", ui_pointer->profil_comboBox->currentText());
        }
    }

    //if(SimConnect_Data->batt_master > 10){  //jesli bateria wlaczona
        uint8_t i;
        for(i=0;i<19;i++){  //bo numer ostatniego led to 19
            if (diody[i] == "RED"){
                x52output.color_led(devices[0], dwPage, i,0);
            } else if (diody[i] == "GREEN"){
                x52output.color_led(devices[0], dwPage, i,1);
            } else if (diody[i] == "ORANGE"){
                x52output.color_led(devices[0], dwPage, i,2);
            } else if(diody[i] == "GEAR"){
                if(SimConnect_Data->gear >0 && SimConnect_Data->gear <= 99){  //GEAR
                    x52output.color_led(devices[0], dwPage, i,2);
                } else if (SimConnect_Data->gear == 100){
                    x52output.color_led(devices[0], dwPage, i,1);
                } else {
                    x52output.color_led(devices[0], dwPage, i,0);
                }
            } else if (diody[i] == "BRAKE"){
                x52output.color_led(devices[0], dwPage, i,SimConnect_Data->brake); //NORMAL BRAKE
            } else if (diody[i] == "PARKING_BRAKE"){
                x52output.color_led(devices[0], dwPage, i,SimConnect_Data->parkingbrake); //PARKING BRAKE
            }else if (diody[i] == "PARK_BRK_COMBO"){
                if(SimConnect_Data->parkingbrake != 0){
                    x52Flash(i, 3, 1);
                } else {
                    x52Flash(i, 0, 0);
                    x52output.color_led_flash(devices[0], dwPage, i, 0);
                }
            } else if (diody[i] == "BRAKES_COMBO"){
                if(SimConnect_Data->brake != 0 && SimConnect_Data->parkingbrake ==0){
                    x52output.color_led(devices[0], dwPage, i, 1);
                    x52Flash(i, 0, 0);
                } else if(SimConnect_Data->parkingbrake !=0){
                    x52Flash(i, 3, 1);
                } else {
                    x52Flash(i, 0, 0);
                    x52output.color_led_flash(devices[0], dwPage, i, 0);
                }
            }  else if (diody[i] == "RPM_ENGINE_1"){
                if(SimConnect_Data->enginerpm1 >500){  //ENGINE 1 RPM
                    x52output.color_led(devices[0], dwPage, i,1);
                } else {
                    x52output.color_led(devices[0], dwPage, i,0);
                }
            } else if (diody[i] == "RPM_ENGINE_2"){
                if(SimConnect_Data->enginerpm2 >500){  //ENGINE 2 RPM
                    x52output.color_led(devices[0], dwPage, i,1);
                } else {
                    x52output.color_led(devices[0], dwPage, i,0);
                }
            } else if (diody[i] == "FLAPS"){
                if(SimConnect_Data->flaps >=10 && SimConnect_Data->flaps < 90){ //FLAPS
                    x52output.color_led(devices[0], dwPage, i,2);
                } else if (SimConnect_Data->flaps >=90){
                    x52output.color_led(devices[0], dwPage, i,1);
                } else {
                    x52output.color_led(devices[0], dwPage, i,0);
                }
            }  else if (diody[i] == "LANDING_LIGHTS"){
                x52output.color_led(devices[0], dwPage, i,SimConnect_Data->lightlanding);
            } else if (diody[i] == "AP_MASTER"){
                x52output.color_led(devices[0], dwPage, i,SimConnect_Data->ap_master);
            } else if (diody[i] == "REV_THRUST"){
                if(SimConnect_Data->rev < 0){ //REVERSE THRUST
                    x52output.color_led(devices[0], dwPage, i,0);
                } else {
                    x52output.color_led(devices[0], dwPage, i,1);
                }
            } else if (diody[i] == "TAXI_LIGHTS"){
                x52output.color_led(devices[0], dwPage, i,SimConnect_Data->light_taxi);
            }  else if (diody[i] == "NAV_LIGHTS"){
                x52output.color_led(devices[0], dwPage, i,SimConnect_Data->light_nav);
            } else if (diody[i] == "BEACON_LIGHTS"){
                x52output.color_led(devices[0], dwPage, i,SimConnect_Data->light_beacon);
            } else if (diody[i] == "STROBE_LIGHTS"){
                x52output.color_led(devices[0], dwPage, i,SimConnect_Data->light_strobe);
            } else if (diody[i] == "PANEL_LIGHTS"){
                x52output.color_led(devices[0], dwPage, i,SimConnect_Data->light_panel);
            } else if (diody[i] == "LIGHTS_COMBO"){
                if(SimConnect_Data->light_taxi == 1 && SimConnect_Data->lightlanding == 1){
                    x52output.color_led(devices[0], dwPage, i,0);
                } else if (SimConnect_Data->light_taxi == 1 && SimConnect_Data->lightlanding == 0){
                    x52output.color_led(devices[0], dwPage, i,2);
                } else if (SimConnect_Data->light_taxi == 0 && SimConnect_Data->lightlanding == 1){
                    x52output.color_led(devices[0], dwPage, i,1);
                } else {
                    x52output.color_led(devices[0], dwPage, i,-1);
                }
            } else if (diody[i] == "STALL_WARNING"){
                if(SimConnect_Data->stall_wrn == 1){
                    x52Flash(i, 1, 1);
                } else {
                    x52Flash(i, 0, 0);
                    x52output.color_led(devices[0], dwPage, i,-1);
                }
            } else if (diody[i] == "OVERSPEED_WARNING"){
                if(SimConnect_Data->overspeed_wrn == 1){
                    x52Flash(i, 1, 1);
                } else {
                    x52Flash(i, 0, 0);
                    x52output.color_led(devices[0], dwPage, i,-1);
                }
            } else if (diody[i] == "MARKERS"){
                if(SimConnect_Data->inner_marker == 1){
                    x52Flash(i, 1, 1);
                } else if(SimConnect_Data->middle_marker == 1){
                    x52Flash(i, 1, 3);
                } else if(SimConnect_Data->outer_marker == 1){
                    x52Flash(i, 1, 2);
                } else {
                    x52Flash(i, 0, 0);
                    x52output.color_led(devices[0], dwPage, i, -1);
                }
            } else if (diody[i] == "INNER_MARKER"){
                if(SimConnect_Data->inner_marker == 1){
                    x52Flash(i, 1, 3);
                } else {
                    x52Flash(i, 0, 0);
                    x52output.color_led(devices[0], dwPage, i,-1);
                }
            } else if (diody[i] == "MIDDLE_MARKER"){
                if(SimConnect_Data->middle_marker == 1){
                    x52Flash(i, 1, 3);
                } else {
                    x52Flash(i, 0, 0);
                    x52output.color_led(devices[0], dwPage, i,-1);
                }
            } else if (diody[i] == "OUTER_MARKER"){
                if(SimConnect_Data->outer_marker == 1){
                    x52Flash(i, 1, 3);
                } else {
                    x52Flash(i, 0, 0);
                    x52output.color_led(devices[0], dwPage, i,-1);
                }
            } else if (diody[i] == "SPOILERS_ARMED"){
                x52output.color_led(devices[0], dwPage, i,SimConnect_Data->spoilers_armed);
            } else if (diody[i] == "SPOILERS"){
                if(SimConnect_Data->spoilers_pos < 5){
                    x52output.color_led(devices[0], dwPage, i,0);
                } else if (SimConnect_Data->spoilers_pos > 95){
                    x52output.color_led(devices[0], dwPage, i,1);
                } else {
                    x52output.color_led(devices[0], dwPage, i,2);
                }
            } else if (diody[i] == "AP_HDG_LOCK"){
                x52output.color_led(devices[0], dwPage, i,SimConnect_Data->ap_hdg_lock);
            } else if (diody[i] == "AP_NAV1_LOCK"){
                x52output.color_led(devices[0], dwPage, i,SimConnect_Data->ap_nav1_lock);
            } else if (diody[i] == "AP_ALT_LOCK"){
                x52output.color_led(devices[0], dwPage, i,SimConnect_Data->ap_alt_lock);
            } else if (diody[i] == "AP_APP_LOCK"){
                x52output.color_led(devices[0], dwPage, i,SimConnect_Data->ap_app_lock);
            } else if (diody[i] == "AUTOTHROTTLE_LOCK"){
                x52output.color_led(devices[0], dwPage, i,SimConnect_Data->autothrottle_lock);
            } else if (diody[i] == "AP_COURSE_COMBO"){
                if(SimConnect_Data->ap_hdg_lock == 0 && SimConnect_Data->ap_nav1_lock == 0 && SimConnect_Data->ap_app_lock == 0){
                    x52output.color_led(devices[0], dwPage, i,-1);
                } else if (SimConnect_Data->ap_app_lock == 1){
                    x52output.color_led(devices[0], dwPage, i,2); //orange
                } else if (SimConnect_Data->ap_nav1_lock == 1){
                    x52output.color_led(devices[0], dwPage, i,1); //green
                } else if (SimConnect_Data->ap_hdg_lock == 1){
                    x52output.color_led(devices[0], dwPage, i,0); //red
                }
            }
        }
    /*} else {
        x52output.clear_led(devices[0], dwPage);
    }*/
}


void MainWindow::mfdPrintLines(){
    wchar_t txt[14];
    //  ---- SINCONNECT --------------
    SimConnect_DataRefs* SimConnect_Data = NULL;
    SimConnect_Data = (SimConnect_DataRefs*)&pObjData->dwData;
    //  ---- dla LVAR ----------------
    //WASM_FCU_DataRefs* pDataRefs_WASM_FCU = NULL;
    //SIMCONNECT_RECV_CLIENT_DATA* pObjData = (SIMCONNECT_RECV_CLIENT_DATA*)pDataLVAR;
    //pDataRefs_WASM_FCU = (WASM_FCU_DataRefs*)&pObjData->dwData;

    for(uint8_t i=0; i<3; i++){
        if(mfdLine[i] == "SPD"){
            swprintf_s(txt, L"SPD: %.0f kts", SimConnect_Data->spd);
        } else if(mfdLine[i] == "ALT"){
            swprintf_s(txt, L"ALT: %.0f ft", SimConnect_Data->altitude);
        } else if(mfdLine[i] == "V/S"){
            swprintf_s(txt, L"V/S: %.0ff/m", SimConnect_Data->verticalspeed);
        } else if(mfdLine[i] == "HDG"){
            swprintf_s(txt, L"HDG: %.0f", SimConnect_Data->hdg);
        }  else if(mfdLine[i] == "AP_HDG"){
            swprintf_s(txt, L"ApHDG: %.0f", SimConnect_Data->ap_hdg);
        } else if(mfdLine[i] == "AP_ALT"){
            swprintf_s(txt, L"ApALT: %.0fft", SimConnect_Data->ap_alt);
        } else if(mfdLine[i] == "AP_V/S"){
            swprintf_s(txt, L"ApV/S: %.0ff/m", SimConnect_Data->ap_vs);
        } else if(mfdLine[i] == "AP_SPD"){
            swprintf_s(txt, L"ApSPD: %.0f kts", SimConnect_Data->ap_spd);
        } else if(mfdLine[i] == "AP_CRS1"){
            swprintf_s(txt, L"ApCRS1: %.0f", SimConnect_Data->ap_crs1);
        } else if(mfdLine[i] == "AP_CRS2"){
            swprintf_s(txt, L"ApCRS2: %.0f", SimConnect_Data->ap_crs2);
        } else {
            swprintf_s(txt, L"");
        }
        int txtsize = std::wcslen (txt);
        DirectOutput_SetString(devices[0], dwPage, i, txtsize, txt);
    }
}


void buttons_enable(Ui::MainWindow *ui_pointer, bool stan)
{
    ui_pointer->fireA_comboBox->setEnabled(stan);
    ui_pointer->fireB_comboBox->setEnabled(stan);
    ui_pointer->fireD_comboBox->setEnabled(stan);
    ui_pointer->fireE_comboBox->setEnabled(stan);
    ui_pointer->clutch_comboBox->setEnabled(stan);
    ui_pointer->fire_comboBox->setEnabled(stan);
    ui_pointer->mfd1_comboBox->setEnabled(stan);
    ui_pointer->mfd2_comboBox->setEnabled(stan);
    ui_pointer->mfd3_comboBox->setEnabled(stan);
    ui_pointer->pov2_comboBox->setEnabled(stan);
    ui_pointer->profil_comboBox->setEnabled(stan);
    ui_pointer->throttle_comboBox->setEnabled(stan);
    ui_pointer->toggle12_comboBox->setEnabled(stan);
    ui_pointer->toggle34_comboBox->setEnabled(stan);
    ui_pointer->toggle56_comboBox->setEnabled(stan);
    ui_pointer->addProfile_pushButton->setEnabled(stan);
    ui_pointer->removeProfil_Button->setEnabled(stan);
    ui_pointer->save_Button->setEnabled(stan);
}

//============================================================================================================================
void MainWindow::delay( int millisecondsToWait )
{
    QTime dieTime = QTime::currentTime().addMSecs( millisecondsToWait );
    while( QTime::currentTime() < dieTime )
    {
        QCoreApplication::processEvents( QEventLoop::AllEvents, 100 );
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //---- zaladowanie obrazka -------------------------------------
    QPixmap pm(QApplication::applicationDirPath()+"/x52.png");
    ui->image_label->setPixmap(pm);
    ui->image_label->setScaledContents(false);
    ui->image_label->setGeometry(30, 100, pm.width(), pm.height());
    //---- zaladowanie XML -----------------------------------------

    XMLretrievPlanes(ui, "plane", "name");
    //---- wczytanie funkcji dla LED ---------------------------------
    ui->fire_comboBox->addItems(LEDS_FUNCTIONS);
    ui->fire_comboBox->model()->sort(0, Qt::AscendingOrder);
    ui->fireA_comboBox->addItems(LEDS_FUNCTIONS);
    ui->fireA_comboBox->model()->sort(0, Qt::AscendingOrder);
    ui->fireB_comboBox->addItems(LEDS_FUNCTIONS);
    ui->fireB_comboBox->model()->sort(0, Qt::AscendingOrder);
    ui->fireD_comboBox->addItems(LEDS_FUNCTIONS);
    ui->fireD_comboBox->model()->sort(0, Qt::AscendingOrder);
    ui->fireE_comboBox->addItems(LEDS_FUNCTIONS);
    ui->fireE_comboBox->model()->sort(0, Qt::AscendingOrder);
    ui->clutch_comboBox->addItems(LEDS_FUNCTIONS);
    ui->clutch_comboBox->model()->sort(0, Qt::AscendingOrder);
    ui->pov2_comboBox->addItems(LEDS_FUNCTIONS);
    ui->pov2_comboBox->model()->sort(0, Qt::AscendingOrder);
    ui->throttle_comboBox->addItems(LEDS_FUNCTIONS);
    ui->throttle_comboBox->model()->sort(0, Qt::AscendingOrder);
    ui->toggle12_comboBox->addItems(LEDS_FUNCTIONS);
    ui->toggle12_comboBox->model()->sort(0, Qt::AscendingOrder);
    ui->toggle34_comboBox->addItems(LEDS_FUNCTIONS);
    ui->toggle34_comboBox->model()->sort(0, Qt::AscendingOrder);
    ui->toggle56_comboBox->addItems(LEDS_FUNCTIONS);
    ui->toggle56_comboBox->model()->sort(0, Qt::AscendingOrder);
    //---- wczytanie funkcji dla MFD ---------------------------------
    ui->mfd1_comboBox->addItems(MFD_FUNCTIONS);
    ui->mfd1_comboBox->model()->sort(0, Qt::AscendingOrder);
    ui->mfd2_comboBox->addItems(MFD_FUNCTIONS);
    ui->mfd2_comboBox->model()->sort(0, Qt::AscendingOrder);
    ui->mfd3_comboBox->addItems(MFD_FUNCTIONS);
    ui->mfd3_comboBox->model()->sort(0, Qt::AscendingOrder);
    //---- ustawienie odczytanych danych -----------------------
    QString def = ui->profil_comboBox->currentText();
    XMLretrievButtons(ui, "plane", def);

    const wchar_t * name = L"X52_plugin";
    DirectOutput_Initialize(name);
    DirectOutput_RegisterDeviceCallback(*DirectOutput_Device_Callback, nullptr);
    DirectOutput_Enumerate(*DirectOutput_Enumerate_Callback, nullptr);
    x52output.init(devices[0], dwPage);
    x52output.color_led(devices[0], dwPage, 0, 1);


    TimerSimconnectReader = new QTimer(this); // timer do obslugi kontroli polaczenia
    connect(TimerSimconnectReader, SIGNAL(timeout()), this, SLOT(readSimmconnectData()));  // sygnal do odczytu simconnect

    TimerFlash = new QTimer(this); // timer do obslugi kontroli polaczenia
    connect(TimerFlash, SIGNAL(timeout()), this, SLOT(x52_flash()));  // sygnal do odczytu simconnect
    TimerFlash->start(80);

    TimerMFDrefresh = new QTimer(this); // timer do obslugi kontroli polaczenia
    connect(TimerMFDrefresh, SIGNAL(timeout()), this, SLOT(mfdPrintLines()));  // sygnal do odczytu simconnect


    TimerAutoconnect = new QTimer(this); // timer do automatycznego polaczenia z simconnect
    connect(TimerAutoconnect, SIGNAL(timeout()), this, SLOT(autoconnect_timer()));
    if(ui->autoconnect_checkBox->isChecked()){
        autocon_time = 10;
        TimerAutoconnect->start(1000);
    }
}

MainWindow::~MainWindow()
{
    delete ui;
    DirectOutput_Deinitialize();
}

void MainWindow::autoconnect_timer()
{
    if(ui->autoconnect_checkBox->isChecked()){
        qDebug() << "Autoconnect in: " + QString::number(autocon_time);
        ui->autoconnect_checkBox->setText("Autoconnect in: " + QString::number(autocon_time));
        if(autocon_time > 0){
            autocon_time--;
        } else {
            MainWindow::on_Connect_Button_clicked(); //laczenie z simconnect (deaktywowanie autoconnect wewnatrz tej funkcji)
        }
    } else {
        ui->autoconnect_checkBox->setText("Autoconnect");
        TimerAutoconnect->stop();
    }
}

void XMLretrievPlanes(Ui::MainWindow *ui_pointer, QString tag, QString att)
{
    QDomDocument document;
    QFile file("settings.xml");
        if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            qDebug() << "Failed to open the file for reading.";
        }
        else
        {
            if(!document.setContent(&file))
            {
                qDebug() << "Failed to load the file for reading.";
            }
            file.close();
        }
        ui_pointer->profil_comboBox->clear();

    QDomElement root = document.firstChildElement();
    QDomNodeList nodes = root.elementsByTagName(tag);

    qDebug() << "PLANES = " << nodes.count();
    for(int i = 0; i < nodes.count(); i++)
    {
        QDomNode elm = nodes.at(i);
        if(elm.isElement())
        {
            QDomElement e = elm.toElement();
            ui_pointer->profil_comboBox->addItem(e.attribute(att));
        }
    }
    ui_pointer->profil_comboBox->model()->sort(0, Qt::AscendingOrder); // posortowanie listy profili
}


void XMLretrievButtons(Ui::MainWindow *ui_pointer, QString tag, QString arg1)
{
    QDomDocument document;
    QFile file("settings.xml");
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "Failed to open the file for reading.";
    }
    else
    {
        if(!document.setContent(&file))
        {
            qDebug() << "Failed to load the file for reading.";
        }
        file.close();
    }
    QDomElement root = document.firstChildElement();
    QDomNodeList plane = root.elementsByTagName(tag);

    for(int i = 0; i < plane.count(); i++)
    {
        QDomNode planenode = plane.at(i);
        if(planenode.isElement())
        {
            QDomElement planeelement = planenode.toElement();
            if(planeelement.attribute("name") == arg1){
                QDomNodeList nodes = planeelement.elementsByTagName("btn");
                QString jakiPzycisk;
                for(int i = 0; i < nodes.count(); i++)
                {
                    QDomNode elm = nodes.at(i);
                    if(elm.isElement())
                    {
                        QDomElement e = elm.toElement();
                        qDebug() << "nazwa: " << e.attribute("name") << " funkcja: " << e.attribute("function");
                        jakiPzycisk = e.attribute("name");
                        diody[dek.at(jakiPzycisk)] = e.attribute("function");
                        if(e.attribute("name") == "LED_FIRE"){
                            ui_pointer->fire_comboBox->setCurrentIndex(ui_pointer->fire_comboBox->findText(e.attribute("function")));
                        } else if(e.attribute("name") == "LED_FIRE_A"){
                            ui_pointer->fireA_comboBox->setCurrentIndex(ui_pointer->fireA_comboBox->findText(e.attribute("function")));
                        } else if(e.attribute("name") == "LED_FIRE_B"){
                            ui_pointer->fireB_comboBox->setCurrentIndex(ui_pointer->fireB_comboBox->findText(e.attribute("function")));
                        } else if(e.attribute("name") == "LED_FIRE_D"){
                            ui_pointer->fireD_comboBox->setCurrentIndex(ui_pointer->fireD_comboBox->findText(e.attribute("function")));
                        } else if(e.attribute("name") == "LED_FIRE_E"){
                            ui_pointer->fireE_comboBox->setCurrentIndex(ui_pointer->fireE_comboBox->findText(e.attribute("function")));
                        } else if(e.attribute("name") == "LED_TOGGLE_1_2"){
                            ui_pointer->toggle12_comboBox->setCurrentIndex(ui_pointer->toggle12_comboBox->findText(e.attribute("function")));
                        } else if(e.attribute("name") == "LED_TOGGLE_3_4"){
                            ui_pointer->toggle34_comboBox->setCurrentIndex(ui_pointer->toggle34_comboBox->findText(e.attribute("function")));
                        } else if(e.attribute("name") == "LED_TOGGLE_5_6"){
                            ui_pointer->toggle56_comboBox->setCurrentIndex(ui_pointer->toggle56_comboBox->findText(e.attribute("function")));
                        } else if(e.attribute("name") == "LED_POV_2"){
                            ui_pointer->pov2_comboBox->setCurrentIndex(ui_pointer->pov2_comboBox->findText(e.attribute("function")));
                        } else if(e.attribute("name") == "LED_CLUTCH"){
                            ui_pointer->clutch_comboBox->setCurrentIndex(ui_pointer->clutch_comboBox->findText(e.attribute("function")));
                        } else if(e.attribute("name") == "LED_THROTTLE"){
                            ui_pointer->throttle_comboBox->setCurrentIndex(ui_pointer->throttle_comboBox->findText(e.attribute("function")));
                        }
                    }
                }
                QDomNodeList nodesMFD = planeelement.elementsByTagName("mfd");
                for(int i = 0; i < nodesMFD.count(); i++)
                {
                    QDomNode elemm = nodesMFD.at(i);
                    if(elemm.isElement())
                    {
                        QDomElement el = elemm.toElement();
                        qDebug() << "MFD nazwa: " << el.attribute("name") << " funkcja: " << el.attribute("function");
                        if(el.attribute("name") == "0"){
                            ui_pointer->mfd1_comboBox->setCurrentIndex(ui_pointer->mfd1_comboBox->findText(el.attribute("function")));
                            mfdLine[0] = ui_pointer->mfd1_comboBox->currentText();
                        } else if(el.attribute("name") == "1"){
                            ui_pointer->mfd2_comboBox->setCurrentIndex(ui_pointer->mfd2_comboBox->findText(el.attribute("function")));
                            mfdLine[1] = ui_pointer->mfd2_comboBox->currentText();
                        } else if(el.attribute("name") == "2"){
                            ui_pointer->mfd3_comboBox->setCurrentIndex(ui_pointer->mfd3_comboBox->findText(el.attribute("function")));
                            mfdLine[3] = ui_pointer->mfd3_comboBox->currentText();
                        }
                    }
                }

            }
        }
    }
}

void XMLcreateNewProfile(Ui::MainWindow *ui_pointer, QString name)
{
    QDomDocument doc;
    QFile file("settings.xml");
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "Failed to open the file for reading.";
    }
    else
    {
        if(!doc.setContent(&file))
        {
            qDebug() << "Failed to load the file for reading.";
        }
        file.close();
    }
    QDomElement root = doc.firstChildElement();

    QDomElement btns = doc.createElement("plane");
    btns.setAttribute("name",name);
    root.appendChild(btns);
    //------------------------------------------------------------------
    XMLsetAttribute(doc, btns, "LED_FIRE", ui_pointer->fire_comboBox->currentText(), 0);
    XMLsetAttribute(doc, btns, "LED_FIRE_A", ui_pointer->fireA_comboBox->currentText(), 0);
    XMLsetAttribute(doc, btns, "LED_FIRE_B", ui_pointer->fireB_comboBox->currentText(), 0);
    XMLsetAttribute(doc, btns, "LED_FIRE_D", ui_pointer->fireD_comboBox->currentText(), 0);
    XMLsetAttribute(doc, btns, "LED_FIRE_E", ui_pointer->fireE_comboBox->currentText(), 0);
    XMLsetAttribute(doc, btns, "LED_TOGGLE_1_2", ui_pointer->toggle12_comboBox->currentText(), 0);
    XMLsetAttribute(doc, btns, "LED_TOGGLE_3_4", ui_pointer->toggle34_comboBox->currentText(), 0);
    XMLsetAttribute(doc, btns, "LED_TOGGLE_5_6", ui_pointer->toggle56_comboBox->currentText(), 0);
    XMLsetAttribute(doc, btns, "LED_POV_2", ui_pointer->pov2_comboBox->currentText(), 0);
    XMLsetAttribute(doc, btns, "LED_CLUTCH", ui_pointer->clutch_comboBox->currentText(), 0);
    XMLsetAttribute(doc, btns, "LED_THROTTLE", ui_pointer->throttle_comboBox->currentText(), 0);
    XMLsetAttribute(doc, btns, "0", ui_pointer->mfd1_comboBox->currentText(), 1);
    XMLsetAttribute(doc, btns, "1", ui_pointer->mfd2_comboBox->currentText(), 1);
    XMLsetAttribute(doc, btns, "2", ui_pointer->mfd3_comboBox->currentText(), 1);
    //------------------------------------------------------------------

    QFile file2("settings.xml");
    if(file2.open(QFile::WriteOnly | QFile::Text)){
        QTextStream in(&file2);
        in<<doc.toString();
        file2.flush();
        file2.close();
        qDebug()<<"finished.";
    }
    else qDebug()<<"file open failed.";
}


/*
 * btns = plik
 * name = nazwa led
 * func = funkcja jaka ten led bedzie pelnil
 * rodzaj = 0 dla LED, 1 dla MFD
*/
void XMLsetAttribute(QDomDocument doc, QDomElement btns, QString name, QString func, uint8_t rodzaj){
    QDomElement btn;
    if(rodzaj == 0){
        btn = doc.createElement("btn");
    } else {
        btn = doc.createElement("mfd");
    }

    btn.setAttribute("name", name);
    btn.setAttribute("function", func);

    btns.appendChild(btn);
}

void XMLremoveProfile(QString name)
{
    QDomDocument doc;
    QFile file("settings.xml");
        if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            qDebug() << "Failed to open the file for reading.";
        }
        else
        {
            if(!doc.setContent(&file))
            {
                qDebug() << "Failed to load the file for reading.";
            }
            file.close();
        }
        QDomElement root = doc.firstChildElement();

        QDomNodeList nodes = root.elementsByTagName("plane");

        qDebug() << "samoloty = " << nodes.count();
        for(int i = 0; i < nodes.count(); i++)
        {
            QDomNode elm = nodes.at(i);
            if(elm.isElement())
            {
                QDomElement e = elm.toElement();
                if(e.attribute("name") == name){
                    root.removeChild(e);
                    qDebug() << "usunieto";
                }
            }
        }

        if(file.open(QFile::WriteOnly | QFile::Text)){
            QTextStream in(&file);
            in<<doc.toString();
            file.flush();
            file.close();
        } else qDebug()<<"file open failed.";
}

void MainWindow::on_profil_comboBox_currentIndexChanged(const QString &arg1)
{
    XMLretrievButtons(ui, "plane", arg1);
}


void MainWindow::on_addProfile_pushButton_clicked()
{
    bool ok;
        QString text = QInputDialog::getText(this,
                                             tr("X51 Plugin"),
                                             tr("Profile name:"),
                                             QLineEdit::Normal,
                                             QDir::home().dirName(),
                                             &ok);
        if (ok && !text.isEmpty()){
            qDebug() << text;
            XMLcreateNewProfile(ui, text);

        }
        XMLretrievPlanes(ui, "plane", "name");
}

void MainWindow::on_removeProfil_Button_clicked()
{
    if(ui->profil_comboBox->currentText() != "!default"){
        QMessageBox msgBox;
        msgBox.setWindowTitle("X52 plugin");
        msgBox.setText("Are you sure you want to delete this profile?   ");
        QPushButton *okButton = msgBox.addButton(tr("Yes"), QMessageBox::ActionRole);
        QPushButton *cancelButton = msgBox.addButton(QMessageBox::Cancel);

        msgBox.exec();

        if (msgBox.clickedButton() == okButton) {
            XMLremoveProfile(ui->profil_comboBox->currentText());
            XMLretrievPlanes(ui, "plane", "name");
        } else if (msgBox.clickedButton() == cancelButton) {
            // abort
        }
    } else {
        QMessageBox msgBox;
        msgBox.setWindowTitle("X52 plugin");
        msgBox.setText("        You cannot delete the default profile!              ");
        msgBox.exec();
    }
}


void MainWindow::on_save_Button_clicked()
{
    QString nazwaProfilu;
    nazwaProfilu = ui->profil_comboBox->currentText();
    XMLremoveProfile(nazwaProfilu);
    XMLcreateNewProfile(ui, nazwaProfilu);
    QMessageBox msgBox;
    msgBox.setWindowTitle("X52 plugin");
    msgBox.setText("        Settings are saved             ");
    msgBox.exec();
}

//======================   OBSLUGA WYSYLANIA/ODBIERANIA SIMCONNECT   ==========================
void CALLBACK dispatchRoutine(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext) {  // callback dla EVENTOW LVAR
    //WASM_FCU_DataRefs* pDataRefs = NULL;
    DWORD receivedID = pData->dwID;
    switch (receivedID)
    {
    case SIMCONNECT_RECV_ID_OPEN: {

        break;
    }
    case SIMCONNECT_RECV_ID_CLIENT_DATA: {
        //SIMCONNECT_RECV_CLIENT_DATA* pObjData = (SIMCONNECT_RECV_CLIENT_DATA*)pData;
        //pDataRefs = (WASM_FCU_DataRefs*)&pObjData->dwData;
        //qDebug() << "Received client data: " << QString::number(pDataRefs->fcu_alt_managed, 'f', 2);
        pDataLVAR = pData;
        break;
    }
    default:
        break;
    }
}


void MainWindow::readSimmconnectData()  // watek obslugi odbioru SIMCONNECT
{
    SimConnect_DataRefs* pDataRefs = NULL;

    hr = SimConnect_GetNextDispatch(hSimConnect, &pData, &cbData); //odczytanie zmiennych
    if (SUCCEEDED(hr)){
        pObjData = (SIMCONNECT_RECV_SIMOBJECT_DATA*)pData;  // zostawic !!! - potrzebne do zapisania danych
        pDataRefs = (SimConnect_DataRefs*)&pObjData->dwData;
        qDebug() << pDataRefs->batt_master;
        SetLed(ui);
    }
    SimConnect_CallDispatch(hSimConnectLVAR, dispatchRoutine, NULL);
    SimConnect_TransmitClientEvent(hSimConnectLVAR, objectID, EVENT_WASM, 0, SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
}

void MainWindow::on_Connect_Button_clicked()
{
    ui->autoconnect_checkBox->setText("Autoconnect");
    TimerAutoconnect->stop();
    //===================================  INICJALIZACJA SIMCONNECT   ==============================
    if (SUCCEEDED(SimConnect_Open(&hSimConnect, "X52 PRO", NULL, 0, 0, 0))){
         qDebug() << "\n --> Connected to Flight Simulator!";
         //---EVENTS ZAPIS-------------------------------------------------------------------------------------------------
         //--A320 BARO PANEL
         hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_KOHLSMAN_INC, "KOHLSMAN_INC");
         hr = SimConnect_AddClientEventToNotificationGroup(hSimConnect, GROUP_A, EVENT_KOHLSMAN_INC);
         hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_KOHLSMAN_DEC, "KOHLSMAN_DEC");
         hr = SimConnect_AddClientEventToNotificationGroup(hSimConnect, GROUP_A, EVENT_KOHLSMAN_DEC);
         hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_KEY_FLIGHT_DIRECTOR, "TOGGLE_FLIGHT_DIRECTOR");
         hr = SimConnect_AddClientEventToNotificationGroup(hSimConnect, GROUP_A, EVENT_KEY_FLIGHT_DIRECTOR);
         hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_KOHLSMAN_SLOT_INDEX_SET, "BAROMETRIC_STD_PRESSURE");
         hr = SimConnect_AddClientEventToNotificationGroup(hSimConnect, GROUP_A, EVENT_KOHLSMAN_SLOT_INDEX_SET);
         //--A320AUTOPILOT
         hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_AP_SPD_VAR_INC, "AP_SPD_VAR_INC");  //SPEED
         hr = SimConnect_AddClientEventToNotificationGroup(hSimConnect, GROUP_A, EVENT_AP_SPD_VAR_INC);
         hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_AP_SPD_VAR_DEC, "AP_SPD_VAR_DEC");
         hr = SimConnect_AddClientEventToNotificationGroup(hSimConnect, GROUP_A, EVENT_AP_SPD_VAR_DEC);

         hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_AP_HDG_VAR_INC, "HEADING_BUG_INC");  //HDG
         hr = SimConnect_AddClientEventToNotificationGroup(hSimConnect, GROUP_A, EVENT_AP_HDG_VAR_INC);
         hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_AP_HDG_VAR_DEC, "HEADING_BUG_DEC");
         hr = SimConnect_AddClientEventToNotificationGroup(hSimConnect, GROUP_A, EVENT_AP_HDG_VAR_DEC);

         hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_AP_ALT_VAR_INC, "AP_ALT_VAR_INC");  //ALTITUDE
         hr = SimConnect_AddClientEventToNotificationGroup(hSimConnect, GROUP_A, EVENT_AP_ALT_VAR_INC);
         hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_AP_ALT_VAR_DEC, "AP_ALT_VAR_DEC");
         hr = SimConnect_AddClientEventToNotificationGroup(hSimConnect, GROUP_A, EVENT_AP_ALT_VAR_DEC);

         hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_AP_VS_VAR_INC, "AP_VS_VAR_INC");  //VS
         hr = SimConnect_AddClientEventToNotificationGroup(hSimConnect, GROUP_A, EVENT_AP_VS_VAR_INC);
         hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_AP_VS_VAR_DEC, "AP_VS_VAR_DEC");
         hr = SimConnect_AddClientEventToNotificationGroup(hSimConnect, GROUP_A, EVENT_AP_VS_VAR_DEC);

         hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_AP_MASTER, "AP_MASTER");  //AP_MASTER
         hr = SimConnect_AddClientEventToNotificationGroup(hSimConnect, GROUP_A, EVENT_AP_MASTER);

         //---VARIABLES----------------------------------------------------------------------------------------------------
         hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "FLAPS HANDLE PERCENT", "Percent");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "SPOILERS HANDLE POSITION", "Percent");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "SPOILERS ARMED", "Bool");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "AIRSPEED INDICATED", "Knots");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "GEAR POSITION", "Percent");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "AUTOPILOT MASTER", "Bool");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "AUTOPILOT HEADING LOCK", "Bool");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "AUTOPILOT NAV1 LOCK", "Bool");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "AUTOPILOT ALTITUDE LOCK", "Bool");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "AUTOPILOT APPROACH HOLD", "Bool");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "AUTOPILOT NAV SELECTED", "Number");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "AUTOPILOT HEADING LOCK DIR", "Degree");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "AUTOPILOT ALTITUDE LOCK VAR", "Feet");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "AUTOPILOT VERTICAL HOLD VAR", "Feet/minute");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "AUTOPILOT AIRSPEED HOLD VAR", "Knots");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "NAV OBS:1", "Degrees");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "NAV OBS:2", "Degrees");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "NAV OBS:3", "Degrees");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "AUTOTHROTTLE ACTIVE", "Bool");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "Indicated Altitude", "feet");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "VERTICAL SPEED", "feet per minute");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "HEADING INDICATOR", "degrees");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "GENERAL ENG THROTTLE LEVER POSITION:1", "Percent");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "LIGHT LANDING", "Bool");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "BRAKE INDICATOR", "Bool");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "BRAKE PARKING POSITION", "Bool");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "GENERAL ENG RPM:1", "Rpm");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "GENERAL ENG RPM:2", "Rpm");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "ELECTRICAL MAIN BUS VOLTAGE", "Volts");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "IS GEAR RETRACTABLE", "Bool");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "LIGHT TAXI", "Bool");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "LIGHT BEACON", "Bool");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "LIGHT NAV", "Bool");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "LIGHT STROBE", "Bool");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "LIGHT PANEL", "Bool");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "STALL WARNING", "Bool");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "OVERSPEED WARNING", "Bool");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "INNER MARKER", "Bool");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "MIDDLE MARKER", "Bool");
             hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "OUTER MARKER", "Bool");

         hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "ELECTRICAL MAIN BUS VOLTAGE", "Volts");
         hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "AUTOPILOT FLIGHT DIRECTOR ACTIVE:1", "Bool");
         hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "KOHLSMAN SETTING MB", "Millibars");
         hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "KOHLSMAN SETTING MB", "inHg");
         hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "AUTOPILOT AIRSPEED HOLD VAR", "Knots");
         hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "AUTOPILOT HEADING LOCK DIR", "Degree");
         hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "AUTOPILOT ALTITUDE LOCK VAR:3", "Feet");
         hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "AUTOPILOT VERTICAL HOLD VAR", "Feet/minute");
         hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "AUTOTHROTTLE ACTIVE", "Bool");
         hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ID_AP, "AUTOPILOT ALTITUDE LOCK VAR:1", "Feet");

         hr = SimConnect_AddToDataDefinition( hSimConnect, DEFINITION_ID_AP, "TITLE", nullptr, SIMCONNECT_DATATYPE::SIMCONNECT_DATATYPE_STRING256); // nazwa samolotu - zostawic jako ostatni!!!



         hr = SimConnect_RequestDataOnSimObject(hSimConnect, REQUEST_AP_SETTINGS, DEFINITION_ID_AP, SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_SIM_FRAME);
         //---EVENTS ODCZYT-------------------------------------------------------------------------------------------------
         //hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_BRAKES, "AP_MASTER");
         //hr = SimConnect_AddClientEventToNotificationGroup(hSimConnect, GROUP_B, EVENT_BRAKES);
         //hr = SimConnect_SetNotificationGroupPriority(hSimConnect, GROUP_B, SIMCONNECT_GROUP_PRIORITY_HIGHEST);
         //-----------------------------------------------------------------------------------------------------------------
         hr = SimConnect_SetNotificationGroupPriority(hSimConnect, GROUP_A, SIMCONNECT_GROUP_PRIORITY_STANDARD);

         //-----------  LVAR !!!!!!!!!!!!!! ------------------------------------------------------------------
         hr = SimConnect_Open(&hSimConnectLVAR, "X52 PRO Plugin", nullptr, 0, 0, 0);
             if (S_OK == hr) {
                 qDebug() << "Connected LVAR Modul\n";
                 hr = SimConnect_MapClientDataNameToID(hSimConnectLVAR, "EFIS_CDA", ClientDataID);
                 hr &= SimConnect_AddToClientDataDefinition(hSimConnectLVAR, DEFINITION_1, SIMCONNECT_CLIENTDATAOFFSET_AUTO, sizeof(WASM_FCU_DataRefs));
                 hr &= SimConnect_CreateClientData(hSimConnectLVAR, ClientDataID, sizeof(WASM_FCU_DataRefs), SIMCONNECT_CLIENT_DATA_REQUEST_FLAG_CHANGED);
                 hr &= SimConnect_RequestClientData(hSimConnectLVAR, ClientDataID, REQUEST_1, DEFINITION_1, SIMCONNECT_CLIENT_DATA_PERIOD_VISUAL_FRAME, SIMCONNECT_CLIENT_DATA_REQUEST_FLAG_DEFAULT);

                 /*************** REQUEST DATA FROM THE AIRCRAFT ***************/
                 hr = SimConnect_MapClientEventToSimEvent(hSimConnectLVAR, EVENT_2, "#0x11097");
                 hr = SimConnect_AddClientEventToNotificationGroup(hSimConnectLVAR, GROUP_A, EVENT_2, true);

                 hr = SimConnect_MapClientEventToSimEvent(hSimConnectLVAR, EVENT_1, "#0x11099");
                 hr = SimConnect_AddClientEventToNotificationGroup(hSimConnectLVAR, GROUP_A, EVENT_1, true);

                 hr = SimConnect_MapClientEventToSimEvent(hSimConnectLVAR, EVENT_WASM, "LVAR_ACCESS.EFIS");
                 hr = SimConnect_AddClientEventToNotificationGroup(hSimConnectLVAR, GROUP_A, EVENT_WASM, true);

                 //hr = SimConnect_SetNotificationGroupPriority(hSimConnectLVAR, GROUP_A, SIMCONNECT_GROUP_PRIORITY_DEFAULT); //usunac jesli wszystko ok
             }
         //---------------------------------------------------------------------------------------------------
         buttons_enable(ui, false);
         ui->Connect_Button->setEnabled(false);
         ui->Disconnect_Button->setEnabled(true);
         ui->Connected_CheckBox->setChecked(true);
         ui->statusbar->showMessage("Connected with MSFS", STATUSBAR_TIMEOUT);
         delay(1000);
         TimerSimconnectReader->start(10); //start timera do odczytu simconnect
         TimerMFDrefresh->start(MFD_REF_TIME); // watek odswiezania MFD
    } else {
        ui->statusbar->showMessage("Can't connect with MSFS!", STATUSBAR_TIMEOUT);
        buttons_enable(ui, true);
    }
}


void MainWindow::on_Disconnect_Button_clicked()
{
    TimerMFDrefresh->stop(); // watek odswiezania MFD
    TimerSimconnectReader->stop();
    hr = SimConnect_Close(hSimConnect);

    qDebug() << "Disconnected";
    ui->Connect_Button->setEnabled(true);
    ui->Disconnect_Button->setEnabled(false);
    ui->Connected_CheckBox->setChecked(false);
    ui->AircraftTitle_label->setText("");
    ui->statusbar->showMessage("Disconnected", STATUSBAR_TIMEOUT);
    buttons_enable(ui, true);
    x52output.intro_msg(devices[0], dwPage);
}


void MainWindow::on_autoconnect_checkBox_stateChanged(int arg1)
{
    if(arg1 == 2){
        if(ui->Connect_Button->isEnabled()){
            autocon_time = 10;
            TimerAutoconnect->start(1000);
        }
    }
}


void MainWindow::on_AboutPushButton_clicked()
{
    QMessageBox msgAbout;
    msgAbout.setWindowTitle("About...");
    msgAbout.setIcon(QMessageBox::Information);
    const QString message = "<p style='text-align: center;'>&nbsp;</p>"
                            "<p style='text-align: center; font-weight:bold; font-size: 26px;'><strong>X52 PRO Plugin</strong></p>"
                            "<p style='text-align: center; font-size: 14;'>for MSFS 2020</p>"
                            "<p style='text-align: center; font-size: 14;'>Version 3.0</p>"
                            "<p style='text-align: center; font-size: 14;'>by <b>KoSik</b></p>"
                            "<p style='text-align: center;'>&nbsp;</p>"
                            "<p style='text-align: center; font-size: 14;'>e-mail: kosik84@gmail.com</p>";
    msgAbout.setInformativeText(message);
    QSpacerItem* horizontalSpacer = new QSpacerItem(500, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    QGridLayout* layout = (QGridLayout*)msgAbout.layout();
    layout->addItem(horizontalSpacer, layout->rowCount(), 0, 1, layout->columnCount());
    msgAbout.exec();
}
