#ifndef SIMPLIFIER_H
#define SIMPLIFIER_H

#include "polygon.h"

struct CollapseCandidate {
    int ring_id = -1;
    int start_index = -1;
    int b_index = -1;
    int c_index = -1;
    Point replacement;
    double cost = 0.0;
    bool valid = false;
};

struct SimplificationResult {
    Polygon polygon;
    double total_areal_displacement = 0.0;
};

SimplificationResult simplify_polygon(const Polygon& polygon, int target_vertices);

#endif