//
// Created by Stardust on 2023/12/17.
//

#include "SerialPortCom.h"
#include "config.h"
#include <string>
#include <unistd.h>

// 串口配置
auto serial_port_path = config["serial_port"]["path"].as<std::string>();
auto serial_baudrate = config["serial_port"]["baudrate"].as<int>();
auto serial_stopbits = config["serial_port"]["stopbits"].as<int>();
auto serial_databits = config["serial_port"]["bytesize"].as<int>();
auto serial_parity = config["serial_port"]["parity"].as<char>();

WzSerialPortPlus serialPort;

// Init serial port
bool serialInit() {
    // Init serial port
    bool ret = serialPort.open(serial_port_path, serial_baudrate, serial_stopbits, serial_databits, serial_parity);
    setSerialDevice(serial_port_path);
    setSerialBaudrate(std::to_string(serial_baudrate));
    // std::cout << "Serial port init: " << serial_port_path << " (" << serial_baudrate << ")" << std::endl;
    // if (ret) setSerialStatus("Connected");
    // else setSerialStatus("Connect Failed");
    return ret;
}

// crc16
static unsigned short crc16(const unsigned char* data_p, unsigned char length){
    unsigned char x;
    unsigned short crc = 0xFFFF;

    while (length--){
        x = crc >> 8 ^ *data_p++;
        x ^= x>>4;
        crc = (crc << 8) ^ ((unsigned short)(x << 12)) ^ ((unsigned short)(x <<5)) ^ ((unsigned short)x);
    }
    return crc;
}

// Send data to serial port
bool serialSend(const char *data) {
    bool ret = serialPort.send((char *) &data, sizeof(data));
    return ret;
}

// Close serial port
void serialClose() {
    serialPort.close();
}

// 初始化数据结构
serialData serical_data;

// 通过函数修改数据
void setSerialData(double x, double y) {
    int x_2_send = (int)x*100;
    int y_2_send = (int)y*100;
    serical_data.data.x = x_2_send;
    serical_data.data.y = y_2_send;
    setSerialSend(x_2_send,y_2_send);
    serical_data.crc[0] = (unsigned char) (crc16((unsigned char *) &serical_data.data, sizeof(serical_data.data)) >> 8);
    serical_data.crc[1] = (unsigned char) (crc16((unsigned char *) &serical_data.data, sizeof(serical_data.data)) & 0xFF);
}

// 串口发送函数入口（多线程）
void serialSendEntry() {
    serialInit();
    // sleep(5);
    while (true) {
        serialSend((char *) &serical_data);
        // std::cout << "Serial port send: " << serical_data.data.x << " " << serical_data.data.y << std::endl;
        setSerialStatus("Sending...");
    }
}
