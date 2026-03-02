#ifndef STATE_COMMS_H
#define STATE_COMMS_H

#include "mbed.h"
#include <cstdio>
#include <cstdlib>

class StateComms {
private:
    BufferedSerial _serial;
    const int _num_states; // 送受信する状態の数
    char _rx_buffer[128];
    int _rx_index;

public:
    /**
     * コンストラクタ
     * @param tx 送信ピン
     * @param rx 受信ピン
     * @param num_states 送受信するbool値の数
     * @param baud ボーレート
     */
    StateComms(PinName tx, PinName rx, int num_states, int baud) 
        : _serial(tx, rx), _num_states(num_states) {
        _serial.set_baud(baud);
        _rx_index = 0;
    }

    /**
     * bool状態配列を送信する
     * @param states 送信するbool配列
     */
    void send(const bool* states) {
        char buffer[128];
        int offset = 0;

        for (int i = 0; i < _num_states; i++) {
            // trueなら"1", falseなら"0" に変換してカンマ区切りにする
            int val = states[i] ? 1 : 0;
            
            if (i < _num_states - 1) {
                offset += sprintf(buffer + offset, "%d,", val);
            } else {
                offset += sprintf(buffer + offset, "%d\n", val);
            }
        }

        if (offset > 0) {
            _serial.write(buffer, offset);
        }
    }

    /**
     * データを受信し、bool配列に格納する
     * @param out_states 受信データを格納するbool配列
     * @return 更新成功ならtrue
     */
    bool read(bool* out_states) {
        bool updated = false;

        // データがある限り読み続ける
        while (_serial.readable()) {
            char c;
            if (_serial.read(&c, 1) > 0) {
                // printf("%c", c);
                // 改行コードでパース実行
                if (c == '\n') {
                    _rx_buffer[_rx_index] = '\0';
                    // printf("RX: %s\n", _rx_buffer);
                    _rx_index = 0;

                    // --- パース処理 ---
                    char* token = strtok(_rx_buffer, ",");
                    int count = 0;
                    bool temp_values[_num_states]; // 一時バッファ

                    while (token != NULL && count < _num_states) {
                        temp_values[count] = (atoi(token) != 0);
                        token = strtok(NULL, ",");
                        count++;
                    }

                    if (count == _num_states) {
                        for(int i=0; i<_num_states; i++){
                            out_states[i] = temp_values[i];
                        }
                        updated = true; // 更新フラグを立てる
                        // ここでreturn trueすると、バッファに残ったゴミデータで次回ずれる可能性があるので
                        // 最新のデータを取り切るまでループを回し続けてもよいが、
                        // 一旦ここで return true して処理を返したほうが反応が良い場合が多い。
                        return true; 
                    } else {
                        // printf("Count mismatch: %d != %d\n", count, _num_states);
                    }
                } else {
                    // バッファへの蓄積
                    if (_rx_index < sizeof(_rx_buffer) - 1) {
                        _rx_buffer[_rx_index++] = c;
                    } else {
                        _rx_index = 0; // オーバーフロー時はリセット
                    }
                }
            }
        }
        return updated;
    }
};

#endif