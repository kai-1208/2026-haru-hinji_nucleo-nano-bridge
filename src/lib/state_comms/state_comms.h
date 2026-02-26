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
    StateComms(PinName tx, PinName rx, int num_states, int baud = 9600) 
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
        if (!_serial.readable()) {
            return false;
        }

        char c;
        ssize_t n = _serial.read(&c, 1);
        
        if (n > 0) {
            if (c == '\n') {
                _rx_buffer[_rx_index] = '\0';
                _rx_index = 0;

                char* token = strtok(_rx_buffer, ",");
                int count = 0;
                
                // 一時的に値を保持する配列
                // (パース失敗時に元のデータを壊さないため)
                bool temp_values[_num_states];

                while (token != NULL && count < _num_states) {
                    // "0"以外ならtrue、"0"ならfalseとして解釈
                    int val = atoi(token);
                    temp_values[count] = (val != 0);
                    
                    token = strtok(NULL, ",");
                    count++;
                }

                // 数が一致した場合のみ採用
                if (count == _num_states) {
                    for(int i=0; i<_num_states; i++){
                        out_states[i] = temp_values[i];
                    }
                    return true;
                }

            } else {
                if (_rx_index < sizeof(_rx_buffer) - 1) {
                    _rx_buffer[_rx_index++] = c;
                } else {
                    _rx_index = 0; // オーバーフローリセット
                }
            }
        }
        return false;
    }
};

#endif