/*
 * Copyright (c) 2010 Kolja Glogowski
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "pvutils.h"

#include <ctime>
#include <cerrno>

#ifdef _WIN32
#include <windows.h>
int msleep(unsigned int ms)
{
    Sleep(DWORD(ms));
    return 0;
}
#else
int msleep(unsigned int ms)
{
    time_t s = static_cast<time_t>(ms / 1000);
    long ns = (ms - 1000 * s) * 1000000L;

    timespec t, r;
    t.tv_sec = s;
    t.tv_nsec = ns;

    while (nanosleep(&t, &r) == -1)
    {
        if (errno == EINTR)
            t = r;
        else
            return -1;
    }

    return 0;
}
#endif


static const int PvErrorCodeCount = 23;

static const char *PvErrorCodeStrList[PvErrorCodeCount] = {
    "ePvErrSuccess",
    "ePvErrCameraFault",
    "ePvErrInternalFault",
    "ePvErrBadHandle",
    "ePvErrBadParameter",
    "ePvErrBadSequence",
    "ePvErrNotFound",
    "ePvErrAccessDenied",
    "ePvErrUnplugged",
    "ePvErrInvalidSetup",
    "ePvErrResources",
    "ePvErrBandwidth",
    "ePvErrQueueFull",
    "ePvErrBufferTooSmall",
    "ePvErrCancelled",
    "ePvErrDataLost",
    "ePvErrDataMissing",
    "ePvErrTimeout",
    "ePvErrOutOfRange",
    "ePvErrWrongType",
    "ePvErrForbidden",
    "ePvErrUnavailable",
    "ePvErrFirewall"
};

static const char *PvErrorMessageList[PvErrorCodeCount] = {
    "No error",
    "Unexpected camera fault",
    "Unexpected fault in PvApi or driver",
    "Camera handle is invalid",
    "Bad parameter to API call",
    "Sequence of API calls is incorrect",
    "Camera or attribute not found",
    "Camera cannot be opened in the specified mode",
    "Camera was unplugged",
    "Setup is invalid (an attribute is invalid)",
    "System/network resources or memory not available",
    "1394 bandwidth not available",
    "Too many frames on queue",
    "Frame buffer is too small",
    "Frame cancelled by user",
    "The data for the frame was lost",
    "Some data in the frame is missing",
    "Timeout during wait",
    "Attribute value is out of the expected range",
    "Attribute is not this type (wrong access function)",
    "Attribute write forbidden at this time",
    "Attribute is not available at this time",
    "A firewall is blocking the traffic"
};

const char * PvErrorCodeStr(tPvErr code)
{
    if (code >= 0 && code < PvErrorCodeCount)
        return PvErrorCodeStrList[code];
    else
        return "";
}

const char * PvErrorMessage(tPvErr code)
{
    if (code >= 0 && code < PvErrorCodeCount)
        return PvErrorMessageList[code];
    else
        return "Unkown error";
}


