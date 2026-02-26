#include "mbed.h"
#include "led_controller.hpp"
#include "state_comms.h"

BufferedSerial pc(USBTX, USBRX);
CAN can1(PA_11, PA_12);
CAN can2(PB_12, PB_13);

LedController led(PA_9, PA_10);
StateComms comms(D0, D1, 2, 9600); // tx, rx, 送信するデータ数, ボーレート

LedState curr_state = LedState::OFF;
LedState prev_state = LedState::OFF;

bool can_status1[2] = {false}; // false: can生きてる, true: can死んでる, [0]: CAN1, [1]: CAN2
int can_error_count[2] = {0};

bool can_status2[2] = {false}; // false: can生きてる, true: can死んでる, [0]: CAN1, [1]: CAN2
bool uart_status = false; // false: uart生きてる, true: uart死んでる

bool is_completed = false; // 条件達成時true

bool multi_states[5] = {false}; // [0]: sub uart, [1]: main can1, [2]: main can2, [3]: sub can1, [4]: sub can2

int main() {
    mbed::HighResClock::time_point now_time = HighResClock::now ();
    mbed::HighResClock::time_point curr_time = HighResClock::now ();
    led.setup();
    while(1) {
        CANMessage msg1;
        CANMessage msg2;
        if (now_time - curr_time >= 1ms) { // 1000Hz
            curr_time = now_time;
            // sub nucleoのcan状態
            if (can1.read(msg1)) {
                can_error_count[0] = 0;
            } else {
                can_error_count[0]++;
            }
            if (can2.read(msg2)) {
                can_error_count[1] = 0;
            } else {
                can_error_count[1]++;
            }
            can_status1[0] = (can_error_count[0] > 50);
            can_status1[1] = (can_error_count[1] > 50);

            // main nucleoのcan, uart状態
            bool received_status[2] = {false};
            if (comms.read(received_status)) {
                uart_status = false;
                can_status2[0] = received_status[0];
                can_status2[1] = received_status[1];
            } else {
                uart_status = true;
            }

            // 状態の決定
            if (uart_status) {
                curr_state = LedState::UARTLost;
            } else if (can_status1[0] || can_status1[1] || can_status2[0] || can_status2[1]) {
                curr_state = LedState::CANLost;
            } else {
                curr_state = LedState::Normal;
            }
            if (is_completed) {
                curr_state = LedState::Clear;
            }

            // multi_statesの更新
            multi_states[0] = uart_status;
            multi_states[1] = can_status1[0];
            multi_states[2] = can_status1[1];
            multi_states[3] = can_status2[0];
            multi_states[4] = can_status2[1];

            // 状態送信
            if (curr_state != prev_state) {
                led.sendLedState(curr_state, multi_states, 5);
                prev_state = curr_state;
            }
        }
    }
}