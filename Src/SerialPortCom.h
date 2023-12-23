//
// Created by Stardust on 2023/12/17.
//

#ifndef SERIALPORTCOM_H
#define SERIALPORTCOM_H

bool serialInit();
static unsigned short crc16(const unsigned char* data_p, unsigned char length);
bool serialSend(const char *data, int length);
void serialClose();


struct pos_data {
    int x{0}; // 与视野中心点的偏移量
    int y{0}; // (x,y)都扩大10倍
};

struct serialData {
    unsigned char head[2]{0xAA, 0xDD};  // 帧头
    unsigned char cmd{0x01};  // 标志位(0为未识别，1为识别)
    unsigned char length{8};  // 数据长度
    pos_data data;  // 数据
    unsigned char crc[2]{0x00, 0x00};  // CRC-16-CCITT-FALSE 校验
    unsigned char end[2]{0x0D, 0x0A};  // 帧尾
};

void setCmdStatus(unsigned char cmd);
void setSerialData(double x, double y);
void serialSendEntry();

#endif //SERIALPORTCOM_H
