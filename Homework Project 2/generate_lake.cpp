#include <iostream>
#include <fstream>
#include <cmath>
#include <random>
#include <numbers>

int main()
{
    const int N = 1850;
    const double base_radius = 1000.0;

    std::ofstream out("test_cases/jaggedlake.csv");
    out << "ring_id,vertex_id,x,y\n";

    std::mt19937 rng(42);
    //more jagged   -> noise(-200, 200)
    //smoother      -> noise(-30, 30)
    std::uniform_real_distribution<double> noise(-200.0, 200.0);

    for (int i = 0; i < N; ++i)
    {
        double angle = 2.0 * std::numbers::pi * i / N;

        // Add jaggedness using noise + sine waves
        // More "spiky" -> sin(20 * angle)
        double r = base_radius
                 + noise(rng)
                 + 50.0 * std::sin(5 * angle)
                 + 30.0 * std::sin(13 * angle);

        double x = 1455072.5064 + r * std::cos(angle);
        double y = -1372428.5066 + r * std::sin(angle);

        out << "0," << i << "," << x << "," << y << "\n";
    }

    out.close();
    return 0;
}