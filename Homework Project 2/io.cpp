#include "io.h"

#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

std::string trim(const std::string& s) {
    std::size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
        ++start;
    }

    std::size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        --end;
    }

    return s.substr(start, end - start);
}

}  // namespace

Polygon read_polygon_from_csv(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open input file: " + filename);
    }

    std::string line;
    if (!std::getline(file, line)) {
        throw std::runtime_error("Input file is empty: " + filename);
    }

    if (line.size() >= 3 &&
        static_cast<unsigned char>(line[0]) == 0xEF &&
        static_cast<unsigned char>(line[1]) == 0xBB &&
        static_cast<unsigned char>(line[2]) == 0xBF) {
        line = line.substr(3);
    }

    if (trim(line) != "ring_id,vertex_id,x,y") {
        throw std::runtime_error("Invalid CSV header in file: " + filename);
    }

    std::map<int, std::vector<std::pair<int, Point>>> grouped;
    std::map<int, std::set<int>> seen_vertex_ids;

    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty()) {
            continue;
        }

        std::stringstream ss(line);
        std::string token;

        int ring_id = 0;
        int vertex_id = 0;
        double x = 0.0;
        double y = 0.0;

        if (!std::getline(ss, token, ',')) throw std::runtime_error("Malformed CSV line: " + line);
        ring_id = std::stoi(trim(token));

        if (!std::getline(ss, token, ',')) throw std::runtime_error("Malformed CSV line: " + line);
        vertex_id = std::stoi(trim(token));

        if (!std::getline(ss, token, ',')) throw std::runtime_error("Malformed CSV line: " + line);
        x = std::stod(trim(token));

        if (!std::getline(ss, token, ',')) throw std::runtime_error("Malformed CSV line: " + line);
        y = std::stod(trim(token));

        if (ring_id < 0) {
            throw std::runtime_error("Negative ring_id in input: " + std::to_string(ring_id));
        }
        if (vertex_id < 0) {
            throw std::runtime_error("Negative vertex_id in input for ring " + std::to_string(ring_id));
        }
        if (seen_vertex_ids[ring_id].count(vertex_id) != 0U) {
            throw std::runtime_error("Duplicate vertex_id " + std::to_string(vertex_id) +
                                     " in ring " + std::to_string(ring_id));
        }

        seen_vertex_ids[ring_id].insert(vertex_id);
        grouped[ring_id].push_back({vertex_id, Point{x, y}});
    }

    if (grouped.empty()) {
        throw std::runtime_error("Input file contains no vertices: " + filename);
    }

    Polygon polygon;
    int expected_ring_id = 0;

    for (const auto& [ring_id, entries] : grouped) {
        if (ring_id != expected_ring_id) {
            throw std::runtime_error("Ring ids must be contiguous starting at 0");
        }
        ++expected_ring_id;

        Ring ring;
        ring.ring_id = ring_id;
        ring.vertices.resize(entries.size());
        std::vector<bool> filled(entries.size(), false);

        for (const auto& [vertex_id, point] : entries) {
            if (vertex_id < 0 || vertex_id >= static_cast<int>(entries.size())) {
                throw std::runtime_error("vertex_id values in ring " + std::to_string(ring_id) +
                                         " must be contiguous from 0 to " +
                                         std::to_string(static_cast<int>(entries.size()) - 1));
            }
            ring.vertices[vertex_id] = point;
            filled[vertex_id] = true;
        }

        for (bool ok : filled) {
            if (!ok) {
                throw std::runtime_error("Missing vertex_id in ring " + std::to_string(ring_id));
            }
        }

        if (ring.vertices.size() < 3) {
            throw std::runtime_error("Each ring must have at least 3 vertices");
        }

        polygon.rings.push_back(ring);
    }

    return polygon;
}

void write_polygon_to_stdout(const Polygon& polygon,
                             double input_area,
                             double output_area,
                             double areal_displacement) {
    std::cout << "ring_id,vertex_id,x,y\n";

    std::streamsize old_precision = std::cout.precision();
    std::ios::fmtflags old_flags = std::cout.flags();

    std::cout << std::setprecision(15);
    for (std::size_t r = 0; r < polygon.rings.size(); ++r) {
        const Ring& ring = polygon.rings[r];
        for (std::size_t i = 0; i < ring.vertices.size(); ++i) {
            std::cout << r << ',' << i << ','
                      << ring.vertices[i].x << ','
                      << ring.vertices[i].y << '\n';
        }
    }

    std::cout.setf(std::ios::scientific);
    std::cout.precision(6);
    std::cout << "Total signed area in input: " << input_area << '\n';
    std::cout << "Total signed area in output: " << output_area << '\n';
    std::cout << "Total areal displacement: " << areal_displacement << '\n';

    std::cout.flags(old_flags);
    std::cout.precision(old_precision);
}