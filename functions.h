#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <math.h>
#include "readconfig.h"

float normalize( config*, int level );

float logarithmic( float* );
float exponential( float* );
float power( float* );
float factorial( float* );

int scaled_brightness( config*, int level, float (*scale)(float*),
        float* params, int nparams );

int choose_function_and_params( config*, int );

#endif // FUNCTIONS_H
