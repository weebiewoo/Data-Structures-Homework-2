#include "simplifier.h"

#include <stdexcept>
#include <iostream>

#include "geometry.h"
#include "polygon.h"
// Add to simplifier.cpp

namespace
{
    constexpr double EPS = 1e-9;
    constexpr double AREA_EPS = 1e-8;

    // Area-preserving point selection (your existing function)
    Point choose_area_preserving_point(const Point &a, const Point &b,
                                       const Point &c, const Point &d)
    {
        const double EPS = 1e-9;

        // Line through a and d (the base edge)
        //double line_ad_a = d.y - a.y;                            // Coefficient for x
        //double line_ad_b = a.x - d.x;                            // Coefficient for y
        //double line_ad_c = -(line_ad_a * a.x + line_ad_b * a.y); // Constant term

        // The area-preserving line equation (ensures equal area distribution)
        // Derived from condition that area(abe) + area(cde) = area(abcd)/2
        double area_line_a = d.y - a.y;
        double area_line_b = a.x - d.x;
        double area_line_c = -b.y * a.x + (a.y - c.y) * b.x + (b.y - d.y) * c.x + c.y * d.x;

        // Helper function to find intersection of two lines
        auto line_intersection = [](double a1, double b1, double c1,
                                    double a2, double b2, double c2) -> Point
        {
            double det = a1 * b2 - a2 * b1;
            if (std::abs(det) < 1e-9)
                return {0, 0};
            return {(b1 * c2 - b2 * c1) / det, (c1 * a2 - c2 * a1) / det};
        };

        // Find intersection with line a-b
        double ab_a = b.y - a.y, ab_b = a.x - b.x, ab_c = -(ab_a * a.x + ab_b * a.y);
        Point e_ab = line_intersection(area_line_a, area_line_b, area_line_c, ab_a, ab_b, ab_c);
        bool has_ab = std::abs(area_line_a * ab_b - ab_a * area_line_b) > EPS;

        // Find intersection with line c-d
        double cd_a = d.y - c.y, cd_b = c.x - d.x, cd_c = -(cd_a * c.x + cd_b * c.y);
        Point e_cd = line_intersection(area_line_a, area_line_b, area_line_c, cd_a, cd_b, cd_c);
        bool has_cd = std::abs(area_line_a * cd_b - cd_a * area_line_b) > EPS;

        // Determine which side points are on relative to line a-d
        double side_b = triangle_area2(a, d, b);
        double side_c = triangle_area2(a, d, c);

        // If both intersections exist, choose the better one based on geometry
        if (has_ab && has_cd)
        {
            // Prefer the intersection that lies between b and c
            double dist_to_ab = std::abs(triangle_area2(a, d, e_ab));
            double dist_to_cd = std::abs(triangle_area2(a, d, e_cd));

            if (side_b * side_c > 0)
            { // b and c on same side
                return (dist_to_ab < dist_to_cd) ? e_ab : e_cd;
            }
            else
            { // b and c on opposite sides
                return (dist_to_ab > dist_to_cd) ? e_ab : e_cd;
            }
        }

        // Return available intersection or fallback to midpoint
        if (has_ab)
            return e_ab;
        if (has_cd)
            return e_cd;
        return {(b.x + c.x) * 0.5, (b.y + c.y) * 0.5};
    }
    // Calculate local area displacement cost
    double local_areal_displacement(const Point &a, const Point &b,
                                    const Point &c, const Point &d,
                                    const Point &e)
    {
        const double area_abe = 0.5 * std::abs(triangle_area2(a, b, e));
        const double area_bce = 0.5 * std::abs(triangle_area2(b, c, e));
        const double area_cde = 0.5 * std::abs(triangle_area2(c, d, e));
        return area_abe + area_bce + area_cde;
    }

    // Check if collapse maintains topology
    bool collapse_maintains_topology(const Polygon &polygon, int ring_id,
                                     int start_index, const Point &a,
                                     const Point &d, const Point &e)
    {
        // Check for intersections with new edges
        const Ring &ring = polygon.rings[ring_id];
        const int n = ring.vertices.size();

        for (size_t rr = 0; rr < polygon.rings.size(); ++rr)
        {
            const Ring &other = polygon.rings[rr];
            for (size_t j = 0; j < other.vertices.size(); ++j)
            {
                const Point &p = other.vertices[j];
                const Point &q = other.vertices[(j + 1) % other.vertices.size()];

                // Skip edges being collapsed
                if (static_cast<int>(rr) == ring_id)
                {
                    if (static_cast<int>(j) == start_index ||
                        static_cast<int>(j) == (start_index + 1) % n ||
                        static_cast<int>(j) == (start_index + 2) % n)
                    {
                        continue;
                    }
                }

                if (segments_properly_intersect(a, e, p, q, EPS) ||
                    segments_properly_intersect(e, d, p, q, EPS))
                {
                    return false;
                }
            }
        }
        return true;
    }
}

// APSC-based simplification
SimplificationResult simplify_polygon_APSC(const Polygon &polygon, int target_vertices)
{
    if (target_vertices < 3)
    {
        throw std::runtime_error("target_vertices must be at least 3");
    }

    SimplificationResult result;
    result.polygon = polygon;
    result.total_areal_displacement = 0.0;

    int current_vertices = total_vertex_count(result.polygon);
    std::cout << "APSC: Starting from " << current_vertices << " to " << target_vertices << std::endl;

    if (current_vertices <= target_vertices)
    {
        return result;
    }

    // Main simplification loop
    while (total_vertex_count(result.polygon) > target_vertices)
    {
        CollapseCandidate best;
        best.valid = false;
        double best_cost = std::numeric_limits<double>::infinity();

        // Find best collapse across all rings
        for (size_t r = 0; r < result.polygon.rings.size(); ++r)
        {
            Ring &ring = result.polygon.rings[r];
            const int n = ring.vertices.size();

            if (n < 4)
                continue;
            if (r != 0 && n < 5)
                continue; // Holes need min 4 after collapse

            // Try each possible collapse
            for (int i = 0; i < n; ++i)
            {
                const Point &a = ring.vertices[i];
                const Point &b = ring.vertices[(i + 1) % n];
                const Point &c = ring.vertices[(i + 2) % n];
                const Point &d = ring.vertices[(i + 3) % n];

                Point e = choose_area_preserving_point(a, b, c, d);

                // Check if point is valid
                if (same_point(e, a, EPS) || same_point(e, b, EPS) ||
                    same_point(e, c, EPS) || same_point(e, d, EPS))
                {
                    continue;
                }

                // Check topology
                if (!collapse_maintains_topology(result.polygon, r, i, a, d, e))
                {
                    continue;
                }

                double cost = local_areal_displacement(a, b, c, d, e);

                if (cost < best_cost - EPS)
                {
                    best_cost = cost;
                    best.valid = true;
                    best.ring_id = r;
                    best.start_index = i;
                    best.b_index = (i + 1) % n;
                    best.c_index = (i + 2) % n;
                    best.replacement = e;
                    best.cost = cost;
                }
            }
        }

        if (!best.valid)
        {
            std::cout << "No valid collapses found, stopping early" << std::endl;
            break;
        }

        // Apply the best collapse
        Ring &ring = result.polygon.rings[best.ring_id];
        Ring new_ring;
        new_ring.ring_id = ring.ring_id;

        for (size_t i = 0; i < ring.vertices.size(); ++i)
        {
            if (static_cast<int>(i) == best.b_index)
            {
                new_ring.vertices.push_back(best.replacement);
            }
            else if (static_cast<int>(i) == best.c_index)
            {
                continue;
            }
            else
            {
                new_ring.vertices.push_back(ring.vertices[i]);
            }
        }

        ring = new_ring;
        result.total_areal_displacement += best.cost;

        current_vertices = total_vertex_count(result.polygon);
        std::cout << "Collapsed to " << current_vertices << " vertices (cost: " << best.cost << ")" << std::endl;
    }

    std::cout << "APSC final vertex count: " << total_vertex_count(result.polygon) << std::endl;
    return result;
}

SimplificationResult simplify_polygon(const Polygon &polygon, int target_vertices)
{
    if (target_vertices < 3)
    {
        throw std::runtime_error("target_vertices must be at least 3");
    }

    SimplificationResult result;
    result.polygon = polygon;
    result.total_areal_displacement = 0.0;

    int current_vertices = total_vertex_count(result.polygon);

    if (current_vertices <= target_vertices)
    {
        std::cout << "Already at or below target" << std::endl;
        return result;
    }

    // Calculate vertex budget per ring
    int total_rings = static_cast<int>(result.polygon.rings.size());
    std::vector<int> ring_targets(total_rings);

    // Distribute target vertices among rings proportionally to their current size
    int remaining_budget = target_vertices;
    for (int i = 0; i < total_rings - 1; ++i)
    {
        double proportion = static_cast<double>(result.polygon.rings[i].vertices.size()) / current_vertices;
        ring_targets[i] = std::max(3, static_cast<int>(std::round(proportion * target_vertices)));
        if (i != 0 && ring_targets[i] < 4) // Holes need at least 4
            ring_targets[i] = 4;
        remaining_budget -= ring_targets[i];
    }
    // Last ring gets the remainder
    ring_targets[total_rings - 1] = std::max(3, remaining_budget);
    if (total_rings - 1 != 0 && ring_targets[total_rings - 1] < 4)
        ring_targets[total_rings - 1] = 4;

    // Choose simplification method based on reduction ratio
    double reduction_ratio = static_cast<double>(target_vertices) / current_vertices;

    // Apply simplification to each ring
    for (int i = 0; i < total_rings; ++i)
    {
        Ring &ring = result.polygon.rings[i];
        int original_size = static_cast<int>(ring.vertices.size());
        int target_size = ring_targets[i];

        if (original_size <= target_size)
        {
            std::cout << "Ring " << i << ": " << original_size << " -> " << original_size
                      << " (already at or below target)" << std::endl;
            continue;
        }

        Ring simplified;

        // Choose algorithm based on reduction needed
        if (reduction_ratio < 0.3)
        {
            // Aggressive reduction - use adaptive sampling
            simplified = simplify_ring_adaptive(ring, target_size);
        }
        else if (reduction_ratio < 0.7)
        {
            // Moderate reduction - use Visvalingam-Whyatt (area-based)
            simplified = simplify_ring_to_target(ring, target_size);
        }
        else
        {
            // Mild reduction - use uniform sampling
            simplified = simplify_ring_uniform(ring, target_size);
        }

        // Calculate area change
        double old_area = std::abs(signed_area(ring));
        double new_area = std::abs(signed_area(simplified));
        double area_change = std::abs(old_area - new_area);

        ring = simplified;
        result.total_areal_displacement += area_change;
    }

    std::cout << "Final vertex count: " << total_vertex_count(result.polygon)
              << " (target: " << target_vertices << ")" << std::endl;

    return result;
}

SimplificationResult simplify_polygon_hybrid(const Polygon &polygon, int target_vertices)
{
    int current = total_vertex_count(polygon);
    double reduction_ratio = static_cast<double>(target_vertices) / current;

    if (reduction_ratio > 0.7)
    {
        // Mild reduction: Use fast uniform sampling
        std::cout << "Using fast uniform sampling" << std::endl;
        return simplify_polygon(polygon, target_vertices); // Your current method
    }
    else if (reduction_ratio > 0.3)
    {
        // Moderate reduction: Use APSC for better quality
        std::cout << "Using APSC for moderate reduction" << std::endl;
        return simplify_polygon_APSC(polygon, target_vertices);
    }
    else
    {
        // Aggressive reduction: Two-pass approach
        std::cout << "Using hybrid two-pass approach" << std::endl;

        // First pass: APSC to ~2x target
        int intermediate = target_vertices * 2;
        auto intermediate_result = simplify_polygon_APSC(polygon, intermediate);

        // Second pass: Uniform sampling to exact target
        return simplify_polygon(intermediate_result.polygon, target_vertices);
    }
}
