#ifndef IO_H
#define IO_H

#include <string>

#include "polygon.h"

Polygon read_polygon_from_csv(const std::string& filename);

void write_polygon_to_stdout(const Polygon& polygon,
                             double input_area,
                             double output_area,
                             double areal_displacement);

#endif