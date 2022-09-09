#ifndef STRUCTURES_SIMCONNECT_H
#define STRUCTURES_SIMCONNECT_H

#include "SimConnect.h"
#include <QStringList>

enum GROUP_ID{
    GROUP_A,
    GROUP_B
};

enum DATA_DEFINE_ID{
    DEFINITION_ID_AP,
};

enum DATA_REQUEST_ID{
    REQUEST_AP_SETTINGS,
};

enum VARIABLE_ID{
    EVENT_SET_AP_ALTITUDE,
};

enum EVENT_ID2 {
    EVENT_1 = 1,
    EVENT_WASM = 2,
    EVENT_2 = 3,
};

enum DATA_DEFINE_ID2 {
    DEFINITION_1 = 12,
    DEFINITION_2 = 13,
};

enum DATA_REQUEST_ID2 {
    REQUEST_1 = 10,
};

enum EVENT_ID{              //EVENTY SIMCONNECT
    EVENT_KOHLSMAN_INC,
    EVENT_KOHLSMAN_DEC,
    EVENT_KEY_FLIGHT_DIRECTOR,
    EVENT_KOHLSMAN_SLOT_INDEX_SET,
    EVENT_AP_SPD_VAR_INC,
    EVENT_AP_SPD_VAR_DEC,
    EVENT_AP_HDG_VAR_INC,
    EVENT_AP_HDG_VAR_DEC,
    EVENT_AP_ALT_VAR_INC,
    EVENT_AP_ALT_VAR_DEC,
    EVENT_AP_VS_VAR_INC,
    EVENT_AP_VS_VAR_DEC,
    EVENT_AP_MASTER,
};

enum LVAR_EFIS_PANEL  		// A320 EFIS + AUTOPILOT PANEL  - EVENTY WASM
{
    XMLVAR_Baro1_Mode,
    XMLVAR_Baro_Selector_HPA_1,
    A32NX_AUTOPILOT_SPEED_SELECTED,  // 1 - auto
    A32NX_AUTOPILOT_1_ACTIVE,
    A32NX_AUTOPILOT_2_ACTIVE,
    A32NX_AUTOTHRUST_STATUS,
    A32NX_FMA_EXPEDITE_MODE,
    A32NX_AUTOPILOT_APPR_MODE,
    A32NX_AUTOPILOT_LOC_MODE,
    XMLVAR_Autopilot_Altitude_Increment,
    BTN_LS_1_FILTER_ACTIVE,
    A32NX_FCU_SPD_MANAGED_DOT,
    A32NX_FCU_SPD_MANAGED_DASHES,
    A32NX_FCU_HDG_MANAGED_DOT,
    A32NX_FCU_HDG_MANAGED_DASHES,
    A32NX_AUTOPILOT_VS_SELECTED,
    A32NX_FCU_VS_MANAGED,  // 1 - auto
    A32NX_AUTOPILOT_HEADING_SELECTE, // HDG
    A32NX_FCU_ALT_MANAGED,  // 1 - auto
};

struct WASM_FCU_DataRefs{
    double id = 1;
    double baro1_mode = 1.;
    double baro_selector_hpa = 1.;
    double autopilot_speed_selected = 1.;
    double autopilot_1_active = 1.;
    double autopilot_2_active = 1.;
    double autothrust_status = 1.;
    double fma_expedite_mode = 1.;
    double autopilot_appr_mode = 1.;
    double autopilot_loc_mode = 1.;
    double autopilot_altitude_increment = 1.;
    double baro_ls = 1.;
    double fcu_speed_dot = 1.;
    double fcu_speed_dashes = 1.;
    double fcu_hdg_dot = 1.;
    double fcu_hdg_dashes = 1.;
    double autopilot_vs_selected = 1.;
    double fcu_vs_managed = 1.; // 1 - auto
    double autopilot_heading_selected = 1.;
    double fcu_alt_managed = 1.;
};

struct SimConnect_DataRefs{
    double flaps;
     double spoilers_pos;
     double spoilers_armed;
     double spd;
     double gear;
     double ap_master;
     double ap_hdg_lock;
     double ap_nav1_lock;
     double ap_alt_lock;
     double ap_app_lock;
     double ap_nav_selected;
     double ap_hdg;
     double ap_alt;
     double ap_vs;
     double ap_spd;
     double ap_crs1;
     double ap_crs2;
     double ap_crs3;
     double autothrottle_lock;
     double altitude;
     double verticalspeed;
     double hdg;
     double rev;
     double lightlanding;
     double brake;
     double parkingbrake;
     double enginerpm1;
     double enginerpm2;
     double batt_master;
     double retr_gear;
     double light_taxi;
     double light_beacon;
     double light_nav;
     double light_strobe;
     double light_panel;
     double stall_wrn;
     double overspeed_wrn;
     double inner_marker;
     double middle_marker;
     double outer_marker;
    double main_volt;
    double flight_director_1;
    double baro_milibars;
    double baro_inhg;
    double ap_selected_airspeed;//AUTOPILOT AIRSPEED HOLD VAR
    double ap_selected_heading; //AUTOPILOT HEADING LOCK DIR
    double ap_selected_altitude;
    double ap_selected_vertspeed;
    double ap_autothrottle;
    double ap_altitude;
    char aircraft_title[256];
};

class structures_simconnect
{
public:

};

#endif // STRUCTURES_SIMCONNECT_H
