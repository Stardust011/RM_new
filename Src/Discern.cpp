//
// Created by Stardust on 2023/12/17.
//

#include "Discern.h"

#include <thread>

#include "Camera.h"
#include "config.h"
#include "SerialPortCom.h"
using namespace cv;

// 获取摄像头参数
const auto camera_device = config["camera"]["device"].as<std::string>();
const auto camera_thread_number = config["thread"]["frame"].as<int>();
const auto discern_thread_number = config["thread"]["discern"].as<int>();
const auto queue_size = config["thread"]["queue_size"].as<int>();
const auto armour_color = config["armour_color"].as<int>();
// auto camera_thread_number = 1;
// auto discern_thread_number = 2;

std::mutex g_mutex;
std::queue<Mat> frames;//先进先出队列

// 赋予识别位置初值
auto prev_point_fire = Point(0, 0);
auto prev_distance = 0.0;

double distance_single = 0;
auto target_flags = false;


// 设置摄像头对象
VideoCapture cap;

// 打开摄像头
void Camera_Open() {
    if (std::all_of(camera_device.begin(), camera_device.end(), ::isdigit)) {
        cap.open(std::stoi(camera_device));
    } else {
        cap.open(camera_device);
    }
}


// 读取摄像头帧
void readFrame(VideoCapture cap)
{
    // 保持frames队列的大小不超过queue_size
    while(frames.size()>=queue_size) {
        g_mutex.lock();//修改公共资源，用进程锁锁定
        frames.pop();
        g_mutex.unlock();
    };
    Mat frame2;
    cap >> frame2;//bool judge=cap.read(frame2);//先别管帧有没有读取成功
    g_mutex.lock();//修改公共资源，用进程锁锁定
    frames.push(frame2);
    g_mutex.unlock();
}


// 显示摄像头帧
void showFrame(const Mat& frame) {
    imshow("test", frame);
    waitKey(1);
}

// ---莫名的参数---
int Polyfit_Get(const double distance) {
    const int Get_Ployfit =
            69.42 * pow(distance, 3) - 6.453 * pow(distance, 4) - 248.4 * pow(distance, 2) + 273 * distance + 192;
    return Get_Ployfit;
}

// 获取新的ROI区域
Rect get_roi_rect(bool new_flags) {
    Rect roi_rect;
    // TODO：add roi info to tui
    // 默认ROI区域
    if (new_flags) {
        roi_rect.x = 0;
        roi_rect.y = 0;
        roi_rect.width = 640;
        roi_rect.height = 480;
        return roi_rect;
    }
    // 为了测试方便，暂时不加入新的ROI区域
    roi_rect.x = 0;
    roi_rect.y = 0;
    roi_rect.width = 640;
    roi_rect.height = 480;
    return roi_rect;
    // 测试代码结束！

    // 根据距离设置ROI宽度
    int Roi_width;
    if (distance_single < 0.5)
        Roi_width = 280;
    else
        Roi_width = Polyfit_Get(distance_single) + 20;
    // 默认高度
    int Roi_height = 150;
    roi_rect.x = 320 - Roi_width / 2.0;
    roi_rect.y = 240 - Roi_height / 2.0;
    roi_rect.width = Roi_width;
    roi_rect.height = Roi_height;
    return roi_rect;
}


// 判断ROI区域是否符合要求
bool judge_satisfy(const Mat &Roi) {
    // std::cout << "size: height:" << Roi.rows << " width: " << Roi.cols << std::endl;
    // std::cout << "percent : " << (double) Roi.rows / Roi.cols << std::endl;

    if (static_cast<double>(Roi.rows) / Roi.cols < 1.40 || static_cast<double>(Roi.rows) / Roi.cols > 4.00)
        return false;
    /*if((double) Roi.rows / Roi.cols < 1.0)
        return false;
    /*if (Roi.rows > 200 || Roi.cols > 200)
        return false;*/
    /*imshow("roi", Roi);
    waitKey();*/
    return true;
}


// 腐蚀
Mat erode_pic(const Mat &src_image, const int size_element = 1) {
    Mat dst_image(src_image.rows, src_image.cols, CV_8UC1);
    const Mat kernel = getStructuringElement(MORPH_RECT, Size(size_element, size_element));
    erode(src_image, dst_image, kernel);
    return dst_image;
}


// 膨胀
Mat dilate_pic(const Mat &src_image, const int size_element = 3) {
    Mat dst_image(src_image.rows, src_image.cols, CV_8UC1);
    const Mat kernel = getStructuringElement(MORPH_RECT, Size(size_element, size_element));
    dilate(src_image, dst_image, kernel);
    return dst_image;
}


// 图像预处理
Mat convertBGR2HSV2Gray(const Mat &src_image,
        int h_min = 0, int h_max = 255, int s_min = 0, int s_max = 255, int v_min = 0, int v_max = 255) {
    Mat hsv_image;
    cvtColor(src_image, hsv_image, COLOR_BGR2HSV);
    switch (armour_color) {
        case MODE_BLUE:
            h_min = 45;
            h_max = 205;
            s_min = 132;
            s_max = 255;
            v_min = 136;
            v_max = 255;
            break;
        case MODE_RED:
            h_min = 140;
            h_max = 189;
            s_min = 160;
            s_max = 255;
            v_min = 30;
            v_max = 223;
            break;
        case MODE_NONE:
            // do nothing
            break;
        default :
            // TODO：给予提示，非法目标颜色参数
            break;
    }
    Mat dst_image;
    inRange(hsv_image, Scalar(h_min, s_min, v_min), Scalar(h_max, s_max, v_max), dst_image);
    return dst_image;
}


/*
函数名称:get_rect_pic_contour
传入参数:二值化后的图片,轮廓描绘的点的容器,用于保存合格的轮廓的容器
传出变量:空
函数作用:用于剔除轮廓中不合格的*/
void get_rect_pic_contour(const Mat &gray_image, const std::vector<std::vector<Point>> &contour_vector,
                          std::vector<std::vector<Point>> &contour_rect)//4
{
    //cout << "enter get_rect_pic_contour func" << endl;
    for (const auto & num : contour_vector) {
        Rect tmp_contour = boundingRect(num);
        //display("ROI", ROI);
        if (Mat ROI = gray_image(tmp_contour); !judge_satisfy(ROI)) {
            continue;
        }
        contour_rect.push_back(num);
    }
}


// 获取轮廓
std::vector<std::vector<Point>> get_point_contours(const Mat &src_image)
{
    std::vector<std::vector<Point>> contour_vector;
    std::vector<Vec4i> contour_hierachy;
    // cout << "hello1 " << endl;
    findContours(src_image, contour_vector, contour_hierachy, RETR_CCOMP, CHAIN_APPROX_TC89_L1);
    // cout << "hello2" << endl;
    //cout << "contour_vector size:" << contour_vector.size() << endl;
    return contour_vector;
}


// 计算并返回一组矩形的旋转角度
std::vector<double> get_rotate_angle(const Mat &src_image, const std::vector<std::vector<Point>> &rotate_rect) {
    std::vector<double> theata_vector;
    for (const auto & num : rotate_rect) {
        int flags = 0;
        const Rect tmp_rect = boundingRect(num);
        Point left_up(src_image.cols, tmp_rect.y);
        Point left_down(src_image.cols, tmp_rect.y + tmp_rect.height);
        for (const auto obj_num : num) {
            if (obj_num.y == tmp_rect.y)
                if (obj_num.x < left_up.x)
                    left_up.x = obj_num.x;
            if (obj_num.y == tmp_rect.y + tmp_rect.height - 1)
                if (obj_num.x < left_down.x)
                    left_down.x = obj_num.x;
        }
        if (left_down.x > left_up.x) flags = 1;
        //circle(src_image,left_up,1,Scalar(0,0,255),2,8);
        //circle(src_image,left_down,1,Scalar(0,255,255),2,8);
        const double gradient = fabs(static_cast<double>(left_down.y - left_up.y)) / fabs(static_cast<double>(left_down.x - left_up.x));
        double theata = atan(gradient) * 180.0 / 3.1415926;
        if (flags == 1) theata = 180 - theata;
        theata_vector.push_back(theata);
    }
    return theata_vector;
}


/*
传入参数:所获得的各个轮廓的角度
传出变量:各个函数的角度的差值和各个轮廓在容器中的位置
函数作用:算出两两轮廓之间角度的变化值
说明:Scalar中第一个val中的第一个变量的值是两个对比的轮廓的角度
    第二个变量是对比的轮廓在轮廓容器中的排列位置
    第三个变量是对比的第二个轮廓在容器中的排列位
    该函数中不存在筛选函数调用*/
std::vector<Scalar> cal_distance_theata(const std::vector<double> &theata_vector) {
    //cout << "enter cal_distance_theata function" << endl;
    std::vector<Scalar> tmp_Scalar_vector;
    if (theata_vector.empty() || theata_vector.size() == 1)//如果发现角度容器中存放的没有或者只有一个轮廓的角度,那么这时候返回一空容器
    {
        return tmp_Scalar_vector;
    }
    for (int out_side = 0; out_side < theata_vector.size() - 1; out_side++)//遍历整个容器进行一一对比
    {
        for (int index_side = out_side + 1; index_side < theata_vector.size(); index_side++) {
            const double distance_theata = fabs(theata_vector[out_side] - theata_vector[index_side]);//两者角度差值的绝对值定义为两个轮廓之间的距离
            Scalar tmp_scalar(distance_theata, out_side, index_side);
            tmp_Scalar_vector.push_back(tmp_scalar);
        }
    }
    return tmp_Scalar_vector;
}


/*
传入参数:获得的各个轮廓的角度差 最大允许范围角度
传出变量:经过筛选后的轮廓的角度差
函数作用:筛选,将角度差大于指定值的角度剔除
说明:Scalar中第一个val中的第一个变量的值是两个对比的轮廓的角度
    第二个变量是对比的轮廓在轮廓容器中的排列位置
	第三个变量是对比的第二个轮廓在容器中的排列位
 	该函数中不存在筛选函数调用*/
std::vector<Scalar> judge_the_theata(const std::vector<Scalar> &angle_Scalar, const double max_angle = 20) {
    //cout << "enter judge the theata func" << endl;
    //cout << "angle_Scalar size is: " << angle_Scalar.size() << endl;
    std::vector<Scalar> tmp_vector_angle;
    for (const auto & num : angle_Scalar) {
        // cout << "angle is : " << num.val[0] << endl;
        if (num.val[0] > max_angle) continue;
        tmp_vector_angle.push_back(num);
    }
    return tmp_vector_angle;
}


/*
函数名称:judge_two_sides
传入参数:一对配对的轮廓所构成的点的容器A 一对配对的轮廓所构成的点的容器B
传出变量:轮廓是否符合要求
函数作用:通过判断一对配对的轮廓两端最长的距离来判断这两个轮廓是否符合要求
*/
bool judge_two_sides(const std::vector<Point> &point_A_vector, const std::vector<Point> &point_B_vector) {
    double a_max_distance = 0;
    double b_max_distance = 0;
    double tmp_distance;
    //cout << "size_A:" << point_A_vector.size() << "B:" << point_B_vector.size() << endl;
    //找到组成轮廓A的最长的距离
    for (int obj_num = 0; obj_num < point_A_vector.size() - 1; obj_num++) {
        for (int index_num = obj_num; index_num < point_A_vector.size(); index_num++) {
            tmp_distance = sqrt(pow(static_cast<double>(point_A_vector[obj_num].x - point_A_vector[index_num].x), 2) +
                                pow(static_cast<double>(point_A_vector[obj_num].y - point_A_vector[index_num].y), 2));
            if (tmp_distance > a_max_distance)
                a_max_distance = tmp_distance;
        }
    }
    //找到组成轮廓B的最长的距离
    for (int obj_num = 0; obj_num < point_B_vector.size() - 1; obj_num++) {
        for (int index_num = obj_num; index_num < point_B_vector.size(); index_num++) {
            tmp_distance = sqrt(pow(static_cast<double>(point_B_vector[obj_num].x - point_B_vector[index_num].x), 2) +
                                pow(static_cast<double>(point_B_vector[obj_num].y - point_B_vector[index_num].y), 2));
            if (tmp_distance > b_max_distance)
                b_max_distance = tmp_distance;
        }
    }

    if (a_max_distance / b_max_distance < 0.69 || a_max_distance / b_max_distance > 1.44 ||
        b_max_distance / a_max_distance > 1.44 || b_max_distance / a_max_distance < 0.69) {
        return false;
    }
    // cout << "a_max_distance : " << a_max_distance << " b_max_distance:" << b_max_distance << endl;
    // cout << "a/b:" << a_max_distance / b_max_distance << "b/a" << b_max_distance / a_max_distance << endl;
    // cout << "|a-b| " << fabs(a_max_distance - b_max_distance) << endl;
    //waitKey();
    return true;
}


// 四点计算矩形
Rect cal_the_rect_by_point(const std::vector<Point> &point_vector, int init_rows = 480, const int init_cols = 640) {
    int min_x = init_cols, min_y = init_rows;
    int max_x = 0, max_y = 0;
    for (const auto obj_num : point_vector) {
        if (min_x > obj_num.x) min_x = obj_num.x;
        if (min_y > obj_num.y) min_y = obj_num.y;
        if (max_x < obj_num.x) max_x = obj_num.x;
        if (max_y < obj_num.y) max_y = obj_num.y;
    }
    const Rect tmp_rect(Point(min_x, min_y), Point(max_x, max_y));
    return tmp_rect;
}


// 判断y轴角度是否合法
bool judge_y_axis(const Mat &src_image, const Rect &A_rect, const Rect &B_rect, const int judge_angle = 18) {
    // 绘图
    // Mat dst_image = src_image.clone();
    // rectangle(dst_image, A_rect, Scalar(255, 255, 255), 2, 8);
    // rectangle(dst_image, B_rect, Scalar(255, 255, 255), 2, 8);
    //imshow("debug",dst_image);

    // 计算角度
    const double gradient = fabs((static_cast<double>(A_rect.y) - B_rect.y) / (A_rect.x - B_rect.x));
    if (const double angle = atan(gradient) * 180.0 / 3.1415926; angle > judge_angle) return false;
    // cout << "theata : " << angle << endl;
    /*cout << "b_min - a_max:" << B_y_min - A_y_max << endl;
    cout << "A_y_max:" << A_y_max << " A_y_min:" << A_y_min << endl;
    cout << "B_y_max:" << B_y_max << " B_y_min:" << B_y_min << endl;*/
    //waitKey();
    return true;
}

/*
函数名称:set_the_rect
传入参数:二值图 含有不同轮廓角度差的容器 灰度图中所找到轮廓的点的容器 预测到的可能装甲板的容器 图片的高度 图片的宽度
传出变量:空
函数作用:根据筛选到的配对好的轮廓的对数来找到对应的正方形*/
void set_the_rect(const Mat &gray_image, const std::vector<Scalar> &place_info,
                          const std::vector<std::vector<Point>> &contour_point_vector, std::vector<Rect> &armour_place_info,
                          int pic_max_rows = 480, int pic_max_cols = 640) {
    //double debug_time = get_sys_time();
    for (const auto & num : place_info) {
        std::vector<Point> tmp_armour;
        const int out_side = static_cast<int>(num.val[1]);
        const int in_side = static_cast<int>(num.val[2]);
        // Rect tmp_a_rect, tmp_b_rect;
        // tmp_a_rect = boundingRect(contour_point_vector[out_side]);
        // tmp_b_rect = boundingRect(contour_point_vector[in_side]);
        // Mat a_image = gray_image(tmp_a_rect);
        // Mat b_image = gray_image(tmp_b_rect);
        // imshow("a", a_image);
        // imshow("b", b_image);
        if (!judge_two_sides(contour_point_vector[out_side], contour_point_vector[in_side])) continue;
        //if (!judge_y_axis(contour_point_vector[out_side],contour_point_vector[in_side],gray_image.rows)) continue;
        if (!judge_y_axis(gray_image, boundingRect(contour_point_vector[out_side]),
                          boundingRect(contour_point_vector[in_side])))
            continue;
        for (auto out_num : contour_point_vector[out_side]) {
            tmp_armour.push_back(out_num);
        }
        for (auto in_num : contour_point_vector[in_side]) {
            tmp_armour.push_back(in_num);
        }
        armour_place_info.push_back(cal_the_rect_by_point(tmp_armour, pic_max_rows, pic_max_cols));
        //display("debug", gray_image(cal_the_rect_by_point(tmp_armour, pic_max_rows, pic_max_cols)));
        //waitKey();

    }
    //debug_time = get_sys_time() - debug_time;
    //cout << "debug_time : " << debug_time / getTickFrequency() << endl;
}


// 根据一组矩形的轮廓来筛选出可能的装甲板区域
std::vector<Rect> judge_contour(const Mat& src_image, const std::vector<Rect> &rect_vector) {
    int circle_num = 0;
    std::vector<int> contour_num_vector;
    int count_contour = 0;
    // cout << "size : " << rect_vector.size() << endl;
    //cvtColor(src_image,src_image,CV_BGR2GRAY);
    // 对于每一个矩形，函数首先检查该矩形的高度和宽度是否为0，
    // 如果是，那么函数将circle_num加1，并跳过当前的循环。
    // 否则，函数将该矩形从源图像中切割出来，然后在这个矩形上找到所有的轮廓，
    // 并将这些轮廓的数量添加到contour_num_vector中。
    for (auto obj_num : rect_vector) {
        //cout << "src_size: " << src_image.rows << "src_height : "<< src_image.cols << endl;
        //cout << "rect_vector.width" << rect_vector[obj_num].x  << "," << rect_vector[obj_num].width+rect_vector[obj_num].x << endl;
        //cout << "rect_vector.height:" << rect_vector[obj_num].y << "," << rect_vector[obj_num].height + rect_vector[obj_num].y << endl;
        if (obj_num.height == 0) {
            //cout << "wrong" << endl;
            //exit( -1 );
            circle_num++;
            continue;
        }
        if (obj_num.width == 0) {
            circle_num++;
            continue;
        }
        Mat ROI_pic = src_image(obj_num);
        std::vector<std::vector<Point>> contour_vector;
        std::vector<Vec4i> contour_hierachy;
        // cout << "hello" << endl;
        findContours(ROI_pic, contour_vector, contour_hierachy, RETR_EXTERNAL, CHAIN_APPROX_TC89_L1);
        //CV_LINK_RUNS may can be used in cv4
        //imshow("roi_pic",ROI_pic);

        // cout << "contour size:" << contour_vector.size() << endl;
        //waitKey();
        count_contour += contour_vector.size();
        contour_num_vector.push_back(contour_vector.size());
        //cout << "contour_size:" << contour_vector.size() << endl;
    }

    const double ave = static_cast<double>(count_contour) / (rect_vector.size() - circle_num);
    // cout << "ave : " << ave << endl;
    // cout << "size : " << rect_vector.size() << endl;
    /*double tmp_stddev = 0;
    for(int obj_num = 0;obj_num < contour_num_vector.size();obj_num++)
    {
        tmp_stddev += pow(contour_num_vector[obj_num] - ave,2);
    }
    double stddev = sqrt(tmp_stddev/(contour_num_vector.size()));
    double normal = stddev + ave;
    //if(contour_vector.size() > 4) return false;*/
    std::vector<Rect> dst_vector_rect;
    for (int obj_num = 0; obj_num < contour_num_vector.size(); obj_num++) {
        if (contour_num_vector[obj_num] <= 1.0 * ave) {
            // cout << "obj_num : " << obj_num << "rect_vector.size()" << rect_vector.size() << endl;
            dst_vector_rect.push_back(rect_vector[obj_num]);
        }
    }
    //cout << "ave:" << ave << " stddev: "<< stddev << " normal :" << normal << endl;
    return dst_vector_rect;
}


// 判断矩形是否符合要求
bool judge_rect_satisfy(const Rect &Roi) {
    // cout << "rows : " << Roi.height << " cols : " << Roi.width << endl;
    // cout << "percent : " << (double) Roi.height / Roi.width << endl;
    // cout << "area: " << Roi.height * Roi.width << endl;
    //waitKey();
    if (static_cast<double>(Roi.height) / Roi.width > 0.9)
        return false;
    if (static_cast<double>(Roi.height) / Roi.width < 0.25)
        return false;
    return true;
}


// 绘制满足条件的矩形(似乎只传出了src_image)
Mat rect_the_pic(const Mat &gray_image, const Mat &src_image, std::vector<Rect> &rect_vector) {
    Mat dst_image;
    std::vector<Rect> rect_vector_tmp;
    src_image.copyTo(dst_image);
    rect_vector = judge_contour(gray_image, rect_vector);
    for (auto & obj_num : rect_vector) {
        // 绘图
        if constexpr (DEBUG_MOD) Mat debug_pic = src_image(obj_num);
        //display("debug", debug_pic);
        if (!judge_rect_satisfy(obj_num)) continue;
        rect_vector_tmp.push_back(obj_num);
        //rectangle(dst_image, rect_vector[obj_num], Scalar(255, 255, 0));
    }
    // rect_vector似乎无用
    rect_vector = rect_vector_tmp;
    return dst_image;
}


// 调整一组矩形的位置
void move_Point(const Rect normal_rect, std::vector<Rect> &point_rect) {
    for (auto & obj_num : point_rect) {
        obj_num = Rect(obj_num.x + normal_rect.x, obj_num.y + normal_rect.y, obj_num.width, obj_num.height);
    }
}

// 现在似乎还没有用到
// Point select_the_rect(Mat &src_image, std::vector<Rect> &vector_rect) {
//     double min_distance;
//     static Rect prev_rect;
//     //cout << "debug_size:" << vector_rect.size() << endl;
//     static Point prev_point;
//     register double max_area = 0;
//     register int tmp_index = 0;
//     register Point tmp_point = ERROR_POINT;
//     vector_rect = get_the_distance_vector(vector_rect);
//     vector<Rect> proper_rect;
//     register double height_width_proportion;
//     if (vector_rect.size() == 0) return tmp_point;
//     //vector_rect = sort_area(vector_rect);
//     max_area = vector_rect[0].area();
//     for (int obj_num = 0; obj_num < vector_rect.size(); obj_num++) {
//         height_width_proportion = (double) vector_rect[obj_num].height / vector_rect[obj_num].width;
//         if (height_width_proportion > 0.41 && height_width_proportion < 0.47) {
//             proper_rect.push_back(vector_rect[obj_num]);
//         }
//     }
//     /*if(proper_rect.size() != 0)
//     {
//         for(int obj_num = 0;obj_num < proper_rect.size();obj_num ++)
//         {
//             //cout << "blueblue" <<endl;
//             if(max_area > proper_rect[obj_num].area())
//             {
//                 max_area = proper_rect[obj_num].area();
//                 tmp_point = Point(proper_rect[obj_num].x + proper_rect[obj_num].width/2.0,proper_rect[obj_num].y + proper_rect[obj_num].height/2.0);
//                 tmp_index = obj_num;
//             }
//         }
//         prev_point = tmp_point;
//         first_select_flags = true;
//         rectangle(src_image,proper_rect[tmp_index],Scalar(255,0,255),2,8);
//         return tmp_point;
//     }*/
//     if (!first_select_flags) {
//         for (int obj_num = 0; obj_num < vector_rect.size(); obj_num++) {
//             //cout << "balabala" << endl;
//             if (vector_rect[obj_num].area() < max_area) {
//                 tmp_point = Point(vector_rect[obj_num].x + vector_rect[obj_num].width / 2.0,
//                                   vector_rect[obj_num].y + vector_rect[obj_num].height / 2.0);
//                 max_area = vector_rect[obj_num].area();
//                 tmp_index = obj_num;
//             }
//         }
//         first_select_flags = true;
//         prev_point = tmp_point;
//         prev_rect = vector_rect[tmp_index];
//         rectangle(src_image, vector_rect[tmp_index], Scalar(0, 0, 255), 2, 8);
//         distance_single = CalculateZ(0, vector_rect[tmp_index].height, 60, focus_global);
//         return tmp_point;
//     }
//     vector<double> distance;
//     for (register int obj_num = 0; obj_num < vector_rect.size(); obj_num++) {
//         distance.push_back(sqrt(pow(320 - vector_rect[obj_num].x, 2) + pow(240 - vector_rect[obj_num].y, 2) +
//                                 0.3 * pow(vector_rect[obj_num].x - prev_point.x, 2) +
//                                 0.3 * pow(vector_rect[obj_num].y - prev_point.y, 2) +
//                                 0.2 * pow(prev_rect.width - vector_rect[obj_num].width, 2) +
//                                 0.2 * pow(prev_rect.height - vector_rect[obj_num].height, 2)));
//     }
//     if (distance.size() < 5) {
//         //vector<Rect> sort_rect = bubble_sort(distance,vector_rect);
//         register double min_area = vector_rect[0].area();
//         //register double min_distance = distance[0];
//         register int min_index = 0;
//
//         for (register int obj_num = 0; obj_num < vector_rect.size(); obj_num++) {
//
//             if (min_area > vector_rect[obj_num].area()) {
//                 min_area = vector_rect[obj_num].area();
//                 min_index = obj_num;
//             }
//             /*if(min_distance > distance[obj_num])
//             {
//                 min_distance = distance[obj_num];
//                 min_index = obj_num;
//             }*/
//         }
//         min_distance = sqrt(pow(prev_point.x - vector_rect[min_index].x - vector_rect[min_index].width / 2.0, 2) +
//                             pow(prev_point.y - vector_rect[min_index].y - vector_rect[min_index].height / 2.0, 2));
//         rectangle(src_image, vector_rect[min_index], Scalar(0, 255, 0), 3.86);
//         circle(src_image, prev_point, 2, Scalar(0, 255, 255), 3.6);
//         circle(src_image, Point(vector_rect[min_index].x + vector_rect[min_index].width / 2.0,
//                                 vector_rect[min_index].y + vector_rect[min_index].height / 2.0), 2, Scalar(0, 255, 0),
//                2, 8);
//         cout << "distance : "
//              << sqrt(pow(prev_point.x - vector_rect[min_index].x - vector_rect[min_index].width / 2.0, 2) +
//                      pow(prev_point.y - vector_rect[min_index].y - vector_rect[min_index].height / 2.0, 2)) << endl;
//         //display("debug" , src_image);
//         //waitKey();
//         /*	if(min_distance > 150)
//             {
//                 distance_times ++;
//                 if(distance_times> 20)
//                 {
//                     distance_times = 0;
//                     prev_point = Point(vector_rect[min_index].x +vector_rect[min_index].width / 2.0,vector_rect[min_index].y + vector_rect[min_index].height/2.0);
//                     prev_rect = vector_rect[min_index];
//                 }
//                 distance_single = CalculateZ(0,prev_rect.height,60,focus_global);
//                 return prev_point;
//             }*/
//         distance_times = 0;
//         //	cout << "debug" << endl;
//         //double min_distance =  sqrt(pow(prev_point.x - vector_rect[min_index].x - vector_rect[min_index].width / 2.0,2) + pow(prev_point.y - vector_rect[min_index].y - vector_rect[min_index].height/2.0,2));
//         prev_point = Point(vector_rect[min_index].x + vector_rect[min_index].width /
//                                                       2.0 /*+ (vector_rect[min_index].x + vector_rect[min_index].width / 2.0 - prev_point.x)*/,
//                            vector_rect[min_index].y + vector_rect[min_index].height /
//                                                       2.0 /*+ (vector_rect[min_index].y + vector_rect[min_index].height / 2.0 - prev_point.y)*/);
//         //prev_point = Point(sort_rect[0].x + sort_rect[0].width / 2.0,sort_rect[0].y + sort_rect[0].height / 2.0);
//         distance_single = CalculateZ(0, vector_rect[min_index].height, 60, focus_global);
//         return prev_point;
//     }
//     register int min_index;
//     register double min_area;
//     vector<Rect> sort_rect = bubble_sort(distance, vector_rect);
//     min_area = sort_rect[0].area();
//     double max_area_tmp = get_max_area(sort_rect);
//     //cout << "max_area:" << max_area_tmp << endl;
//     for (register int obj_num = 0; obj_num < 2; obj_num++) {
//         if (min_area > sort_rect[obj_num].area()) {
//
//             /*if(sort_rect[obj_num].area() < max_area_tmp/1.5)
//                 continue;*/
//             min_area = sort_rect[obj_num].area();
//             //cout << "min_area : "<< min_area << " obj_area:" << sort_rect[obj_num].area() << endl;
//             min_index = obj_num;
//         }
//     }
//     min_distance = sqrt(pow(prev_point.x - sort_rect[min_index].x - sort_rect[min_index].width / 2.0, 2) +
//                         pow(prev_point.y - sort_rect[min_index].y - sort_rect[min_index].height / 2.0, 2));
//     rectangle(src_image, sort_rect[min_index], Scalar(255, 255, 0), 2, 8);
//     circle(src_image, prev_point, 2, Scalar(0, 255, 255), 2, 8);
//     circle(src_image, Point(sort_rect[min_index].x + sort_rect[min_index].width / 2.0,
//                             sort_rect[min_index].y + sort_rect[min_index].height / 2.0), 2, Scalar(0, 255, 0), 2, 8);
//     //cout << "distance : "<< sqrt(pow(prev_point.x - sort_rect[min_index].x - sort_rect[min_index].width / 2.0,2) + pow(prev_point.y - sort_rect[min_index].y - sort_rect[min_index].height/2.0,2)) <<endl;
//     //display("debug" , src_image);
//     //waitKey();
//     if (min_distance > 150) {
//         distance_times++;
//         if (distance_times > 20) {
//             distance_times = 0;
//             prev_rect = sort_rect[min_index];
//             prev_point = Point(sort_rect[min_index].x + sort_rect[min_index].width / 2.0,
//                                sort_rect[min_index].y + sort_rect[min_index].height / 2.0);
//         }
//         distance_single = CalculateZ(0, prev_rect.height, 60, focus_global);
//         return prev_point;
//     }
//     distance_times = 0;
//     prev_point = Point(sort_rect[min_index].x + sort_rect[min_index].width /
//                                                 2.0  /*+ (sort_rect[min_index].x + sort_rect[min_index].width / 2.0 - prev_point.x)*/,
//                        sort_rect[min_index].y + sort_rect[min_index].height /
//                                                 2.0 /*+ (sort_rect[min_index].y + sort_rect[min_index].height / 2.0 - prev_point.y)*/);
//     prev_rect = sort_rect[min_index];
//     distance_single = CalculateZ(0, sort_rect[min_index].height, 60, focus_global);
//     //cout << "area: " << min_area << endl;
//
//     //return prev_point
//     //waitKey();
//
//
//     return prev_point;
//     //return prev_point;
// }

double im_real_weights = 0;
int real_distance_height = 60; // 不确定参数含义，可能可以优化为常量
// 计算并返回物体在三维空间中的Z轴距离。
// 根据两个数字坐标、实际距离和焦距来计算物体在三维空间中的Z轴距离。
double CalculateZ(const float Digital_First_X, const float Digital_Second_X, const float Real_Distance, const double focus) {
    const float Image_W = Digital_Second_X - Digital_First_X;
    im_real_weights = static_cast<double>(real_distance_height) / Image_W;
    const double Disitance = focus * Real_Distance / Image_W;
    return Disitance;
}


// 冒泡排序
std::vector<Rect> bubble_sort(std::vector<double> distance, std::vector<Rect> distance_Rect) {
    Rect tmp_Rect;
    int tmp_index;
    if (distance.empty())
        return distance_Rect;
    for (int obj_num = 0; obj_num < distance.size(); obj_num++) {
        for (int index_num = 0; index_num < distance.size() - 1 - obj_num; index_num++) {
            if (distance[index_num + 1] < distance[index_num]) {
                tmp_index = distance[index_num + 1];
                distance[index_num + 1] = distance[index_num];
                distance[index_num] = tmp_index;
                tmp_Rect = distance_Rect[index_num + 1];
                distance_Rect[index_num + 1] = distance_Rect[index_num];
                distance_Rect[index_num] = tmp_Rect;
            }
        }
    }
    return distance_Rect;
}


#define ERROR_POINT Point(-1,-1)
auto focus_global = 7.0;
static bool first_select_flags = true;
static int distance_times = 0;
// 初始化无目标计数器
static int no_target = 0;
static Rect prev_rect; //存储上一次选择的矩形和中心点
static auto prev_point = ERROR_POINT; //存储上一次选择的矩形和中心点

//传入：原始图像，装甲板的矩形容器，返回区域，返回矩形
//返回：装甲板的中心点
//在一组矩形中选择一个矩形，并返回该矩形的中心点
Point select_the_rect(Mat &src_image, std::vector<Rect> &vector_rect, int &return_area, Rect &return_rect) {
    double min_distance;
    double max_area = 0;
    auto tmp_point = ERROR_POINT;
    //vector_rect = get_the_distance_vector(vector_rect);
    std::vector<Rect> proper_rect;
    //cout << "prev_rect  -> width :" << prev_rect.width << "height : "<< prev_rect.height << endl;

    // vector_rect为空，并且这不是第一次运行这个函数
    if (vector_rect.empty() && !first_select_flags)
    {
        target_flags = false;
        no_target++;
        if (no_target <= 1) // 返回上一次选择的矩形的面积、中心点和矩形
            {
            return_area = prev_rect.area();
            // 修改全局变量distance_single
            distance_single = CalculateZ(0, static_cast<float>(prev_rect.height), 60, focus_global);
            return_rect = prev_rect;
            return prev_point;
        }
        return ERROR_POINT; // 返回临时点
    }

    // vector_rect为空，并且这是第一次运行这个函数
    if (vector_rect.empty() && first_select_flags)
    {
        target_flags = false;
        return_area = 0;
        return ERROR_POINT; // 默认返回点
    }

    // vector_rect必定不为空
    // 遍历vector_rect中的每一个矩形
    target_flags = true;
    no_target = 0;
    max_area = vector_rect[0].area();
    for (auto & obj_num : vector_rect) {
        const double height_width_proportion = static_cast<double>(obj_num.height) / obj_num.width;
        if (height_width_proportion > 0.41 && height_width_proportion < 0.47) {
            // 将符合条件的矩形放入proper_rect中
            proper_rect.push_back(obj_num);
        }
    }

    // 第一次运行
    if (first_select_flags) {
        // 排序
        int tmp_index = 0;
        for (int obj_num = 0; obj_num < vector_rect.size(); obj_num++) {
            if (vector_rect[obj_num].area() < max_area) {
                tmp_point = Point(vector_rect[obj_num].x + vector_rect[obj_num].width / 2.0,
                                  vector_rect[obj_num].y + vector_rect[obj_num].height / 2.0);
                max_area = vector_rect[obj_num].area();
                tmp_index = obj_num;
            }
        }
        // 记录为上一次返回
        first_select_flags = false;
        prev_point = tmp_point;
        prev_rect = vector_rect[tmp_index];
        if constexpr (DEBUG_MOD) rectangle(src_image, vector_rect[tmp_index], Scalar(0, 0, 255), 2, 8);
        distance_single = CalculateZ(0, vector_rect[tmp_index].height, 60, focus_global);
        return_area = prev_rect.area();
        return_rect = prev_rect;
        return tmp_point;
    }

    // 不是第一次运行
    std::vector<double> distance; //偏差
    // 加权计算偏差
    for (const auto & obj_num : vector_rect) {
        const double Roi_width_bytes_height = static_cast<double>(obj_num.width) / obj_num.height - 0.42;
        distance.push_back(sqrt(1 * pow(Roi_width_bytes_height, 2) + 0.3 * pow(320 - obj_num.x, 2) +
                                0.3 * pow(240 - obj_num.y, 2) +
                                0.3 * pow(obj_num.x - prev_point.x, 2) +
                                0.3 * pow(obj_num.y - prev_point.y, 2) +
                                0.8 * pow(prev_rect.width - obj_num.width, 2) +
                                0.8 * pow(prev_rect.height - obj_num.height, 2)));
    }
    // 如果vector_rect中只有一个矩形
    if (distance.size() < 2) {
        double min_area = vector_rect[0].area();
        int min_index = 0;
        for (int obj_num = 0; obj_num < vector_rect.size(); obj_num++) {
            if (min_area > vector_rect[obj_num].area()) {
                min_area = vector_rect[obj_num].area();
                min_index = obj_num;
            }
        }
        min_distance = sqrt(pow(prev_point.x - vector_rect[min_index].x - vector_rect[min_index].width / 2.0, 2) +
                            pow(prev_point.y - vector_rect[min_index].y - vector_rect[min_index].height / 2.0, 2));
        prev_rect = vector_rect[min_index];

        // 绘图
        if constexpr (DEBUG_MOD) {
            rectangle(src_image, vector_rect[min_index], Scalar(0, 255, 0), 3.86);
            circle(src_image, prev_point, 2, Scalar(0, 255, 255), 3.6);
            circle(src_image, Point(vector_rect[min_index].x + vector_rect[min_index].width / 2.0,
                                    vector_rect[min_index].y + vector_rect[min_index].height / 2.0),
                                    2, Scalar(0, 255, 0), 2, 8);
        }
        // 计算点并返回
        distance_times = 0;
        //double min_distance =  sqrt(pow(prev_point.x - vector_rect[min_index].x - vector_rect[min_index].width / 2.0,2) + pow(prev_point.y - vector_rect[min_index].y - vector_rect[min_index].height/2.0,2));
        prev_point = Point(vector_rect[min_index].x + vector_rect[min_index].width /
                                                      2.0 /*+ (vector_rect[min_index].x + vector_rect[min_index].width / 2.0 - prev_point.x)*/,
                           vector_rect[min_index].y + vector_rect[min_index].height /
                                                      2.0 /*+ (vector_rect[min_index].y + vector_rect[min_index].height / 2.0 - prev_point.y)*/);
        //prev_point = Point(sort_rect[0].x + sort_rect[0].width / 2.0,sort_rect[0].y + sort_rect[0].height / 2.0);
        distance_single = CalculateZ(0, vector_rect[min_index].height, 60, focus_global);
        return_area = prev_rect.area();
        return_rect = prev_rect;
        return prev_point;
    }

    // 有多个偏差值
    int min_index; // 最小偏差的索引
    double min_area; // 最小面积
    std::vector<Rect> sort_rect = bubble_sort(distance, vector_rect);
    min_area = sort_rect[0].area();
    // double max_area_tmp = get_max_area(sort_rect);//Never Used
    //cout << "max_area:" << max_area_tmp << endl;
    // 只比较前两项
    for (int obj_num = 0; obj_num < 2; obj_num++) {
        if (min_area > sort_rect[obj_num].area()) {
            min_area = sort_rect[obj_num].area();
            min_index = obj_num;
        }
    }
    min_distance = sqrt(pow(prev_point.x - sort_rect[min_index].x - sort_rect[min_index].width / 2.0, 2) +
                        pow(prev_point.y - sort_rect[min_index].y - sort_rect[min_index].height / 2.0, 2));

    // 绘图
    if constexpr (DEBUG_MOD) {
        rectangle(src_image, sort_rect[min_index], Scalar(255, 255, 0), 2, 8);
        circle(src_image, prev_point, 2, Scalar(0, 255, 255), 2, 8);
        circle(src_image, Point(sort_rect[min_index].x + sort_rect[min_index].width / 2.0,
                                sort_rect[min_index].y + sort_rect[min_index].height / 2.0), 2, Scalar(0, 255, 0), 2, 8);
    }

    // 如果最小距离大于300
    if (min_distance > 300) {
        distance_times++;
        if (distance_times > 5) {
            distance_times = 0;
            prev_rect = sort_rect[min_index];
            prev_point = Point(sort_rect[min_index].x + sort_rect[min_index].width / 2.0,
                               sort_rect[min_index].y + sort_rect[min_index].height / 2.0);
            prev_rect = sort_rect[min_index];
        }
        distance_single = CalculateZ(0, prev_rect.height, 60, focus_global);
        //	distance_single = CalculateZ(prev_rect);
        return_area = prev_rect.area();
        return_rect = prev_rect;
        return prev_point;
    }

    // 重置计数器
    distance_times = 0;
    prev_point = Point(sort_rect[min_index].x + sort_rect[min_index].width /
                                                2.0  /*+ (sort_rect[min_index].x + sort_rect[min_index].width / 2.0 - prev_point.x)*/,
                       sort_rect[min_index].y + sort_rect[min_index].height /
                                                2.0 /*+ (sort_rect[min_index].y + sort_rect[min_index].height / 2.0 - prev_point.y)*/);
    prev_rect = sort_rect[min_index];
    distance_single = CalculateZ(0, sort_rect[min_index].height, 60, focus_global);
    return_area = prev_rect.area();
    return_rect = prev_rect;
    return prev_point;
}


// 识别装甲板
void discermArmour() {
    // 定义clock_t变量
    const clock_t start = clock();//开始时间
    // 从队列中取出帧，如果没有帧就直接返回
    g_mutex.lock();
    if (frames.empty()) {
        g_mutex.unlock();
        return;
    }
    Mat frame = frames.front();
    frames.pop();
    g_mutex.unlock();
    // showFrame(frame);
    // 识别算法部分开始
    std::vector<Point> test;
    // 获取ROI区域
    Rect Roi_rect;
    Roi_rect = get_roi_rect(!target_flags);
    // 切割图像
    frame = frame(Roi_rect);
    // 预处理
    Mat gray_image = convertBGR2HSV2Gray(frame);
    gray_image = dilate_pic(gray_image, 12);

    // 轮廓处理
    std::vector<std::vector<Point>> vector_Point;
    get_rect_pic_contour(gray_image, get_point_contours(gray_image), vector_Point);
    //vector_Point = judge_the_min_distance(gray_image,vector_Point);
    //cout << "vector_point size:" << vector_Point.size() << endl;

    // 获取并判断可能的灯条旋转角度
    std::vector<double> angle_vector = get_rotate_angle(frame, vector_Point);
    std::vector<Scalar> place_info_vector = judge_the_theata(cal_distance_theata(angle_vector), 3);

    // 尝试获取装甲板
    std::vector<Rect> Rect_vector;
    set_the_rect(gray_image, place_info_vector, vector_Point, Rect_vector, frame.rows, frame.cols);

    // 调试模式下绘制装甲板矩形
    if constexpr (DEBUG_MOD) {
        Mat result_pic = rect_the_pic(gray_image, frame, Rect_vector);
    }

    move_Point(Roi_rect, Rect_vector);
    Rect select_rect;
    int armour_area;
    //	Point send_point = select_the_rect(result_pic,Rect_vector,armour_area);
    // 将要发送的点
    Point send_point = select_the_rect(frame, Rect_vector, armour_area, select_rect);
    // 在tui上显示点
    setDiscernDirection(send_point.x,send_point.y);

    // 绘图
    if constexpr (DEBUG_MOD){
        rectangle(frame,
                  Rect(select_rect.x - Roi_rect.x, select_rect.y - Roi_rect.y, select_rect.width, select_rect.height),
                  Scalar(0, 255, 255), 2, 8);
        circle(frame, send_point, 2, Scalar(255, 0, 255), 2, 8);
    }
    if constexpr (DEBUG_MOD) {
        imshow("DEBUG",frame);
        waitKey(1);
    }

    // TODO:卡尔曼
    // 发送数据
    if (send_point == ERROR_POINT){
        setCmdStatus(NOT_FIND);
    } else {
        setCmdStatus(FIND);
        setSerialData(send_point.x, send_point.y);
    }

    const clock_t end = clock();//结束时间
    setTimeCost(static_cast<double>(end - start)/CLOCKS_PER_SEC);

}

// 识别程序入口（多线程）
void Discern_run() {
    testCamera();
    Camera_Init();
    Camera_Open();
    std::vector<std::thread> threads;
    for (int i = 0; i < camera_thread_number; i++) {
        threads.emplace_back([&]{
            while (true) {
                readFrame(cap);
            }
        });
    }
    // TODO：卡尔曼滤波
    // TODO：卡尔曼传入初值
    for (int i = 0; i < discern_thread_number; i++) {
        threads.emplace_back([&]{
            while (true) {
                discermArmour();
            }
        });
    }
    for(auto& thread : threads) {
        thread.join();
    }
    // while (true) {
    //     //定义clock_t变量
    //     const clock_t start = clock();//开始时间
    //     cv::Mat frame = readFrame(cap);
    //     cv::imshow("taet", frame);
    //     cv::waitKey(1);
    //     const clock_t end = clock();//结束时间
    //     setTimeCost(static_cast<double>(end - start)/CLOCKS_PER_SEC);
    //     // std::cout << "Discern_run time: " << (double)(end-start)/CLOCKS_PER_SEC << std::endl;
    // }
}
