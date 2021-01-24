#ifndef CORDIC_HPP_
#define CORDIC_HPP_

namespace math {
    constexpr double PI = 3.1415926535L;

    void init();

    double mod(double n, double d);

    double cos(double x);
    double sin(double x);
    double tan(double x);
    double sqrt(double x);
}

#endif // CORDIC_HPP_

