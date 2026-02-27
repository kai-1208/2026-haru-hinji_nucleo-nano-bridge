#include "mbed.h"
#include "led_controller.hpp"
#include "state_comms.h"

BufferedSerial pc(USBTX, USBRX);
CAN can1(PA_11, PA_12);
CAN can2(PB_12, PB_13);

LedController led(PA_0, PA_1);
StateComms comms(D0, D1, 2, 9600); // tx, rx, 送信するデータ数, ボーレート

LedState curr_state = LedState::OFF;
LedState prev_state = LedState::OFF;

DigitalIn limit_sw1(PC_10), limit_sw2(PC_11), limit_sw3(PC_12), limit_sw4(PC_0), limit_sw5(PC_1), limit_sw6(PC_2);

bool can_status1[2] = {false}; // false: can生きてる, true: can死んでる, [0]: CAN1, [1]: CAN2
int can_error_count[2] = {0};
bool uart_status = false; // false: uart生きてる, true: uart死んでる
int uart_error_count = 0;

bool can_status2[2] = {false}; // false: can生きてる, true: can死んでる, [0]: CAN1, [1]: CAN2

bool is_completed = false; // 条件達成時true

bool multi_states[12] = {false}; // [0-5] false: 正常（緑点灯）, true: 異常（赤点滅） リミットスイッチ[6-11] false: sw押されている（黄点灯）, true: 何もなし（緑点灯）

int main() {
    led.setup();
    limit_sw1.mode(PullUp);
    limit_sw2.mode(PullUp);
    limit_sw3.mode(PullUp);
    limit_sw4.mode(PullUp);
    limit_sw5.mode(PullUp);
    limit_sw6.mode(PullUp);

    mbed::HighResClock::time_point now_time = HighResClock::now ();
    mbed::HighResClock::time_point curr_time = HighResClock::now ();
    while(1) {
        CANMessage msg1;
        CANMessage msg2;
        if (now_time - curr_time >= 1ms) { // 1000Hz
            curr_time = now_time;
            // sub nucleoのcan状態
            if (can1.write(msg1)) {
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
                uart_error_count++;
                if (uart_error_count > 100) {
                    uart_status = true;
                }
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
            multi_states[0] = can_status1[0];      // main nucleoのcan1
            multi_states[1] = can_status1[1];      // main nucleoのcan2
            multi_states[2] = uart_status;         // main nucleoのuart
            multi_states[3] = can_status2[0];      // sub nucleoのcan1
            multi_states[4] = can_status2[1];      // sub nucleoのcan2
            multi_states[5] = false;               // sub nucleoのuart（arduino側で検知）

            // センサの状態の更新 押されたらtrue: 黄点灯, false: 緑点灯
            multi_states[6] = !limit_sw1.read();   // limitsw1
            multi_states[7] = !limit_sw2.read();   // limitsw2
            multi_states[8] = !limit_sw3.read();   // limitsw3
            multi_states[9] = !limit_sw4.read();   // limitsw4
            multi_states[10] = !limit_sw5.read();  // limitsw5
            multi_states[11] = !limit_sw6.read();  // limitsw6

            // 状態送信
            if (curr_state != prev_state) {
                prev_state = curr_state;
            }
            led.sendLedState(curr_state, multi_states, 12);

            printf("can1: %d, can2: %d, uart: %d, ", multi_states[3], multi_states[4], multi_states[2]);
            printf("sw1: %d, sw2: %d, sw3: %d, sw4: %d, sw5: %d, sw6: %d\n", multi_states[6], multi_states[7], multi_states[8], multi_states[9], multi_states[10], multi_states[11]);
        }
    }
}