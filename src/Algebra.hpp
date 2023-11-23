#ifndef ALGEBRA_H
#define ALGEBRA_H

#include <vector>
#include <cmath>

struct Point {
    float x, y;
    explicit operator bool() const { return x!=0||y!=0; }
    Point &operator+=(Point &p) { x+=p.x; y+=p.y; return *this; }
    Point &operator-=(Point &p) { x-=p.x; y-=p.y; return *this; }
    Point &operator*=(float k) { x*=k; y*=k; return *this; }
    Point &operator/=(float k) { x/=k; y/=k; return *this; }
};

using Vector = Point;

inline Point operator*(Point p, float f) { return {p.x*f,p.y*f}; }
inline Point operator*(float f, Point p) { return {p.x*f,p.y*f}; }
inline Point operator/(Point p, float f) { return {p.x/f,p.y/f}; }
inline Point operator+(Point p1, Point p2) { return {p1.x+p2.x,p1.y+p2.y}; }
inline Point abs(Point p) { return {std::fabs(p.x),std::fabs(p.y)}; }
inline Vector operator-(Point p1, Point p2) { return {p1.x-p2.x,p1.y-p2.y}; }

inline Point lerp(Point a, Point b, float t) {
    return Point{ t*b.x+(1.f-t)*a.x, t*b.y+(1.f-t)*a.y };
}

inline float redon(float x) { return int(x)+.5f; }
inline Point redon(Point p) { return {redon(p.x),redon(p.y)}; }

#endif
