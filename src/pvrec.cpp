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

#include "recorder.h"
#include "cmdopts.h"
#include "version.h"

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cstring>
using namespace std;

enum {
    E_OK = 0,
    E_ERR_GENERIC = 1,
    E_ERR_OPEN = 2,
    E_ERR_SETUP = 3,
    E_ERR_RECORD = 4
};

inline string permittedAccessString(unsigned long permittedAccess) {
    if ((permittedAccess & ePvAccessMaster) != 0)
        return string("Master");
    else if ((permittedAccess & ePvAccessMonitor) != 0)
        return string("Monitor");
    return string("None");
}

inline string interfaceTypeString(tPvInterface interfaceType) {
    if (interfaceType == ePvInterfaceEthernet)
        return string("GigE");
    else if (interfaceType == ePvInterfaceFirewire)
        return string("Firewire");
    return string("Unknown");
}

int main(int argc, char **argv)
{
    CmdLineOptions opts(argc, argv);
    switch (opts.parse())
    {
    case CmdLineOptions::Ok:
        break;
    case CmdLineOptions::Error:
        cout << opts.help_hint() << endl;
        return E_ERR_GENERIC;
    case CmdLineOptions::Help:
        cout << opts.help() << endl << endl;
        return E_OK;
    case CmdLineOptions::Version:
        cout << "PvRec version " << PVREC_VERSION_STRING << "\n"
                << PVREC_COPYRIGHT_STRING << endl;
        return E_OK;
    }

    Recorder rec(opts.numBuffers);
    cout << "PvApi Version: " << rec.apiVersionStr() << endl;

    if (opts.list || opts.info)
    {
        cout << "Searching for cameras..." << endl;
        Recorder::CameraInfoVector camInfoVec = rec.availableCameras(5000);

        if (camInfoVec.size() < 1) {
            cerr << "Error: No camera found." << endl;
            return E_ERR_OPEN;
        }

        if (opts.list) {
            cout << "\nAvailable Cameras:\n";
            size_t maxNameSize = 0;
            Recorder::CameraInfoVector::const_iterator it = camInfoVec.begin();
            for (; it != camInfoVec.end(); ++it)
                maxNameSize = max(strlen(it->CameraName), maxNameSize);

            for (it = camInfoVec.begin(); it != camInfoVec.end(); ++it) {
                cout << "    "
                     << setw(maxNameSize)
                     << it->CameraName << " - "
                     << it->SerialNumber << " - "
                     << "UniqueId: " << it->UniqueId << "\n";
            }
        }

        if (opts.info) {
            int i = 0;
            Recorder::CameraInfoVector::const_iterator it = camInfoVec.begin();
            for (; it != camInfoVec.end(); ++it, ++i) {
                if (opts.cameraId != 0 && opts.cameraId != it->UniqueId)
                    continue;
                cout << "\nCamera " << i << ":"
                     << "\n    UniqueId .......... " << it->UniqueId
                     << "\n    CameraName ........ " << it->CameraName
                     << "\n    ModelName ......... " << it->ModelName
                     << "\n    SerialNumber ...... " << it->SerialNumber
                     << "\n    FirmwareVersion ... " << it->FirmwareVersion
                     << "\n    PermittedAccess ... "
                            << permittedAccessString(it->PermittedAccess)
                     << "\n    InterfaceType ..... "
                            << interfaceTypeString(it->InterfaceType)
                     << "\n    InterfaceId ....... " << it->InterfaceId
                     << endl;
            }
        }

        cout << endl;
        return E_OK;
    }

    if (!opts.force && std::ifstream(opts.fname.c_str())) {
        cerr << "Error: '" << opts.fname << "' already exists. Use -f to "
             << "overwrite it." << endl;
        return E_ERR_GENERIC;
    }

    cout << "Opening camera... " << flush;
    if (!rec.openCamera(opts.cameraId)) {
        cout << endl;
        cerr << "Error: " << rec.lastError() << endl;
        return E_ERR_OPEN;
    }
    cout << "Done" << endl;

    tPvCameraInfoEx camInfo = rec.cameraInfo();
    cout << "\nCamera infos:"
         << "\n    UniqueId .......... " << camInfo.UniqueId
         << "\n    CameraName ........ " << camInfo.CameraName
         << "\n    ModelName ......... " << camInfo.ModelName
         << "\n    SerialNumber ...... " << camInfo.SerialNumber
         << "\n    FirmwareVersion ... " << camInfo.FirmwareVersion
         << "\n    IP Address ........ " << rec.ipAddress()
         << "\n    Sensor ............ " << rec.sensorWidth() << "x"
                                         << rec.sensorHeight() << "@"
                                         << rec.sensorBits()
         << endl;

    if (!rec.setFrameRate(opts.frameRate) ||
        !rec.setExposureTime(opts.exposureTime) ||
        !rec.setPixelFormat(opts.pixelFormat) ||
        !rec.setTriggerMode(opts.triggerMode) ||
        !rec.setTriggerDelay(opts.triggerDelay) ||
        !rec.setPacketSize(opts.packetSize) ||
        !rec.setBandwidth(opts.bandwidth))
    {
        cerr << "Error: " << rec.lastError() << endl;
        return E_ERR_SETUP;
    }

    cout << "\nSettings:"
         << "\n    FrameRate ......... " << rec.frameRate() << " Hz (max)"
         << "\n    ExposureTime ...... " << rec.exposureTime() << " ms"
         << "\n    PixelFormat ....... " << rec.pixelFormat()
         << "\n    TriggerMode ....... " << rec.triggerMode()
         << "\n    TriggerDelay ...... " << rec.triggerDelay() << " us"
         << "\n    Buffers ........... " << rec.numBuffers()
         << "\n    PacketSize ........ " << rec.packetSize() << " bytes"
         << "\n    Bandwidth ......... " << rec.bandwidth() << " MB/s"
         << endl;

    cout << endl;
    cout << "Recording " << opts.numFrames << " frame"
         << (opts.numFrames != 1 ? "s" : "")
         << " to '" << opts.fname << "':" << endl;

    if (!rec.record(opts.fname, opts.numFrames, opts.force)) {
        cerr << "Error: " << rec.lastError() << endl;
        return E_ERR_RECORD;
    }

    Recorder::IndexVector droppedFrames = rec.droppedFrames();
    if (!droppedFrames.empty()) {
        cout << "\n -> " << droppedFrames.size()
             << " dropped frame(s): ";
        for (Recorder::IndexVector::iterator it = droppedFrames.begin();
                it != droppedFrames.end(); ++it)
            cout << *it << " ";
        cout << endl;
    }

    Recorder::IndexVector missingDataFrames = rec.missingDataFrames();
    if (!missingDataFrames.empty()) {
        cout << "\n -> " << missingDataFrames.size()
             << " frame(s) with missing data: ";
        for (Recorder::IndexVector::iterator it = missingDataFrames.begin();
                it != missingDataFrames.end(); ++it)
            cout << *it << " ";
        cout << endl;
    }

    cout << endl;
    cout << "Closing camera... " << flush;
    rec.closeCamera();
    cout << "Done" << endl;

    return E_OK;
}
