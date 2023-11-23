#ifndef RASTER_ARGS_HPP
#define RASTER_ARGS_HPP

#include "Algebra.hpp"

// rasterizar segmento entre dos puntos
void draw_line(Point p0, Point p1,bool antialiasing);

struct curve_ret_t { Point p; Vector d; };
using curve_func_t = curve_ret_t(*)(float);

// rasterizar curva
void draw_curve(curve_func_t f);


#endif

