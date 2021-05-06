#pragma once

#ifndef CONFIG_FILE
#define CONFIG_FILE "/etc/backlight-tooler.conf"
#endif // CONFIG_FILE

#include <string>
#include <vector>
#include <iostream>
#include "functions.hpp"

using std::string;
using std::vector;
using std::ostream;

class InputKey;
typedef class InputKey InputKey;

class Input {
    public:
        Input( int* argc, char*** argv );
        ~Input();

        string webcamDevice = "/dev/video0";
        size_t webcamWidth = 1280,
               webcamHeight = 720,
               webcamLightValLow = 77,
               webcamLightValHigh = 200;

        string backlightDevice = "/sys/class/backlight/intel_backlight/brightness";
        size_t minBrightness = 10,
               maxBrightness = 1000;

        string keyboardDevice = "/sys/devices/platform/thinkpad_acpi/leds/tpacpi::kbd_backlight/brightness";
        size_t keyboardMaxBrightness = 2,
               keyboardMinBrightness = 0;
        bool useKeyboard = true;

        bool readMinMaxBrightness = true;
        double minAfterRead = 0.01;

        string functionChoice = "linear";
        vector<double> functionParams = {};

        BacklightFunction chosenFunction;

        string mode = "";
        double value = -1.0;
        bool verbose = false;
        void setConfigFile( const string& fname ) { _configFile = fname; }
        friend ostream& operator<<( ostream&, const Input& );

    private:
        int _argc;
        char** _argv;
        string _configFile = CONFIG_FILE;
        int _dump_self = 0;

        void _read_config_file();
        void _options_from_argv_vector( vector<char*>& argv );
        void _options_from_argc_argv( int argc, char** argv );

        bool _callback_func( InputKey* );
};

