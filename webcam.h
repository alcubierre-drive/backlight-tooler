#ifndef WEBCAM_H
#define WEBCAM_H

#include "readconfig.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>

int getLightLevel(void**);

#endif // WEBCAM_H
