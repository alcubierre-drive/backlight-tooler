#pragma once

#include "input.hpp"

class Backlight {
    public:
        Backlight( Input& );
        ~Backlight() {};

        size_t currentBrightness,
               keyboardCurrentBrightness;
        double absoluteToRelative( size_t br );
        void setBrightness( double x );
        void setBrightnessUnbound( size_t br, size_t kbr );
    private:
        Input& _input;
        size_t _readMaxBrightness( const string& file );
        size_t _readBrightness( const string& file );
};

class Webcam {
    public:
        Webcam( Input& );
        ~Webcam() {};

        size_t currentLightLevel;
    private:
        Input& _input;
        int getLightLevel(const char* name, int width, int height);
};
