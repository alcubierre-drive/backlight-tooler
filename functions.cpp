#include "functions.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

using std::cerr;
using std::endl;

double normalizeValue( size_t webcamLow, size_t webcamHigh, size_t level ) {
    level = level <= webcamLow ? webcamLow : level;
    level = level >= webcamHigh ? webcamHigh : level;
    return (double)(level - webcamLow) / (double)(webcamHigh - webcamLow);
}

double LinearFunction(double x, const vector<double>&) {
    return x;
}

double LogFunction(double x, const vector<double>& p) {
    assert(p.size() >= 1);
    assert(p[0] > 0.);
    return log( x*(exp(p[0])-1.) + 1. )/p[0];
}

double ExpFunction(double x, const vector<double>& p) {
    assert(p.size() >= 1);
    assert(p[0] > 0.);
    return (exp(p[0]*x)-1.)/(exp(p[0])-1.);
}

double PowerFunction(double x, const vector<double>& p) {
    assert(p.size() >= 1);
    assert(p[0] > 0.);
    return pow(x,p[0]);
}

double FactorialFunction(double x, const vector<double>&) {
    return gamma(x+2.)-1.;
}

double SineFunction(double x, const vector<double>&) {
    return sin(x*M_PI*0.5);
}

double MinkowskiFunction(double x, const vector<double>&) {
    float arg = x;
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

BacklightFunction chooseFunction( const string& functionChoice ) {
    if (functionChoice == "linear") return LinearFunction;
    if (functionChoice == "power") return PowerFunction;
    if (functionChoice == "exp") return ExpFunction;
    if (functionChoice == "log") return LogFunction;
    if (functionChoice == "factorial") return FactorialFunction;
    if (functionChoice == "sine") return SineFunction;
    if (functionChoice == "?") return MinkowskiFunction;
    cerr << "'" << functionChoice << "' is an invalid function choice." << endl
         << "Valid options are 'linear', 'power', 'exp', 'log', 'factorial' and 'sine'." << endl;
    return LinearFunction;
}
