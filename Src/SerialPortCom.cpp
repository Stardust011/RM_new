//
// Created by Stardust on 2023/12/17.
//

#include "SerialPortCom.h"
#include "config.h"
#include <string>
#include <WzSerialPortPlus.h>
#include <string>
// #include <unistd.h>

// 串口配置
auto serial_port_path = config["serial_port"]["path"].as<std::string>();
auto serial_baudrate = config["serial_port"]["baudrate"].as<int>();
auto serial_stopbits = config["serial_port"]["stopbits"].as<int>();
auto serial_databits = config["serial_port"]["bytesize"].as<int>();
auto serial_parity = config["serial_port"]["parity"].as<char>();

WzSerialPortPlus serialPort;
// 初始化数据结构
serialData serical_data;

// Init serial port
bool serialInit() {
    // Init serial port
    const bool ret = serialPort.open(serial_port_path, serial_baudrate, serial_stopbits, serial_databits, serial_parity);
    setSerialDevice(serial_port_path);
    setSerialBaudrate(std::to_string(serial_baudrate));
    // std::cout << "Serial port init: " << serial_port_path << " (" << serial_baudrate << ")" << std::endl;
    // if (ret) setSerialStatus("Connected");
    // else setSerialStatus("Connect Failed");
    return ret;
}

// crc16
static unsigned short crc16(const unsigned char* data_p, const unsigned char length){
    unsigned short crc = 0xFFFF;
    for (unsigned char i = 0; i < length; i++) {
        crc ^= static_cast<unsigned short>(data_p[i] << 8);
        for (unsigned char j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                constexpr unsigned short poly = 0x1021;
                crc = (crc << 1) ^ poly;
            } else {
                crc = crc << 1;
            }
        }
    }
    return crc;
}

// Send data to serial port


// Close serial port
void serialClose() {
    serialPort.close();
}

// 通过函数修改数据
void setSerialData(const double x, const double y) {
    const int x_2_send = static_cast<int>(x * 10);
    const int y_2_send = static_cast<int>(y * 10);
    serical_data.data.x = x_2_send;
    serical_data.data.y = y_2_send;
    // 设置状态
    setSerialSend(x_2_send,y_2_send);
    serical_data.crc[0] = static_cast<unsigned char>(crc16(reinterpret_cast<unsigned char *>(&serical_data.data), sizeof(serical_data.data)) >> 8);
    serical_data.crc[1] = static_cast<unsigned char>(crc16(reinterpret_cast<unsigned char *>(&serical_data.data), sizeof(serical_data.data)) & 0xFF);
}

void setCmdStatus(const unsigned char cmd) {
    serical_data.cmd = cmd;
}

// 串口发送函数入口（多线程）
[[noreturn]] void serialSendEntry() {
    serialInit();
    // sleep(5);
    while (true) {
        setSerialStatus("Sending...");
        setSendDataPreview(reinterpret_cast<char *>(&serical_data), sizeof(serical_data));
        serialPort.send(reinterpret_cast<char *>(&serical_data), sizeof(serical_data));
    }
}
