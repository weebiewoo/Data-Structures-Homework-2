#include <exception>
#include <iostream>
#include <string>

#include "io.h"
#include "polygon.h"
#include "simplifier.h"

int main(int argc, char *argv[])
{
    try
    {
        if (argc < 3)
        {
            std::cerr << "Usage: ./simplify <input_file.csv> <target_vertices>\n";
            return 1;
        }

        const std::string input_file = argv[1];
        const int target_vertices = std::stoi(argv[2]);
        const std::string output_file{};

        if (target_vertices < 3)
        {
            std::cerr << "target_vertices must be at least 3\n";
            return 1;
        }

        const Polygon polygon = read_polygon_from_csv(input_file);
        const int total_vertex_count_input = total_vertex_count(polygon);

        if (target_vertices >= total_vertex_count_input)
        {
            std::cerr << "Target vertices need to be lower than input's total vertices(" << total_vertex_count_input << ")\n";
            return 1;
        }

        const SimplificationResult result = simplify_polygon_hybrid(polygon, target_vertices);
        const double input_area = total_signed_area(polygon);
        const double output_area = total_signed_area(result.polygon);

        write_polygon_to_stdout(result.polygon, input_area, output_area, result.total_areal_displacement);

        if (argc > 3)
        {
            const std::string output_file = argv[3];
            write_polygon_to_file(result.polygon, input_area, output_area, result.total_areal_displacement, output_file);
        }
        return 0;
    }
    catch (const std::exception &ex)
    {
        std::cerr << ex.what() << '\n';
        return 1;
    }
}