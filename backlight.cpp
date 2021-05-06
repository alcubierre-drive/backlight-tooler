#include "backlight.hpp"
#include <regex>
#include <fstream>

#define _XOPEN_SOURCE 700
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

Backlight::Backlight( Input& input ) : _input(input) {
    if (_input.readMinMaxBrightness) {
        _input.maxBrightness = _readMaxBrightness( _input.backlightDevice );
        _input.minBrightness = _input.maxBrightness * _input.minAfterRead;
        if (_input.useKeyboard) {
            _input.keyboardMaxBrightness = _readMaxBrightness( _input.keyboardDevice );
            _input.keyboardMinBrightness = _input.keyboardMaxBrightness * _input.minAfterRead;
        }
    }
    currentBrightness = _readBrightness( _input.backlightDevice );
    keyboardCurrentBrightness = _input.useKeyboard ? _readBrightness( _input.keyboardDevice ) : 0;
}

size_t Backlight::_readMaxBrightness( const string& file ) {
    string maxfile = std::regex_replace(file, std::regex("brightness$"), "max_brightness");
    return _readBrightness(maxfile);
}

size_t Backlight::_readBrightness( const string& file ) {
    std::ifstream infile(file);
    size_t result = 0;
    if (!infile) {
        std::cerr << "could not read '" << file << "'." << std::endl;
        return result;
    }
    infile >> result;
    infile.close();
    return result;
}

double Backlight::absoluteToRelative( size_t br ) {
    double x = (double)(br - _input.minBrightness) / (_input.maxBrightness - _input.minBrightness);
    return x >= 1.0 ? 1.0 : (x <= 0.0 ? 0.0 : x);
}

void Backlight::setBrightnessUnbound( size_t br, size_t kbr ) {
    std::ofstream brfile(_input.backlightDevice);
    if (!brfile) {
        std::cerr << "could not write '" << _input.backlightDevice << "'." << std::endl;
        return;
    }
    brfile << br;
    brfile.close();
    if (_input.useKeyboard) {
        std::ofstream kbrfile(_input.keyboardDevice);
        if (!kbrfile) {
            std::cerr << "could not write '" << _input.keyboardDevice << "'." << std::endl;
            return;
        }
        kbrfile << kbr;
        kbrfile.close();
    }
}

void Backlight::setBrightness( double x ) {
    double newVal_ = _input.chosenFunction( x, _input.functionParams ) *
        (_input.maxBrightness - _input.minBrightness) + _input.minBrightness;
    double newKbdVal_ = _input.chosenFunction( 1.-x, _input.functionParams ) *
        (_input.keyboardMaxBrightness - _input.keyboardMinBrightness) + _input.keyboardMinBrightness;
    size_t newVal = newVal_ > _input.maxBrightness ? _input.maxBrightness : newVal_;
    newVal = newVal < _input.minBrightness ? _input.minBrightness : newVal;
    size_t newKbdVal = newKbdVal_ > _input.keyboardMaxBrightness ? _input.keyboardMaxBrightness : newKbdVal_;
    newKbdVal = newKbdVal < _input.keyboardMinBrightness ? _input.keyboardMinBrightness : newKbdVal;
    setBrightnessUnbound( newVal, newKbdVal );
}

Webcam::Webcam( Input& input ) : _input(input) {
    currentLightLevel = getLightLevel(_input.webcamDevice.c_str(), _input.webcamWidth, _input.webcamHeight);
}

int Webcam::getLightLevel(const char* name, int width, int height) {

    int fd = open(name, O_RDWR);
    struct v4l2_capability cap;
    struct v4l2_format format;
    ioctl(fd, VIDIOC_QUERYCAP, &cap);
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    format.fmt.pix.width = width;
    format.fmt.pix.height = height;

    ioctl(fd, VIDIOC_S_FMT, &format);
    struct v4l2_requestbuffers bufrequest;
    bufrequest.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bufrequest.memory = V4L2_MEMORY_MMAP;
    bufrequest.count = 1;

    ioctl(fd, VIDIOC_REQBUFS, &bufrequest);
    struct v4l2_buffer bufferinfo;
    memset(&bufferinfo, 0, sizeof(bufferinfo));

    bufferinfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bufferinfo.memory = V4L2_MEMORY_MMAP;
    bufferinfo.index = 0;

    ioctl(fd, VIDIOC_QUERYBUF, &bufferinfo);

    void* buffer_start = mmap(
        NULL,
        bufferinfo.length,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd,
        bufferinfo.m.offset
    );

    memset(buffer_start, 0, bufferinfo.length);
    memset(&bufferinfo, 0, sizeof(bufferinfo));

    bufferinfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bufferinfo.memory = V4L2_MEMORY_MMAP;
    bufferinfo.index = 0;

    int type = bufferinfo.type;
    ioctl(fd, VIDIOC_STREAMON, &type);
    ioctl(fd, VIDIOC_QBUF, &bufferinfo);
    ioctl(fd, VIDIOC_DQBUF, &bufferinfo);
    ioctl(fd, VIDIOC_STREAMOFF, &type);

    long total = 0;
    unsigned i = 0;
    for ( i = 0; i != bufferinfo.length; ++i) {
        total += ((unsigned char*)buffer_start)[i];
    }
    munmap(buffer_start, bufferinfo.length);
    return total/i;
}
