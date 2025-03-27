#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>

using namespace cv;
using namespace std;

// Function to compute ZNCC between two windows
float CalcZNCC(const Mat& left_window, const Mat& right_window) {
    int num_pixels = left_window.rows * left_window.cols;
    float sum_L = 0.0f, sum_R = 0.0f;
    float sum_sq_L = 0.0f, sum_sq_R = 0.0f;
    float sum_LR = 0.0f;

    for (int i = 0; i < left_window.rows; ++i) {
        for (int j = 0; j < left_window.cols; ++j) {
            float l = left_window.at<uchar>(i, j);
            float r = right_window.at<uchar>(i, j);
            sum_L += l;
            sum_R += r;
            sum_sq_L += l * l;
            sum_sq_R += r * r;
            sum_LR += l * r;
        }
    }

    float mu_L = sum_L / num_pixels;
    float mu_R = sum_R / num_pixels;
    float sigma_L = sqrt(sum_sq_L / num_pixels - mu_L * mu_L);
    float sigma_R = sqrt(sum_sq_R / num_pixels - mu_R * mu_R);

    if (sigma_L < 1e-6 || sigma_R < 1e-6)
        return 0.0f;

    float covariance = (sum_LR / num_pixels) - mu_L * mu_R;
    return covariance / (sigma_L * sigma_R);
}

// Compute disparity map from left to right
Mat computeDisparity(const Mat& left, const Mat& right, int window_size, int max_disp) {
    int wh = window_size / 2;
    Mat disparity(left.size(), CV_32F, Scalar(0));

    for (int y = wh; y < left.rows - wh; ++y) {
        for (int x = wh; x < left.cols - wh; ++x) {
            float max_zncc = -1.0f;
            int best_d = 0;
            Mat left_window = left(Rect(x - wh, y - wh, window_size, window_size));

            for (int d = 0; d <= max_disp; ++d) {
                int right_x = x - d;
                if (right_x < wh || right_x >= right.cols - wh)
                    continue;

                Mat right_window = right(Rect(right_x - wh, y - wh, window_size, window_size));
                float zncc = CalcZNCC(left_window, right_window);

                if (zncc > max_zncc) {
                    max_zncc = zncc;
                    best_d = d;
                }
            }
            disparity.at<float>(y, x) = best_d;
        }
    }
    return disparity;
}

// Compute right disparity map for cross-check
Mat computeRightDisparity(const Mat& right, const Mat& left, int window_size, int max_disp) {
    int wh = window_size / 2;
    Mat disparity(right.size(), CV_32F, Scalar(0));

    for (int y = wh; y < right.rows - wh; ++y) {
        for (int x = wh; x < right.cols - wh; ++x) {
            float max_zncc = -1.0f;
            int best_d = 0;
            Mat right_window = right(Rect(x - wh, y - wh, window_size, window_size));

            for (int d = 0; d <= max_disp; ++d) {
                int left_x = x + d;
                if (left_x >= left.cols - wh || left_x < wh)
                    continue;

                Mat left_window = left(Rect(left_x - wh, y - wh, window_size, window_size));
                float zncc = CalcZNCC(right_window, left_window);

                if (zncc > max_zncc) {
                    max_zncc = zncc;
                    best_d = d;
                }
            }
            disparity.at<float>(y, x) = best_d;
        }
    }
    return disparity;
}

// Cross-check to invalidate inconsistent disparities
Mat crossCheck(const Mat& D1, const Mat& D2, int max_disp) {
    Mat mask(D1.size(), CV_8U, Scalar(0));

    for (int y = 0; y < D1.rows; ++y) {
        for (int x = 0; x < D1.cols; ++x) {
            float d = D1.at<float>(y, x);
            if (d == 0) {
                mask.at<uchar>(y, x) = 1;
                continue;
            }

            int x_right = x - static_cast<int>(d);
            if (x_right < 0 || x_right >= D2.cols) {
                mask.at<uchar>(y, x) = 1;
                continue;
            }

            float d2 = D2.at<float>(y, x_right);
            if (abs(d - d2) > 1.0f)
                mask.at<uchar>(y, x) = 1;
        }
    }
    return mask;
}

// Fill occlusions using nearest valid disparity
Mat occlusionFill(const Mat& disparity, const Mat& mask) {
    Mat filled = disparity.clone();

    for (int y = 0; y < filled.rows; ++y) {
        for (int x = 0; x < filled.cols; ++x) {
            if (mask.at<uchar>(y, x) == 1) {
                int left = x - 1;
                while (left >= 0 && mask.at<uchar>(y, left) == 1)
                    --left;

                if (left >= 0) {
                    filled.at<float>(y, x) = filled.at<float>(y, left);
                } else {
                    int right = x + 1;
                    while (right < filled.cols && mask.at<uchar>(y, right) == 1)
                        ++right;

                    filled.at<float>(y, x) = (right < filled.cols) ? filled.at<float>(y, right) : 0;
                }
            }
        }
    }
    return filled;
}

// Normalize disparity values to 0-255
Mat normalizeDisparity(const Mat& disparity, int max_disp) {
    Mat normalized;
    disparity.convertTo(normalized, CV_8U, 255.0 / max_disp);
    return normalized;
}

int main() {
    auto total_start = chrono::steady_clock::now();

    // Read calibration data
    int original_width = 0, original_height = 0, original_ndisp = 0;
    ifstream calib("calib.txt");
    string line;
    while (getline(calib, line)) {
        if (line.find("width=") != string::npos)
            original_width = stoi(line.substr(6));
        else if (line.find("height=") != string::npos)
            original_height = stoi(line.substr(7));
        else if (line.find("ndisp=") != string::npos)
            original_ndisp = stoi(line.substr(6));
    }
    calib.close();

    // Read and preprocess images
    Mat left_color = imread("left.png");
    Mat right_color = imread("right.png");
    if (left_color.empty() || right_color.empty()) {
        cerr << "Error reading images" << endl;
        return -1;
    }

    Mat left_gray, right_gray;
    cvtColor(left_color, left_gray, COLOR_BGR2GRAY);
    cvtColor(right_color, right_gray, COLOR_BGR2GRAY);

    float scale_factor = static_cast<float>(left_gray.cols) / original_width;
    Size new_size(left_gray.cols, left_gray.rows);

    Mat left, right;
    resize(left_gray, left, new_size, 0, 0, INTER_LINEAR);
    resize(right_gray, right, new_size, 0, 0, INTER_LINEAR);

    int MAX_DISP = static_cast<int>(original_ndisp * scale_factor);
    int window_size = 9;

    // Compute disparity maps
    auto start = chrono::steady_clock::now();
    Mat D1 = computeDisparity(left, right, window_size, MAX_DISP);
    Mat D2 = computeRightDisparity(right, left, window_size, MAX_DISP);
    auto end = chrono::steady_clock::now();

    // Post-processing
    Mat mask = crossCheck(D1, D2, MAX_DISP);
    Mat filled = occlusionFill(D1, mask);

    // Normalize and save results
    Mat zncc_norm = normalizeDisparity(D1, MAX_DISP);
    imwrite("zncc.png", zncc_norm);

    Mat cross_check_norm = normalizeDisparity(D1, MAX_DISP);
    cross_check_norm.setTo(0, mask);
    imwrite("crosscheck.png", cross_check_norm);

    Mat filled_norm = normalizeDisparity(filled, MAX_DISP);
    imwrite("depthmap.png", filled_norm);

    auto total_end = chrono::steady_clock::now();
    auto total_time = chrono::duration_cast<chrono::milliseconds>(total_end - total_start);
    auto compute_time = chrono::duration_cast<chrono::milliseconds>(end - start);

    cout << "Disparity computation time: " << compute_time.count() << " ms" << endl;
    cout << "Total execution time: " << total_time.count() << " ms" << endl;

    return 0;
}

