#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <math.h>
#include "readconfig.h"

float normalize( void**, int level );

float logarithmic( float* );
float exponential( float* );
float power( float* );
float factorial( float* );
float sine( float* );
float minkowski( float* );

int scaled_brightness( void**, int level, float (*scale)(float*),
        float* params, int nparams );

int choose_function_and_params( void**, int );

#endif // FUNCTIONS_H
