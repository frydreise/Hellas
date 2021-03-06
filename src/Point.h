// (C) 2015 Tim Gurto

#ifndef POINT_H
#define POINT_H

#include "types.h"
#include "util.h"

// Describes a 2D point
struct Point{
    double x;
    double y;

    Point(double xArg = 0, double yArg = 0);

    Point(const Rect &rect);

    bool operator==(const Point &rhs) const;
    Point &operator+=(const Point &rhs);
    Point &operator-=(const Point &rhs);
    Point operator+(const Point &rhs) const;
    Point operator-(const Point &rhs) const;

private:
    static const double EPSILON;
};

inline Rect operator+(const Rect &lhs, const Point &rhs){
    Rect r = lhs;
    r.x += toInt(rhs.x);
    r.y += toInt(rhs.y);
    return r;
}

inline Rect operator-(const Rect &lhs, const Point &rhs){
    Rect r = lhs;
    r.x -= toInt(rhs.x);
    r.y -= toInt(rhs.y);
    return r;
}

inline Point operator-(const Point &lhs, const Rect &rhs){
    Point p = lhs;
    p.x -= rhs.x;
    p.y -= rhs.y;
    return p;
}

inline Point operator+(const Point &lhs, const Rect &rhs){
    Point p = lhs;
    p.x += rhs.x;
    p.y += rhs.y;
    return p;
}

inline Point operator*(const Point &lhs, double rhs){
    Point p = lhs;
    p.x *= rhs;
    p.y *= rhs;
    return p;
}

#endif
