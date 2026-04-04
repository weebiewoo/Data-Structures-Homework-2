#ifndef SIMPLIFIER_H
#define SIMPLIFIER_H

#include "polygon.h"

struct CollapseCandidate {
    int ring_id = -1;        // Which ring the collapse belongs to
    int start_index = -1;    // Index of vertex 'a' (start of the 4-vertex sequence)
    int b_index = -1;        // Index of vertex 'b' (first vertex to collapse)
    int c_index = -1;        // Index of vertex 'c' (second vertex to collapse)
    Point replacement;       // The new point 'e' that replaces b and c
    double cost = 0.0;       // Areal displacement cost (lower is better)
    bool valid = false;      // Whether this candidate is valid
};

struct SimplificationResult {
    Polygon polygon;
    double total_areal_displacement = 0.0;
};

SimplificationResult simplify_polygon_APSC(const Polygon &polygon, int target_vertices);
SimplificationResult simplify_polygon(const Polygon& polygon, int target_vertices);
SimplificationResult simplify_polygon_hybrid(const Polygon &polygon, int target_vertices);

#endif