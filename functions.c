#include "functions.h"

float normalize( void** c, int webcam_level ) {
    int web_lo, web_hi;
    read_config(c,W_LOW,&web_lo);
    read_config(c,W_HIGH,&web_hi);
    // normalize the webcam light level. return value between 0 and 1.
    float result = (float)(webcam_level - web_lo) / (float)(web_hi - web_lo);
    if (result <= 0.0) {
        fprintf(stderr,"[functions] Warn: not normalized: %.3f, needs [0,1]\n",
                result);
        result = 0.0;
    } else if (result >= 1.0) {
        fprintf(stderr,"[functions] Warn: not normalized: %.3f, needs [0,1]\n",
                result);
        result = 1.0;
    }
    return result;
}

float logarithmic( float* x ) {
    if ( *(x+1) == 0.0 ) {
        fprintf(stderr,"[functions] Warn: log/exp c %.3f = 0, non-zero only.\n",
                *(x+1));
        *(x+1) = 1.0;
    }
    return log( (*x)*( exp(*(x+1)) - 1.0 ) + 1.0 )/(*(x+1));
}

float exponential( float* x ) {
    if ( *(x+1) == 0.0 ) {
        fprintf(stderr,"[functions] Warn: log/exp c %.3f = 0, non-zero only.\n",
                *(x+1));
        *(x+1) = 1.0;
    }
    return (exp( *(x+1)*(*x) )-1.0)/(exp( *(x+1) ) -1.0);
}

float power( float* x ) {
    if (*(x+1)<0) {
        fprintf(stderr,"[functions] Warn: exponent %.3f < 0, only positive values\n",
                *(x+1));
        *(x+1) *= -1.0;
    }
    return pow(*x,*(x+1));
}

float factorial( float* x ) {
    return gamma( *x+1.0 );
}

float sine( float* x ) {
    return sin( *x *M_PI*0.5 );
}

float minkowski( float* x ) {
    float arg = *x;
    long p = arg;
    if ((float)p > arg) --p;
    long q = 1, r = p + 1, s = 1, m, n;
    float d = 1, y = p;
    if (arg < (float)p || (p < 0) ^ (r <= 0))
        return arg;
    for (;;) {
        d /= 2;
        if (y + d == y)
            break;
        m = p + r;
        if ((m < 0) ^ (p < 0))
            break;
        n = q + s;
        if (n < 0)
            break;

        if (arg < (float)m / n) {
            r = m;
            s = n;
        } else {
            y += d;
            p = m;
            q = n;
        }
    }
    return y + d;
}

int scaled_brightness( void** c, int level, float (*scale)(float*),
        float* params, int nparams ) {

    float* funcparams = malloc((nparams+1)*sizeof(float));
    funcparams[0] = normalize( c, level );
    for (int i=0; i<nparams; ++i) {
        funcparams[i+1] = params[i];
    }
    float scaled = (*scale)(funcparams);
    free(funcparams);

    int maxbr, minbr;
    read_config(c,MAX_B,&maxbr);
    read_config(c,MIN_B,&minbr);

    return (maxbr-minbr)*scaled + minbr;
}

int choose_function_and_params( void** cfg, int level ) {
    char name[512];
    read_config(cfg,USE_FUNCTION,name);
    float param;
    int result;
    if (!strcmp(name,"linear")) {
        param = 1.0;
        result = scaled_brightness( cfg, level, &power, &param, 1 );
    } else if (!strcmp(name,"quadratic")) {
        param = 2.0;
        result = scaled_brightness( cfg, level, &power, &param, 1 );
    } else if (!strcmp(name,"power")) {
        read_config(cfg,FUNC_PARAM,&param);
        result = scaled_brightness( cfg, level, &power, &param, 1 );
    } else if (!strcmp(name,"exponential")) {
        read_config(cfg,FUNC_PARAM,&param);
        result = scaled_brightness( cfg, level, &exponential, &param, 1 );
    } else if (!strcmp(name,"logarithmic")) {
        read_config(cfg,FUNC_PARAM,&param);
        result = scaled_brightness( cfg, level, &logarithmic, &param, 1 );
    } else if (!strcmp(name,"factorial")) {
        result = scaled_brightness( cfg, level, &factorial, NULL, 0 );
    } else if (!strcmp(name,"sine")) {
        result = scaled_brightness( cfg, level, &sine, NULL, 0 );
    } else if (!strcmp(name,"?")) {
        // Minkowski's question-mark function.
        result = scaled_brightness( cfg, level, &minkowski, NULL, 0 );
    } else {
        param = 1.0;
        result = scaled_brightness( cfg, level, &power, &param, 1 );
        fprintf(stderr,"[functions] Unknown function name %s. Return linear.\n",
                name);
    }
    return result;
}
