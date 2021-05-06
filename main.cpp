#include "input.hpp"
#include "functions.hpp"
#include "backlight.hpp"
#include <iostream>

using std::cout;
using std::endl;
using std::string;

// TODO enable verbose logging

int main( int argc_, char** argv_ ) {
    int argc = argc_;
    char** argv = argv_;
    Input inp( &argc, &argv );
    Backlight back( inp );
    Webcam* pweb = (inp.mode == "info" || inp.mode == "auto") ?
        new Webcam( inp ) : nullptr;

    if (inp.mode == "info") {
        cout << "webcam level: " << pweb->currentLightLevel << endl
             << "backlight level: " << back.currentBrightness << endl;
        if (inp.useKeyboard)
            cout << "keyboard level: " << back.keyboardCurrentBrightness << endl;
        if (inp.readMinMaxBrightness) {
            cout << "max brightness read: " << inp.maxBrightness << endl;
            if (inp.useKeyboard)
                cout << "keyboard max brightness read: " << inp.keyboardMaxBrightness << endl;
        }
    } else if (inp.mode == "set") {
        back.setBrightness( inp.value );
    } else if (inp.mode == "inc") {
        back.setBrightness( back.absoluteToRelative(back.currentBrightness) +
                            inp.value );
    } else if (inp.mode == "dec") {
        back.setBrightness( back.absoluteToRelative(back.currentBrightness) -
                            inp.value );
    } else if (inp.mode == "auto") {
        double x = (double)(pweb->currentLightLevel - inp.webcamLightValLow) /
                   (double)(inp.webcamLightValHigh - inp.webcamLightValLow);
        x = x >= 1.0 ? 1.0 : (x <= 0.0 ? 0.0 : x);
        back.setBrightness( x );
    } else if (inp.mode == "toggle") {
        if (back.currentBrightness <= inp.minBrightness)
            back.setBrightnessUnbound( inp.maxBrightness, 0 );
        else
            back.setBrightnessUnbound( 0, 0 );
    } else {
        cout << "unknown mode '" << inp.mode << "'." << endl;
    }

    if (pweb) delete pweb;
}
