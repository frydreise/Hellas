// (C) 2015 Tim Gurto

#ifndef UTIL_H
#define UTIL_H

#include "Rect.h"

#include <cstdlib>
#include <sstream>

struct Point;

const double SQRT_2 = 1.4142135623731;

bool isDebug();

inline int toInt(double d){
    if (d > 0)
        return static_cast<int>(d + .5);
    else
        return static_cast<int>(d - .5);
}

inline double randDouble(){ return static_cast<double>(rand()) / RAND_MAX; }

double distance(const Point &a, const Point &b);
double distance(const Point &p, const Point &a, const Point &b); // point P to line AB

// Return the point between a and b which is dist from point a,
// or return b if dist exceeds the distance between a and b.
Point interpolate(const Point &a, const Point &b, double dist);

inline int str2int(const std::string str) {
    std::istringstream iss(str);
    int i;
    iss >> i;
    return i;
}

template<typename T1>
std::string makeArgs(T1 val1){
    std::ostringstream oss;
    oss << val1;
    return oss.str();
}

template<typename T1, typename T2>
std::string makeArgs(T1 val1, T2 val2){
    std::ostringstream oss;
    oss << val1 << ',' << val2;
    return oss.str();
}

template<typename T1, typename T2, typename T3>
std::string makeArgs(T1 val1, T2 val2, T3 val3){
    std::ostringstream oss;
    oss << val1 << ',' << val2 << ',' << val3;
    return oss.str();
}

template<typename T1, typename T2, typename T3, typename T4>
std::string makeArgs(T1 val1, T2 val2, T3 val3, T4 val4){
    std::ostringstream oss;
    oss << val1 << ',' << val2 << ',' << val3 << ',' << val4;
    return oss.str();
}

bool collision(const Point &point, const Rect &rect);

#undef min
#undef max
template<typename T>
T min(const T &lhs, const T &rhs) {
    return lhs < rhs ? lhs : rhs;
}

template<typename T>
T max(const T &lhs, const T &rhs) {
    return rhs < lhs ? lhs : rhs;
}

#endif
