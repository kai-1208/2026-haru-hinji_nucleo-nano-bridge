#include "led_controller.hpp"

LedController::LedController(PinName tx_pin, PinName rx_pin)
    : _nano_serial(tx_pin, rx_pin) {
}

void LedController::setup() {
    _nano_serial.set_baud(9600);
}

void LedController::sendLedState(LedState state, const bool* bool_states, int num_bools) {
    char buffer[128];
    int offset = 0;

    // LedStateを先頭に追加 (charとして)
    offset += sprintf(buffer + offset, "%c", static_cast<char>(state));

    // bool値を追加
    for (int i = 0; i < num_bools; ++i) {
        offset += sprintf(buffer + offset, ",%d", bool_states[i] ? 1 : 0);
    }

    // 終端文字を追加
    offset += sprintf(buffer + offset, "\n");

    if (offset > 0) {
        _nano_serial.write(buffer, offset);
    }
}