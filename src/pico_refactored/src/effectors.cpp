/*
 * ApiaryGuard - Effectors Implementation
 */

#include "effectors.h"

void setHeaterPWM(uint8_t duty) {
    analogWrite(HEATER_PWM, duty);
}

void setFanPWM(uint8_t duty) {
    analogWrite(FAN_PWM, duty);
}

void setPump(bool state) {
    digitalWrite(PUMP_RELAY, state ? HIGH : LOW);
}

void setValve1(bool state) {
    digitalWrite(VALVE_1, state ? HIGH : LOW);
}

void setValve2(bool state) {
    digitalWrite(VALVE_2, state ? HIGH : LOW);
}

void setRelay(uint8_t channel, bool state) {
    switch(channel) {
        case 1: digitalWrite(RELAY_CH1, state ? HIGH : LOW); break;
        case 2: digitalWrite(RELAY_CH2, state ? HIGH : LOW); break;
        case 3: digitalWrite(RELAY_CH3, state ? HIGH : LOW); break;
        case 4: digitalWrite(RELAY_CH4, state ? HIGH : LOW); break;
        case 5: digitalWrite(RELAY_CH5, state ? HIGH : LOW); break;
        case 6: digitalWrite(RELAY_CH6, state ? HIGH : LOW); break;
        case 7: digitalWrite(RELAY_CH7, state ? HIGH : LOW); break;
        case 8: digitalWrite(RELAY_CH8, state ? HIGH : LOW); break;
    }
}
