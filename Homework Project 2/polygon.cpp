#include "polygon.h"

constexpr double EPS = 1e-9;


double signed_area_of_points(const std::vector<Point> &vertices)
{
    if (vertices.size() < 3)
    {
        return 0.0;
    }

    double twice_area = 0.0;
    for (std::size_t i = 0; i < vertices.size(); ++i)
    {
        const Point &p = vertices[i];
        const Point &q = vertices[(i + 1) % vertices.size()];
        twice_area += p.x * q.y - q.x * p.y;
    }

    return 0.5 * twice_area;
}

double signed_area(const Ring &ring)
{
    return signed_area_of_points(ring.vertices);
}

double total_signed_area(const Polygon &polygon)
{
    double total = 0.0;
    for (const Ring &ring : polygon.rings)
    {
        total += signed_area(ring);
    }
    return total;
}

int total_vertex_count(const Polygon &polygon)
{
    int total = 0;
    for (const Ring &ring : polygon.rings)
    {
        total += static_cast<int>(ring.vertices.size());
    }
    return total;
}

double vertex_importance(const Point &a, const Point &b, const Point &c)
{
    // Area of triangle = contribution of vertex b
    return std::abs(triangle_area2(a, b, c)) * 0.5;
}

// Simplify a ring to target size using Visvalingam-Whyatt algorithm (area-based)
Ring simplify_ring_to_target(const Ring& ring, int target_size) {
    Ring result;
    result.ring_id = ring.ring_id;
    
    const int n = static_cast<int>(ring.vertices.size());
    if (n <= target_size || target_size < 3) {
        return ring;
    }
    
    // For holes, ensure minimum 4 vertices
    int effective_target = target_size;
    if (ring.ring_id != 0 && effective_target < 4 && n > 4) {
        effective_target = 4;
    }
    
    // Create a list of indices to keep
    std::vector<bool> keep(n, true);
    int vertices_to_remove = n - effective_target;
    
    // Calculate importance for each vertex
    struct VertexImportance {
        int index;
        double importance;
        bool operator<(const VertexImportance& other) const {
            return importance > other.importance; // Min-heap
        }
    };
    
    std::priority_queue<VertexImportance> pq;
    std::vector<double> importance(n, 0.0);
    std::vector<int> prev(n), next(n);
    
    // Initialize linked list
    for (int i = 0; i < n; ++i) {
        prev[i] = (i - 1 + n) % n;
        next[i] = (i + 1) % n;
    }
    
    // Calculate initial importance for all vertices
    for (int i = 0; i < n; ++i) {
        const Point& a = ring.vertices[prev[i]];
        const Point& b = ring.vertices[i];
        const Point& c = ring.vertices[next[i]];
        importance[i] = vertex_importance(a, b, c);
        pq.push({i, importance[i]});
    }
    
    // Remove vertices with smallest importance
    int removed = 0;
    while (removed < vertices_to_remove && !pq.empty()) {
        VertexImportance vi = pq.top();
        pq.pop();
        
        if (!keep[vi.index] || std::abs(importance[vi.index] - vi.importance) > EPS) {
            continue; // Skip if already removed or importance changed
        }
        
        // Check if removing would cause self-intersection or invalid ring
        //const Point& a = ring.vertices[prev[vi.index]];
        //const Point& b = ring.vertices[vi.index];
        //const Point& c = ring.vertices[next[vi.index]];
        
        // Don't remove if it would make the ring invalid
        if (ring.ring_id != 0 && (removed + 1 >= vertices_to_remove)) {
            // For holes, be more conservative near the end
            if (effective_target - (n - removed) <= 1) {
                break;
            }
        }
        
        // Remove the vertex
        keep[vi.index] = false;
        removed++;
        
        // Update neighbors
        int p = prev[vi.index];
        int nxt = next[vi.index];
        next[p] = nxt;
        prev[nxt] = p;
        
        // Recalculate importance for affected vertices
        if (keep[p]) {
            const Point& pp = ring.vertices[prev[p]];
            const Point& pv = ring.vertices[p];
            const Point& pn = ring.vertices[next[p]];
            importance[p] = vertex_importance(pp, pv, pn);
            pq.push({p, importance[p]});
        }
        
        if (keep[nxt]) {
            const Point& np = ring.vertices[prev[nxt]];
            const Point& nv = ring.vertices[nxt];
            const Point& nn = ring.vertices[next[nxt]];
            importance[nxt] = vertex_importance(np, nv, nn);
            pq.push({nxt, importance[nxt]});
        }
    }
    
    // Build result from kept vertices
    for (int i = 0; i < n; ++i) {
        if (keep[i]) {
            result.vertices.push_back(ring.vertices[i]);
        }
    }
    
    // Ensure result has the correct orientation (holes should be opposite orientation)
    if (result.vertices.size() >= 3) {
        double original_area = signed_area(ring);
        double new_area = signed_area(result);
        if ((original_area > 0) != (new_area > 0)) {
            std::reverse(result.vertices.begin(), result.vertices.end());
        }
    }
    
    return result;
}


// Alternative: Simple uniform sampling (faster but less accurate)
Ring simplify_ring_uniform(const Ring& ring, int target_size) {
    Ring result;
    result.ring_id = ring.ring_id;
    
    const int n = static_cast<int>(ring.vertices.size());
    if (n <= target_size || target_size < 3) {
        return ring;
    }
    
    int effective_target = target_size;
    if (ring.ring_id != 0 && effective_target < 4 && n > 4) {
        effective_target = 4;
    }
    
    // Sample evenly around the ring
    double step = static_cast<double>(n) / effective_target;
    for (int i = 0; i < effective_target; ++i) {
        int idx = static_cast<int>(std::round(i * step));
        if (idx >= n) idx = n - 1;
        result.vertices.push_back(ring.vertices[idx]);
    }
    
    // Ensure orientation is preserved
    double original_area = signed_area(ring);
    double new_area = signed_area(result);
    if ((original_area > 0) != (new_area > 0)) {
        std::reverse(result.vertices.begin(), result.vertices.end());
    }
    
    return result;
}

// Advanced: Adaptive sampling based on curvature
Ring simplify_ring_adaptive(const Ring& ring, int target_size) {
    Ring result;
    result.ring_id = ring.ring_id;
    
    const int n = static_cast<int>(ring.vertices.size());
    if (n <= target_size || target_size < 3) {
        return ring;
    }
    
    int effective_target = target_size;
    if (ring.ring_id != 0 && effective_target < 4 && n > 4) {
        effective_target = 4;
    }
    
    // Calculate curvature (angle change) at each vertex
    std::vector<double> curvature(n, 0.0);
    double total_curvature = 0.0;
    
    for (int i = 0; i < n; ++i) {
        const Point& prev = ring.vertices[(i - 1 + n) % n];
        const Point& curr = ring.vertices[i];
        const Point& next = ring.vertices[(i + 1) % n];
        
        // Calculate angle using dot product
        Point v1 = {prev.x - curr.x, prev.y - curr.y};
        Point v2 = {next.x - curr.x, next.y - curr.y};
        double len1 = std::sqrt(v1.x * v1.x + v1.y * v1.y);
        double len2 = std::sqrt(v2.x * v2.x + v2.y * v2.y);
        
        if (len1 > EPS && len2 > EPS) {
            double dot_prod = v1.x * v2.x + v1.y * v2.y;
            double cos_angle = dot_prod / (len1 * len2);
            cos_angle = std::max(-1.0, std::min(1.0, cos_angle));
            double angle = std::acos(cos_angle);
            curvature[i] = angle;
        } else {
            curvature[i] = 0.0;
        }
        total_curvature += curvature[i];
    }
    
    // Sample based on curvature (more vertices in high-curvature areas)
    if (total_curvature > EPS) {
        std::vector<double> cumulative(n, 0.0);
        cumulative[0] = curvature[0];
        for (int i = 1; i < n; ++i) {
            cumulative[i] = cumulative[i-1] + curvature[i];
        }
        
        for (int i = 0; i < effective_target; ++i) {
            double target = (i / static_cast<double>(effective_target)) * total_curvature;
            int idx = 0;
            while (idx < n && cumulative[idx] < target) {
                idx++;
            }
            if (idx >= n) idx = n - 1;
            result.vertices.push_back(ring.vertices[idx]);
        }
        
        // Remove duplicates
        std::vector<Point> unique;
        for (size_t i = 0; i < result.vertices.size(); ++i) {
            if (i == 0 || !same_point(result.vertices[i], result.vertices[i-1], EPS)) {
                unique.push_back(result.vertices[i]);
            }
        }
        result.vertices = unique;
        
        // If we didn't get enough vertices, fall back to uniform sampling
        if (static_cast<int>(result.vertices.size()) < effective_target) {
            return simplify_ring_uniform(ring, target_size);
        }
    } else {
        // Fall back to uniform sampling
        return simplify_ring_uniform(ring, target_size);
    }
    
    // Ensure orientation is preserved
    double original_area = signed_area(ring);
    double new_area = signed_area(result);
    if ((original_area > 0) != (new_area > 0)) {
        std::reverse(result.vertices.begin(), result.vertices.end());
    }
    
    return result;
}