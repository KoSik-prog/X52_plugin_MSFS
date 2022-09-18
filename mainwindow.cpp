#include <QDebug>
#include <QList>
#include <QDateTime>
#include <QTime>
#include <QSignalMapper>
#include <QTimer>
#include <QtXml>
#include <QFile>
#include <QPixmap>
#include <QInputDialog>
#include <QMessageBox>
#include <QSpacerItem>
#include <QGridLayout>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "x52_output.h"
#include "structures_simconnect.h"
#include "settings.h"

extern "C" {
    #include "SimConnect.h"
}

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

#define MFD_REFRESH_TIME 100
#define STATUSBAR_TIMEOUT 3000 // x 1000 = seconds

uint8_t autocon_time = 0;
// ------------ SETTINGS ------------------------------------------------------------
settings xmlSettings;
QStringList profilesArray = {};
QStringList buttonsArray = {};
QStringList mfdArray = {"AP_CRS1", "AP_HDG", "AP_V/S"};
QStringList settingsArray = {};
// ------------ DATA FOR X52 --------------------------------------------------------
extern std::vector<void*> devices;
x52_output x52output;
DWORD dwPage = 1;
QVector<QString> ledsArray {"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""};
std::map<QString, int> buttonDecodeArray {{"LED_FIRE", 0}, {"LED_FIRE_A", 1}, {"LED_FIRE_B", 3}, {"LED_FIRE_D", 5}, {"LED_FIRE_E", 7}, {"LED_TOGGLE_1_2", 9}, {"LED_TOGGLE_3_4", 11}, {"LED_TOGGLE_5_6", 13}, {"LED_POV_2", 15}, {"LED_CLUTCH", 17}, {"LED_THROTTLE", 19}};
// ----------- DATA FOR SIMCONNECT --------------------------------------------------
HANDLE hSimConnect = NULL;
HANDLE hSimConnectLVAR = NULL;
SIMCONNECT_CLIENT_DATA_ID ClientDataID = 1; // for LVAR
SIMCONNECT_OBJECT_ID objectID = SIMCONNECT_OBJECT_ID_USER; // for LVAR
SIMCONNECT_RECV* pDataLVAR;

HRESULT hr;
SIMCONNECT_RECV* pData = NULL;
DWORD cbData = 0;
SIMCONNECT_RECV_SIMOBJECT_DATA* pObjData = NULL;
//--------- FUNCTIONS DECLARATIONS ----------------------------------------------------------
void XMLretrievProfiles(Ui::MainWindow *ui_pointer);
void XMLretrievButtons(Ui::MainWindow *ui_pointer, QString tag, QString arg1);
void XMLsetAttribute(QDomDocument doc, QDomElement btns, QString name, QString func, uint8_t rodzaj);
void SetLed(Ui::MainWindow *ui_pointer);
//---------------------- FUNCTIONS DEFINITIONS ------------------------------------------------------------------------------------------------------
void MainWindow::SetLed(Ui::MainWindow *ui_pointer){
    SimConnect_DataRefs* SimConnect_Data = NULL;
    SimConnect_Data = (SimConnect_DataRefs*)&pObjData->dwData;
    //  ---- for LVARs ----------------
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
                    TimerMFDrefresh->start(MFD_REFRESH_TIME);
                    break;
                }
            }
            XMLretrievButtons(ui_pointer, "plane", ui_pointer->profil_comboBox->currentText());
        }
    }

    //if(SimConnect_Data->batt_master > 10){  //if power on
        uint8_t i;
        for(i=0;i<19;i++){  //bo numer ostatniego led to 19
            if (ledsArray[i] == "RED"){
                x52output.color_led(devices[0], dwPage, i,0);
            } else if (ledsArray[i] == "GREEN"){
                x52output.color_led(devices[0], dwPage, i,1);
            } else if (ledsArray[i] == "ORANGE"){
                x52output.color_led(devices[0], dwPage, i,2);
            } else if(ledsArray[i] == "GEAR"){
                if(SimConnect_Data->gear >0 && SimConnect_Data->gear <= 99){  //GEAR
                    x52output.color_led(devices[0], dwPage, i,2);
                } else if (SimConnect_Data->gear == 100){
                    x52output.color_led(devices[0], dwPage, i,1);
                } else {
                    x52output.color_led(devices[0], dwPage, i,0);
                }
            } else if (ledsArray[i] == "BRAKE"){
                x52output.color_led(devices[0], dwPage, i,SimConnect_Data->brake); //NORMAL BRAKE
            } else if (ledsArray[i] == "PARKING_BRAKE"){
                x52output.color_led(devices[0], dwPage, i,SimConnect_Data->parkingbrake); //PARKING BRAKE
            }else if (ledsArray[i] == "PARK_BRK_COMBO"){
                if(SimConnect_Data->parkingbrake != 0){
                    x52output.x52Flash(i, 3, 1);
                } else {
                    x52output.x52Flash(i, 0, 0);
                    x52output.color_led_flash(devices[0], dwPage, i, 0);
                }
            } else if (ledsArray[i] == "BRAKES_COMBO"){
                if(SimConnect_Data->brake != 0 && SimConnect_Data->parkingbrake ==0){
                    x52output.color_led(devices[0], dwPage, i, 1);
                    x52output.x52Flash(i, 0, 0);
                } else if(SimConnect_Data->parkingbrake !=0){
                    x52output.x52Flash(i, 3, 1);
                } else {
                    x52output.x52Flash(i, 0, 0);
                    x52output.color_led_flash(devices[0], dwPage, i, 0);
                }
            }  else if (ledsArray[i] == "RPM_ENGINE_1"){
                if(SimConnect_Data->enginerpm1 >500){  //ENGINE 1 RPM
                    x52output.color_led(devices[0], dwPage, i,1);
                } else {
                    x52output.color_led(devices[0], dwPage, i,0);
                }
            } else if (ledsArray[i] == "RPM_ENGINE_2"){
                if(SimConnect_Data->enginerpm2 >500){  //ENGINE 2 RPM
                    x52output.color_led(devices[0], dwPage, i,1);
                } else {
                    x52output.color_led(devices[0], dwPage, i,0);
                }
            } else if (ledsArray[i] == "FLAPS"){
                if(SimConnect_Data->flaps >=10 && SimConnect_Data->flaps < 90){ //FLAPS
                    x52output.color_led(devices[0], dwPage, i,2);
                } else if (SimConnect_Data->flaps >=90){
                    x52output.color_led(devices[0], dwPage, i,1);
                } else {
                    x52output.color_led(devices[0], dwPage, i,0);
                }
            }  else if (ledsArray[i] == "LANDING_LIGHTS"){
                x52output.color_led(devices[0], dwPage, i,SimConnect_Data->lightlanding);
            } else if (ledsArray[i] == "AP_MASTER"){
                x52output.color_led(devices[0], dwPage, i,SimConnect_Data->ap_master);
            } else if (ledsArray[i] == "REV_THRUST"){
                if(SimConnect_Data->rev < 0){ //REVERSE THRUST
                    x52output.color_led(devices[0], dwPage, i,0);
                } else {
                    x52output.color_led(devices[0], dwPage, i,1);
                }
            } else if (ledsArray[i] == "TAXI_LIGHTS"){
                x52output.color_led(devices[0], dwPage, i,SimConnect_Data->light_taxi);
            }  else if (ledsArray[i] == "NAV_LIGHTS"){
                x52output.color_led(devices[0], dwPage, i,SimConnect_Data->light_nav);
            } else if (ledsArray[i] == "BEACON_LIGHTS"){
                x52output.color_led(devices[0], dwPage, i,SimConnect_Data->light_beacon);
            } else if (ledsArray[i] == "STROBE_LIGHTS"){
                x52output.color_led(devices[0], dwPage, i,SimConnect_Data->light_strobe);
            } else if (ledsArray[i] == "PANEL_LIGHTS"){
                x52output.color_led(devices[0], dwPage, i,SimConnect_Data->light_panel);
            } else if (ledsArray[i] == "LIGHTS_COMBO"){
                if(SimConnect_Data->light_taxi == 1 && SimConnect_Data->lightlanding == 1){
                    x52output.color_led(devices[0], dwPage, i,0);
                } else if (SimConnect_Data->light_taxi == 1 && SimConnect_Data->lightlanding == 0){
                    x52output.color_led(devices[0], dwPage, i,2);
                } else if (SimConnect_Data->light_taxi == 0 && SimConnect_Data->lightlanding == 1){
                    x52output.color_led(devices[0], dwPage, i,1);
                } else {
                    x52output.color_led(devices[0], dwPage, i,-1);
                }
            } else if (ledsArray[i] == "STALL_WARNING"){
                if(SimConnect_Data->stall_wrn == 1){
                    x52output.x52Flash(i, 1, 1);
                } else {
                    x52output.x52Flash(i, 0, 0);
                    x52output.color_led(devices[0], dwPage, i,-1);
                }
            } else if (ledsArray[i] == "OVERSPEED_WARNING"){
                if(SimConnect_Data->overspeed_wrn == 1){
                    x52output.x52Flash(i, 1, 1);
                } else {
                    x52output.x52Flash(i, 0, 0);
                    x52output.color_led(devices[0], dwPage, i,-1);
                }
            } else if (ledsArray[i] == "MARKERS"){
                if(SimConnect_Data->inner_marker == 1){
                    x52output.x52Flash(i, 1, 1);
                } else if(SimConnect_Data->middle_marker == 1){
                    x52output.x52Flash(i, 1, 3);
                } else if(SimConnect_Data->outer_marker == 1){
                    x52output.x52Flash(i, 1, 2);
                } else {
                    x52output.x52Flash(i, 0, 0);
                    x52output.color_led(devices[0], dwPage, i, -1);
                }
            } else if (ledsArray[i] == "INNER_MARKER"){
                if(SimConnect_Data->inner_marker == 1){
                    x52output.x52Flash(i, 1, 3);
                } else {
                    x52output.x52Flash(i, 0, 0);
                    x52output.color_led(devices[0], dwPage, i,-1);
                }
            } else if (ledsArray[i] == "MIDDLE_MARKER"){
                if(SimConnect_Data->middle_marker == 1){
                    x52output.x52Flash(i, 1, 3);
                } else {
                    x52output.x52Flash(i, 0, 0);
                    x52output.color_led(devices[0], dwPage, i,-1);
                }
            } else if (ledsArray[i] == "OUTER_MARKER"){
                if(SimConnect_Data->outer_marker == 1){
                    x52output.x52Flash(i, 1, 3);
                } else {
                    x52output.x52Flash(i, 0, 0);
                    x52output.color_led(devices[0], dwPage, i,-1);
                }
            } else if (ledsArray[i] == "SPOILERS_ARMED"){
                x52output.color_led(devices[0], dwPage, i,SimConnect_Data->spoilers_armed);
            } else if (ledsArray[i] == "SPOILERS"){
                if(SimConnect_Data->spoilers_pos < 5){
                    x52output.color_led(devices[0], dwPage, i,0);
                } else if (SimConnect_Data->spoilers_pos > 95){
                    x52output.color_led(devices[0], dwPage, i,1);
                } else {
                    x52output.color_led(devices[0], dwPage, i,2);
                }
            } else if (ledsArray[i] == "AP_HDG_LOCK"){
                x52output.color_led(devices[0], dwPage, i,SimConnect_Data->ap_hdg_lock);
            } else if (ledsArray[i] == "AP_NAV1_LOCK"){
                x52output.color_led(devices[0], dwPage, i,SimConnect_Data->ap_nav1_lock);
            } else if (ledsArray[i] == "AP_ALT_LOCK"){
                x52output.color_led(devices[0], dwPage, i,SimConnect_Data->ap_alt_lock);
            } else if (ledsArray[i] == "AP_APP_LOCK"){
                x52output.color_led(devices[0], dwPage, i,SimConnect_Data->ap_app_lock);
            } else if (ledsArray[i] == "AUTOTHROTTLE_LOCK"){
                x52output.color_led(devices[0], dwPage, i,SimConnect_Data->autothrottle_lock);
            } else if (ledsArray[i] == "AP_COURSE_COMBO"){
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
        if(mfdArray.at(i) == "SPD"){
            swprintf_s(txt, L"SPD: %.0f kts", SimConnect_Data->spd);
        } else if(mfdArray.at(i) == "ALT"){
            swprintf_s(txt, L"ALT: %.0f ft", SimConnect_Data->altitude);
        } else if(mfdArray.at(i) == "V/S"){
            swprintf_s(txt, L"V/S: %.0ff/m", SimConnect_Data->verticalspeed);
        } else if(mfdArray.at(i) == "HDG"){
            swprintf_s(txt, L"HDG: %.0f", SimConnect_Data->hdg);
        }  else if(mfdArray.at(i) == "AP_HDG"){
            swprintf_s(txt, L"ApHDG: %.0f", SimConnect_Data->ap_hdg);
        } else if(mfdArray.at(i) == "AP_ALT"){
            swprintf_s(txt, L"ApALT: %.0fft", SimConnect_Data->ap_alt);
        } else if(mfdArray.at(i) == "AP_V/S"){
            swprintf_s(txt, L"ApV/S: %.0ff/m", SimConnect_Data->ap_vs);
        } else if(mfdArray.at(i) == "AP_SPD"){
            swprintf_s(txt, L"ApSPD: %.0f kts", SimConnect_Data->ap_spd);
        } else if(mfdArray.at(i) == "AP_CRS1"){
            swprintf_s(txt, L"ApCRS1: %.0f", SimConnect_Data->ap_crs1);
        } else if(mfdArray.at(i) == "AP_CRS2"){
            swprintf_s(txt, L"ApCRS2: %.0f", SimConnect_Data->ap_crs2);
        } else {
            swprintf_s(txt, L"");
        }
        int txtsize = std::wcslen (txt);
        x52output.setString(devices[0], dwPage, i, txtsize, txt);
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
void MainWindow::delay( int millisecondsToWait )
{
    QTime dieTime = QTime::currentTime().addMSecs( millisecondsToWait );
    while( QTime::currentTime() < dieTime )
    {
        QCoreApplication::processEvents( QEventLoop::AllEvents, 100 );
    }
}

//---------- SETTINGS ------------------------------------------------
void XMLretrievProfiles(Ui::MainWindow *ui_pointer){
    xmlSettings.get_planes("plane", "name");
    bool oldState = ui_pointer->profil_comboBox->blockSignals(true);
    ui_pointer->profil_comboBox->clear();
    ui_pointer->profil_comboBox->blockSignals(oldState);
    for(int i=0; i<profilesArray.size(); i++){
        ui_pointer->profil_comboBox->addItem(profilesArray.at(i));
    }
}

void XMLretrievButtons(Ui::MainWindow *ui_pointer, QString tag, QString name){
    xmlSettings.get_functions("plane", name);
    ui_pointer->fire_comboBox->setCurrentIndex(ui_pointer->fire_comboBox->findText(buttonsArray.at(0)));
    ui_pointer->fireA_comboBox->setCurrentIndex(ui_pointer->fireA_comboBox->findText(buttonsArray.at(1)));
    ui_pointer->fireB_comboBox->setCurrentIndex(ui_pointer->fireB_comboBox->findText(buttonsArray.at(2)));
    ui_pointer->fireD_comboBox->setCurrentIndex(ui_pointer->fireD_comboBox->findText(buttonsArray.at(3)));
    ui_pointer->fireE_comboBox->setCurrentIndex(ui_pointer->fireE_comboBox->findText(buttonsArray.at(4)));
    ui_pointer->toggle12_comboBox->setCurrentIndex(ui_pointer->toggle12_comboBox->findText(buttonsArray.at(5)));
    ui_pointer->toggle34_comboBox->setCurrentIndex(ui_pointer->toggle34_comboBox->findText(buttonsArray.at(6)));
    ui_pointer->toggle56_comboBox->setCurrentIndex(ui_pointer->toggle56_comboBox->findText(buttonsArray.at(7)));
    ui_pointer->pov2_comboBox->setCurrentIndex(ui_pointer->pov2_comboBox->findText(buttonsArray.at(8)));
    ui_pointer->clutch_comboBox->setCurrentIndex(ui_pointer->clutch_comboBox->findText(buttonsArray.at(9)));
    ui_pointer->throttle_comboBox->setCurrentIndex(ui_pointer->throttle_comboBox->findText(buttonsArray.at(10)));

    ui_pointer->mfd1_comboBox->setCurrentIndex(ui_pointer->mfd1_comboBox->findText(mfdArray.at(0)));
    ui_pointer->mfd2_comboBox->setCurrentIndex(ui_pointer->mfd2_comboBox->findText(mfdArray.at(1)));
    ui_pointer->mfd3_comboBox->setCurrentIndex(ui_pointer->mfd3_comboBox->findText(mfdArray.at(2)));
}

void XMLcreateNewProfile(Ui::MainWindow *ui_pointer, QString name){
    settingsArray.clear();
    buttonsArray.clear();
    mfdArray.clear();

    buttonsArray.append(ui_pointer->fire_comboBox->currentText());
    buttonsArray.append(ui_pointer->fireA_comboBox->currentText());
    buttonsArray.append(ui_pointer->fireB_comboBox->currentText());
    buttonsArray.append(ui_pointer->fireD_comboBox->currentText());
    buttonsArray.append(ui_pointer->fireE_comboBox->currentText());
    buttonsArray.append(ui_pointer->toggle12_comboBox->currentText());
    buttonsArray.append(ui_pointer->toggle34_comboBox->currentText());
    buttonsArray.append(ui_pointer->toggle56_comboBox->currentText());
    buttonsArray.append(ui_pointer->pov2_comboBox->currentText());
    buttonsArray.append(ui_pointer->clutch_comboBox->currentText());
    buttonsArray.append(ui_pointer->throttle_comboBox->currentText());
    mfdArray.append(ui_pointer->mfd1_comboBox->currentText());
    mfdArray.append(ui_pointer->mfd2_comboBox->currentText());
    mfdArray.append(ui_pointer->mfd3_comboBox->currentText());

    settingsArray.append(buttonsArray);
    settingsArray.append(mfdArray);
    qDebug() << "ble:" << settingsArray;
    xmlSettings.create_newProfile(name, settingsArray);
}
//======================   SIMCONNECT SEND / RECEIVE OPERATION   ==========================
void CALLBACK dispatchRoutine(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext) {  // callback for LVAR events
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
        pObjData = (SIMCONNECT_RECV_SIMOBJECT_DATA*)pData;
        pDataRefs = (SimConnect_DataRefs*)&pObjData->dwData;
        //qDebug() << pDataRefs->batt_master;
        SetLed(ui);
    }
    SimConnect_CallDispatch(hSimConnectLVAR, dispatchRoutine, NULL);
    SimConnect_TransmitClientEvent(hSimConnectLVAR, objectID, EVENT_WASM, 0, SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
}

//===================== MAIN ============================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QPixmap pm(QApplication::applicationDirPath()+"/x52.png");
    ui->image_label->setPixmap(pm);
    ui->image_label->setScaledContents(false);
    ui->image_label->setGeometry(30, 100, pm.width(), pm.height());
    //---- read XML -----------------------------------------
    //XMLretrievPlanes(ui, "plane", "name");
    //xmlSettings.XMLretrievPlanes("plane", "name");
    XMLretrievProfiles(ui);
    //---- load LED functions ---------------------------------
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
    //---- read MFD functions ---------------------------------
    ui->mfd1_comboBox->addItems(MFD_FUNCTIONS);
    ui->mfd1_comboBox->model()->sort(0, Qt::AscendingOrder);
    ui->mfd2_comboBox->addItems(MFD_FUNCTIONS);
    ui->mfd2_comboBox->model()->sort(0, Qt::AscendingOrder);
    ui->mfd3_comboBox->addItems(MFD_FUNCTIONS);
    ui->mfd3_comboBox->model()->sort(0, Qt::AscendingOrder);
    //---- OTHERS ---------------------------------------------
    QString def = ui->profil_comboBox->currentText();
    XMLretrievButtons(ui, "plane", def);

    x52output.init(L"X52_plugin");
    x52output.addPage(devices[0], dwPage);
    x52output.color_led(devices[0], dwPage, 0, 1);


    TimerSimconnectReader = new QTimer(this);
    connect(TimerSimconnectReader, SIGNAL(timeout()), this, SLOT(readSimmconnectData()));

    TimerFlash = new QTimer(this);
    connect(TimerFlash, SIGNAL(timeout()), this, SLOT(x52_flash()));
    TimerFlash->start(80);

    TimerMFDrefresh = new QTimer(this);
    connect(TimerMFDrefresh, SIGNAL(timeout()), this, SLOT(mfdPrintLines()));

    TimerAutoconnect = new QTimer(this); // auto connection timer
    connect(TimerAutoconnect, SIGNAL(timeout()), this, SLOT(autoconnect_timer()));
    if(ui->autoconnect_checkBox->isChecked()){
        autocon_time = 10;
        TimerAutoconnect->start(1000);
    }
}

MainWindow::~MainWindow()
{
    delete ui;
    x52output.deInit();
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



void MainWindow::on_profil_comboBox_currentIndexChanged(const QString &arg1){
    XMLretrievButtons(ui, "plane", arg1);
}


void MainWindow::on_addProfile_pushButton_clicked(){
    bool ok;
        QString text = QInputDialog::getText(this,
                                             tr("X51 Plugin"),
                                             tr("Profile name:"),
                                             QLineEdit::Normal,
                                             QDir::home().dirName(),
                                             &ok);
        if (ok && !text.isEmpty()){
            qDebug() << "new profile: " << text;
            XMLcreateNewProfile(ui, text);

        }
        XMLretrievProfiles(ui);
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
            xmlSettings.remove_profile(ui->profil_comboBox->currentText());
            XMLretrievProfiles(ui);
        } else if (msgBox.clickedButton() == cancelButton) {
            // abort
        }
    } else {
        QMessageBox msgBox;
        msgBox.setWindowTitle("X52 plugin");
        msgBox.setText("        You can't delete the default profile!              ");
        msgBox.exec();
    }
}


void MainWindow::on_save_Button_clicked()
{
    QString nazwaProfilu;
    nazwaProfilu = ui->profil_comboBox->currentText();
    xmlSettings.remove_profile(nazwaProfilu);
    XMLcreateNewProfile(ui, nazwaProfilu);
    QMessageBox msgBox;
    msgBox.setWindowTitle("X52 plugin");
    msgBox.setText("        Settings are saved             ");
    msgBox.exec();
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
         hr = SimConnect_AddToDataDefinition( hSimConnect, DEFINITION_ID_AP, "TITLE", nullptr, SIMCONNECT_DATATYPE::SIMCONNECT_DATATYPE_STRING256); // airplane name - at last!!!

         hr = SimConnect_RequestDataOnSimObject(hSimConnect, REQUEST_AP_SETTINGS, DEFINITION_ID_AP, SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_SIM_FRAME);
         //--- EVENTS READ -------------------------------------------------------------------------------------------------
         //hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_BRAKES, "AP_MASTER");
         //hr = SimConnect_AddClientEventToNotificationGroup(hSimConnect, GROUP_B, EVENT_BRAKES);
         //hr = SimConnect_SetNotificationGroupPriority(hSimConnect, GROUP_B, SIMCONNECT_GROUP_PRIORITY_HIGHEST);
         //-----------------------------------------------------------------------------------------------------------------
         hr = SimConnect_SetNotificationGroupPriority(hSimConnect, GROUP_A, SIMCONNECT_GROUP_PRIORITY_STANDARD);
         //-----------  LVAR  ----------------------------------------------------------------------------------------------
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
         TimerSimconnectReader->start(10); //simconnect read timer start
         TimerMFDrefresh->start(MFD_REFRESH_TIME); // MFD refresh thread
    } else {
        ui->statusbar->showMessage("Can't connect with MSFS!", STATUSBAR_TIMEOUT);
        buttons_enable(ui, true);
    }
}


void MainWindow::on_Disconnect_Button_clicked()
{
    TimerMFDrefresh->stop(); // MFD refresh thread
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

