#pragma once

#include <string>
#include <vector>

using std::string;
using std::vector;

typedef double (*BacklightFunction)(double, const vector<double>&);

double LinearFunction(double x, const vector<double>& p);
double PowerFunction(double x, const vector<double>& p);
double ExpFunction(double x, const vector<double>& p);
double LogFunction(double x, const vector<double>& p);
double FactorialFunction(double x, const vector<double>& p);
double MinkowskiFunction(double x, const vector<double>& p);
double SineFunction(double x, const vector<double>& p);

BacklightFunction chooseFunction( const string& functionChoice );

double normalizeValue( size_t webcamLow, size_t webcamHigh, size_t level );


