//
// Created by Stardust on 2023/12/21.
//

#include "tui.h"
#include <thread>
#include <utility>
#include <sstream>
#include <iomanip>
#include <curses.h>
using namespace std;

string opencv_device = "Finding...";
string camera_exposure = "Finding...";
string serial_device = "Finding...";
string camera_status = "Finding...";
string discern_status = "Finding...";
string serial_status = "Finding...";
string serial_baudrate = "Finding...";
string serial_send = "Finding...";
string discern_direction = "Finding...";
string time_cost = "...";
string send_data_preview = "...";

void setOpencvDevice(string device) {
    opencv_device = std::move(device);
}

void setCameraExposure(string exposure) {
    camera_exposure = std::move(exposure);
}

void setSerialDevice(string device) {
    serial_device = std::move(device);
}

void setCameraStatus(string status) {
    camera_status = std::move(status);
}

void setDiscernStatus(string status) {

    discern_status = std::move(status);
}

void setSerialStatus(string status) {
    serial_status = std::move(status);
}

void setSerialBaudrate(string baudrate) {
    serial_baudrate = std::move(baudrate);
}

void setSerialSend(const int send_x, const int send_y) {
    // 转换为字符串,补足零为三位整数
    ostringstream out_x;
    out_x << fixed << setprecision(0) << setw(6)  << send_x;
    ostringstream out_y;
    out_y << fixed << setprecision(0) << setw(6)  << send_y;
    serial_send = "( x:" + out_x.str() + ", y:" + out_y.str() + " )";
}

void setDiscernDirection(double x, double y, double r) {
    // 转换为字符串,补足零为三位整数
    ostringstream out_x;
    out_x << fixed << setprecision(3) << setw(8)  << x;
    ostringstream out_y;
    out_y << fixed << setprecision(3) << setw(8)  << y;
    ostringstream out_r;
    out_r << fixed << setprecision(3) << setw(8)  << r;
    discern_direction = "( x:" + out_x.str() + ", y:" + out_y.str() + ", r:" + out_r.str() + " )";
}

void setTimeCost(const double cost) {
    // 转换为字符串,补足零为三位整数
    ostringstream out_cost;
    out_cost << fixed << setprecision(5) << setw(8)  << cost;
    time_cost = out_cost.str() + " s";
}

void setSendDataPreview(const char *data , const int length) {
    // 转换为字符串,HEX显示
    ostringstream out_data;
    for (int i = 0; i < length; i++) {
        out_data << uppercase << hex << setw(2) << setfill('0') << (static_cast<unsigned>(data[i])&0xff) << " ";
    }
    send_data_preview = out_data.str();
}

[[noreturn]] void displayStatus() {
    // 初始化ncurses
    initscr();
    // 不显示输入的字符
    noecho();

    // 获取窗口的大小
    int height, width;
    getmaxyx(stdscr, height, width);
    int half_height = height / 2;
    if (half_height < 7) { half_height = 7;} // 防止窗口过小
    int half_width = width / 2;

    // 添加分割线
    mvhline(half_height, 0, ACS_HLINE, width);

    while (true) {
        // 在窗口中显示一些文本
        // 分割线上方
        // 摄像头地址
        mvprintw(1, 1, ("Opencv device: " + opencv_device).c_str());
        // 摄像头状态
        // 清除上一次的内容
        move(2,1);
        clrtoeol();
        mvprintw(2, 1, ("Camera status: " + camera_status).c_str());
        // 摄像头曝光度
        move(3,1);
        clrtoeol();
        mvprintw(3, 1, ("Camera exposure: " + camera_exposure).c_str());
        // 识别状态
        move(4,1);
        clrtoeol();
        mvprintw(4, 1, ("Discern status: " + discern_status).c_str());
        // 识别方位
        mvprintw(5, 1, ("Discern direction: " + discern_direction).c_str());
        // 耗时
        mvprintw(6, 1, ("Time cost: " + time_cost).c_str());

        // 分割线下方
        // 串口地址
        mvprintw(half_height+1, 1, ("Serial Port device: " + serial_device).c_str());
        // 串口波特率
        // 清除上一次的内容
        move(half_height+2,1);
        clrtoeol();
        mvprintw(half_height+2, 1, ("Serial Port baudrate: " + serial_baudrate).c_str());
        // 串口状态
        mvprintw(half_height+3, 1, ("Serial Port status: " + serial_status).c_str());
        // 串口发送数据
        mvprintw(half_height+4, 1, ("Serial Port send: " + serial_send).c_str());
        // 数据预览
        move(half_height+5,1);
        clrtoeol();
        mvprintw(half_height+5, 1, ("Data preview: " + send_data_preview).c_str());

        // 最后两行
        // 按键提示
        mvprintw(half_height+7, 1, "Press 'Ctrl + C' to quit.");
        refresh();
        // 每10毫秒刷新一次
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
