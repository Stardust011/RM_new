//
// Created by Stardust on 2023/12/17.
//

#ifndef SERIALPORTCOM_H
#define SERIALPORTCOM_H

#include <WzSerialPortPlus.h>
#include <string>

bool serialInit();
static unsigned short crc16(const unsigned char* data_p, unsigned char length);
bool serialSend(const char *data, int length);
void serialClose();


struct pos_data {
    int x{0};
    int y{0};
    // unsigned char z[4]{0x00, 0x00, 0x00, 0x00};
    // unsigned char yaw[4]{0x00, 0x00, 0x00, 0x00};
    // unsigned char pitch[4]{0x00, 0x00, 0x00, 0x00};
    // unsigned char roll[4]{0x00, 0x00, 0x00, 0x00};
};

struct serialData {
    unsigned char head{0x40};
    // unsigned char id{0x01};
    unsigned char cmd{0x01};
    unsigned int length{20};
    pos_data data;
    unsigned char crc[2]{0x00, 0x00};
    unsigned char end[2]{0x0D, 0x0A};
};

void setCmdStatus(unsigned char cmd);
void setSerialData(double x, double y);
void serialSendEntry();

#endif //SERIALPORTCOM_H
