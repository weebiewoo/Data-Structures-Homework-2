#include "geometry.h"

#include <algorithm>
#include <cmath>

Point operator+(const Point& a, const Point& b) {
    return {a.x + b.x, a.y + b.y};
}

Point operator-(const Point& a, const Point& b) {
    return {a.x - b.x, a.y - b.y};
}

Point operator*(double s, const Point& p) {
    return {s * p.x, s * p.y};
}

bool nearly_equal(double a, double b, double eps) {
    return std::abs(a - b) <= eps;
}

bool same_point(const Point& a, const Point& b, double eps) {
    return nearly_equal(a.x, b.x, eps) && nearly_equal(a.y, b.y, eps);
}

double dot(const Point& a, const Point& b) {
    return a.x * b.x + a.y * b.y;
}

double cross(const Point& a, const Point& b) {
    return a.x * b.y - a.y * b.x;
}

double triangle_area2(const Point& a, const Point& b, const Point& c) {
    return cross(b - a, c - a);
}

double distance2(const Point& a, const Point& b) {
    const double dx = a.x - b.x;
    const double dy = a.y - b.y;
    return dx * dx + dy * dy;
}

int orientation(const Point& a, const Point& b, const Point& c, double eps) {
    const double val = triangle_area2(a, b, c);
    if (std::abs(val) <= eps) {
        return 0;
    }
    return (val > 0.0) ? 1 : -1;
}

bool on_segment(const Point& a, const Point& b, const Point& p, double eps) {
    if (orientation(a, b, p, eps) != 0) {
        return false;
    }

    return std::min(a.x, b.x) - eps <= p.x && p.x <= std::max(a.x, b.x) + eps &&
           std::min(a.y, b.y) - eps <= p.y && p.y <= std::max(a.y, b.y) + eps;
}

bool segments_intersect_or_touch(const Point& a, const Point& b,
                                 const Point& c, const Point& d,
                                 double eps) {
    const int o1 = orientation(a, b, c, eps);
    const int o2 = orientation(a, b, d, eps);
    const int o3 = orientation(c, d, a, eps);
    const int o4 = orientation(c, d, b, eps);

    if (o1 != o2 && o3 != o4) {
        return true;
    }

    if (o1 == 0 && on_segment(a, b, c, eps)) return true;
    if (o2 == 0 && on_segment(a, b, d, eps)) return true;
    if (o3 == 0 && on_segment(c, d, a, eps)) return true;
    if (o4 == 0 && on_segment(c, d, b, eps)) return true;

    return false;
}

bool segments_properly_intersect(const Point& a, const Point& b,
                                 const Point& c, const Point& d,
                                 double eps) {
    if (!segments_intersect_or_touch(a, b, c, d, eps)) {
        return false;
    }

    if (same_point(a, c, eps) || same_point(a, d, eps) ||
        same_point(b, c, eps) || same_point(b, d, eps)) {
        return false;
    }

    if (on_segment(a, b, c, eps) || on_segment(a, b, d, eps) ||
        on_segment(c, d, a, eps) || on_segment(c, d, b, eps)) {
        return true;
    }

    return true;
}