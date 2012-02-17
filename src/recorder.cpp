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
#include "pvutils.h"
#include "fitswriter.h"
#include "version.h"

#include <cassert>
#include <sstream>
#include <cstring>  // for std::memset()
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
using std::flush;

Recorder::Recorder(int numBuffers)
    : m_device(0),
      m_sensorBits(0),
      m_sensorWidth(0),
      m_sensorHeight(0),
      m_numBuffers(numBuffers),
      m_frameBufferSize(0)
{
    PvInitialize();
}

Recorder::~Recorder()
{
    closeCamera();
    PvUnInitialize();
}

bool Recorder::openCamera()
{
    clearError();

    if (isCameraOpen())
        closeCamera();

    assert(m_device == 0);
    
    // try to find a camera for at most 3000ms
    unsigned long camCount = 0;
    for (int i = 0; i < 30 && camCount < 1; ++i) {
        camCount = PvCameraCount();
        msleep(100);
    }

    if (camCount < 1) {
        setError("No camera found.");
        return false;
    }

    tPvCameraInfoEx *camInfos = new tPvCameraInfoEx[camCount];
    unsigned long n = PvCameraListEx(camInfos, camCount, 0,
                                     sizeof(tPvCameraInfoEx));

    // try to open the first camera with master access
    tPvErr err = ePvErrSuccess;
    for (unsigned long i = 0; i < n && !m_device; ++i)
    {
        tPvCameraInfoEx *info = camInfos + i;
        if ((info->PermittedAccess & ePvAccessMaster))
        {
            err = PvCameraOpen(info->UniqueId, ePvAccessMaster, &m_device);
            if (err != ePvErrSuccess)
                m_device = 0;
            else
                m_camInfo = *info;
        }
    }

    delete [] camInfos;

    if (!m_device) {
        clearCameraInfo();
        if (err != ePvErrSuccess)
            setPvError("Cannot open camera.", err);
        else
            setError("Cannot open camera.");
        return false;
    }

    char sensorType[32];
    err = PvAttrEnumGet(m_device, "SensorType", sensorType, 32, 0);
    if (err != ePvErrSuccess) {
        setPvError("Cannot get sensor type.", err);
        closeCamera();
        return false;
    }

    if (std::string(sensorType) != "Mono") {
        std::stringstream ss;
        ss << "Sensor type '" << sensorType << "' is not supported.";
        setError(ss.str());
        closeCamera();
        return false;
    }

    tPvUint32 sensorBits;
    err = PvAttrUint32Get(m_device, "SensorBits", &sensorBits);
    if (err != ePvErrSuccess) {
        setPvError("Cannot get sensor bit depth.", err);
        closeCamera();
        return false;
    }

    tPvUint32 sensorWidth;
    err = PvAttrUint32Get(m_device, "SensorWidth", &sensorWidth);
    if (err != ePvErrSuccess) {
        setPvError("Cannot get sensor width.", err);
        closeCamera();
        return false;
    }

    tPvUint32 sensorHeight;
    err = PvAttrUint32Get(m_device, "SensorHeight", &sensorHeight);
    if (err != ePvErrSuccess) {
        setPvError("Cannot get sensor height.", err);
        closeCamera();
        return false;
    }

    char ipAddress[32];
    err = PvAttrStringGet(m_device, "DeviceIPAddress", ipAddress, 32, 0);
    if (err != ePvErrSuccess) {
        setPvError("Cannot get IP address.", err);
        closeCamera();
        return false;
    }

    char ethAddress[32];
    err = PvAttrStringGet(m_device, "DeviceEthAddress", ethAddress, 32, 0);
    if (err != ePvErrSuccess) {
        setPvError("Cannot get MAC address.", err);
        closeCamera();
        return false;
    }

    m_sensorBits = int(sensorBits);
    m_sensorWidth = int(sensorWidth);
    m_sensorHeight = int(sensorHeight);
    m_ipAddress = std::string(ipAddress);
    m_ethAddress = std::string(ethAddress);

    if (!initCamera())
        return false;

    return true;
}

bool Recorder::initCamera()
{
    tPvErr err;

    err = PvAttrEnumSet(m_device, "ConfigFileIndex", "Factory");
    if (err != ePvErrSuccess) {
        setPvError("Cannot select factory settings.", err);
        return false;
    }

    err = PvCommandRun(m_device, "ConfigFileLoad");
    if (err != ePvErrSuccess) {
        setPvError("Cannot load factory settings.", err);
        return false;
    }

    err = PvAttrEnumSet(m_device, "AcquisitionMode", "Continuous");
    if (err != ePvErrSuccess) {
        setPvError("Cannot set AcquisitionMode to Continuous.", err);
        return false;
    }

    err = PvAttrEnumSet(m_device, "FrameStartTriggerMode", "FixedRate");
    if (err != ePvErrSuccess) {
        setPvError("Cannot set FrameStartTriggerMode.", err);
        return false;
    }

    if (!setPixelFormat("Mono8"))
        return false;

    if (!setPacketSize(0))
        return false;

    return true;
}

void Recorder::allocateFrames(int numBuffers, size_t bufferSize)
{
    if(!m_frameQueue.empty())
        freeFrames();

    for (int i = 0; i < numBuffers; ++i)
    {
        unsigned char *buffer = new unsigned char[bufferSize];
        tPvFrame *frame = new tPvFrame;
        std::memset(frame, 0, sizeof(tPvFrame));
        frame->ImageBuffer = buffer;
        frame->ImageBufferSize = bufferSize;
        m_frameQueue.push_back(frame);
    }
}

void Recorder::freeFrames()
{
    while (!m_frameQueue.empty()) {
        tPvFrame *frame = m_frameQueue.front();
        delete [] reinterpret_cast<unsigned char *>(frame->ImageBuffer);
        delete frame;
        m_frameQueue.pop_front();
    }
}

void Recorder::closeCamera()
{
    PvCameraClose(m_device);
    m_device = 0;
    m_sensorBits = 0;
    m_frameBufferSize = 0;
    m_droppedFrames.clear();
    m_missingDataFrames.clear();
    freeFrames();
}

bool Recorder::isCameraOpen() const
{
    return m_device != 0;
}

bool Recorder::record(const std::string &fname, int numFrames, bool clobber)
{
    clearError();

    if (!m_device) {
        setError("Cannot start recording, camera device not opened.");
        return false;
    }

    tPvErr err;
    err = PvCaptureStart(m_device);
    if (err != ePvErrSuccess) {
        setPvError("Cannot start capturing.", err);
        return false;
    }

    int width = m_sensorWidth;
    int height = m_sensorHeight;
    int bytesPerPixel = 1;
    FitsWriter::PixelType pixelType = FitsWriter::Uint8;
    std::string format = pixelFormat();
    if (format == "Mono16") {
        bytesPerPixel = 2;
        pixelType = FitsWriter::Int16;
    }
    else if (format != "Mono8") {
        setError("Unsupported pixel format.");
        PvCaptureEnd(m_device);
        return false;
    }

    m_frameBufferSize = size_t(bytesPerPixel * width * height);
    allocateFrames(m_numBuffers, m_frameBufferSize);

    for (FrameQueue::iterator it = m_frameQueue.begin();
            it != m_frameQueue.end(); ++it)
    {
        err = PvCaptureQueueFrame(m_device, *it, 0);
        if (err != ePvErrSuccess) {
            setPvError("Cannot enqueue frame.", err);
            PvCaptureQueueClear(m_device);
            PvCaptureEnd(m_device);
            return false;
        }
    }

    // create output file
    FitsWriter writer(fname, pixelType, width, height, numFrames, clobber);
    if (!writer.isOpen()) {
        setError(writer.lastError());
        PvCaptureQueueClear(m_device);
        PvCaptureEnd(m_device);
        return false;
    }

#ifdef APP_VERSION
    // write program version to the FITS header
    std::string creator = std::string("PvRec v") + PVREC_VERSION_STRING;
    if (!writer.writeKey(TSTRING, "CREATOR",
                         const_cast<char*>(creator.c_str()),
                         "program that created this file"))
    {
        setError(writer.lastError());
        PvCaptureQueueClear(m_device);
        PvCaptureEnd(m_device);
        return false;
    }
#endif

    // write settings to the FITS header
    double expTime = exposureTime();
    float maxFps = frameRate();
    if (!writer.writeKey(
            TDOUBLE, "EXPTIME", &expTime, "exposure time [ms]") ||
        !writer.writeKey(
            TFLOAT, "MAXFPS", &maxFps, "maximum frame rate [Hz]"))
    {
        setError(writer.lastError());
        PvCaptureQueueClear(m_device);
        PvCaptureEnd(m_device);
        return false;
    }

    err = PvCommandRun(m_device, "AcquisitionStart");
    if (err != ePvErrSuccess) {
        setPvError("Cannot start acquisition.", err);
        PvCaptureQueueClear(m_device);
        PvCaptureEnd(m_device);
        return false;
    }

    // the capture loop
    m_droppedFrames.clear();
    m_missingDataFrames.clear();
    for (unsigned long i = 1; i <= numFrames; ++i)
    {
        tPvFrame *frame = m_frameQueue.front();
        m_frameQueue.pop_front();
        m_frameQueue.push_back(frame);

        err = PvCaptureWaitForFrameDone(m_device, frame, PVINFINITE);
        if (err != ePvErrSuccess) {
            setPvError("Waiting for frame failed.", err);
            PvCaptureQueueClear(m_device);
            PvCaptureEnd(m_device);
            return false;
        }

        if (frame->Status == ePvErrSuccess ||
            frame->Status == ePvErrDataMissing)
        {
            if (frame->FrameCount > i) {
                while (i < frame->FrameCount) {
                    cout << "D";
                    m_droppedFrames.push_back(i);
                    i++;
                }
            }
            else if (frame->FrameCount < i) // this should not occur
                cout << "E" << flush;

            if (i <= numFrames)
            {
                if (frame->Status == ePvErrSuccess)
                    cout << "." << flush;
                else if (frame->Status == ePvErrDataMissing) {
                    cout << "M" << flush;
                    m_missingDataFrames.push_back(i);
                }

                if (!writer.writeFrame(i, reinterpret_cast<unsigned char *>(
                        frame->ImageBuffer)))
                    cerr << endl << writer.lastError() << endl;
            }

            //cout << " " << frame->FrameCount << " " << flush;
        }
        else {
            cout << endl;
            cout << PvErrorMessage(frame->Status) << " ["
                 << PvErrorCodeStr(frame->Status) << "]" << endl;
        }

        err = PvCaptureQueueFrame(m_device, frame, 0);
        if (err != ePvErrSuccess) {
            setPvError("Cannot reenqueue frame.", err);
            PvCaptureQueueClear(m_device);
            PvCaptureEnd(m_device);
            return false;
        }
    }
    cout << endl;

    err = PvCommandRun(m_device, "AcquisitionStop");
    if (err != ePvErrSuccess) {
        setPvError("Cannot stop acquisition.", err);
        return false;
    }

    err = PvCaptureQueueClear(m_device);
    if (err != ePvErrSuccess) {
        setPvError("Cannot clear capture queue.", err);
        PvCaptureEnd(m_device);
        return false;
    }

    // write number of buggy frames to the FITS header
    unsigned long numDrop = m_droppedFrames.size();
    unsigned long numMiss = m_missingDataFrames.size();
    writer.writeKey(TULONG, "NDROP", &numDrop, "number of dropped frames");
    writer.writeKey(TULONG, "NMISS", &numMiss,
                    "number of frames with missing data");

    err = PvCaptureEnd(m_device);
    if (err != ePvErrSuccess) {
        setPvError("Cannot stop capturing.", err);
        return false;
    }

    return true;
}

Recorder::IndexVector Recorder::droppedFrames() const
{
    return m_droppedFrames;
}

Recorder::IndexVector Recorder::missingDataFrames() const
{
    return m_missingDataFrames;
}

bool Recorder::setFrameRate(float frameRate)
{
    tPvErr err = PvAttrFloat32Set(m_device, "FrameRate", frameRate);
    if (err != ePvErrSuccess) {
        setPvError("Cannot set frame rate.", err);
        return false;
    }
    return true;
}

float Recorder::frameRate() const
{
    tPvFloat32 value;
    tPvErr err = PvAttrFloat32Get(m_device, "FrameRate", &value);
    return (err == ePvErrSuccess) ? value : 0.0;
}

bool Recorder::setExposureTime(double exposureTime)
{
    unsigned int value = static_cast<unsigned int>(1e3 * exposureTime + 0.5);
    tPvErr err = PvAttrUint32Set(m_device, "ExposureValue", value);
    if (err != ePvErrSuccess) {
        setPvError("Cannot set exposure time.", err);
        return false;
    }
    return true;
}

double Recorder::exposureTime() const
{
    tPvUint32 value;
    tPvErr err = PvAttrUint32Get(m_device, "ExposureValue", &value);
    return (err == ePvErrSuccess) ? double(value) / 1e3 : 0;
}

bool Recorder::setPixelFormat(const std::string &pixelFormat)
{
    tPvErr err = PvAttrEnumSet(m_device, "PixelFormat", pixelFormat.c_str());
    if (err != ePvErrSuccess) {
        setPvError("Cannor set pixel format.", err);
        return false;
    }
    return true;
}

std::string Recorder::pixelFormat() const
{
    char value[32];
    tPvErr err = PvAttrEnumGet(m_device, "PixelFormat", value, 32, 0);
    return (err == ePvErrSuccess) ? value : "";
}

bool Recorder::setPacketSize(unsigned int packetSize)
{
    tPvErr err;

    if (packetSize != 0)
    {
        err = PvAttrUint32Set(m_device, "PacketSize", packetSize);
        if (err != ePvErrSuccess) {
            setPvError("Cannot set packet size.", err);
            return false;
        }
    } else {
        err = PvCaptureAdjustPacketSize(m_device, 8228);
        if (err != ePvErrSuccess) {
            setPvError("Cannot adjust packet size.", err);
            return false;
        }
    }
    return true;
}

unsigned int Recorder::packetSize() const
{
    tPvUint32 value;
    tPvErr err = PvAttrUint32Get(m_device, "PacketSize", &value);
    return (err == ePvErrSuccess) ? value : 0;
}

bool Recorder::setBandwidth(double bandwidth)
{
    unsigned int value = static_cast<unsigned int>(1e6 * bandwidth + 0.5);
    tPvErr err = PvAttrUint32Set(m_device, "StreamBytesPerSecond", value);
    if (err != ePvErrSuccess) {
        setPvError("Cannot set bandwidth.", err);
        return false;
    }
    return true;
}

double Recorder::bandwidth() const
{
    tPvUint32 value;
    tPvErr err = PvAttrUint32Get(m_device, "StreamBytesPerSecond", &value);
    return (err == ePvErrSuccess) ? double(value) / 1e6 : 0.0;
}

std::string Recorder::cameraInfoStr() const
{
    if (!isCameraOpen())
        return "";

    std::stringstream ss;
    ss << "        UniqueId: " << m_camInfo.UniqueId << "\n"
       << "      CameraName: " << m_camInfo.CameraName << "\n"
       << "       ModelName: " << m_camInfo.ModelName << "\n"
       << "    SerialNumber: " << m_camInfo.SerialNumber << "\n"
       << " FirmwareVersion: " << m_camInfo.FirmwareVersion << "\n"
       << "      IP Address: " << m_ipAddress << "\n"
       << "          Sensor: " << m_sensorWidth << "x" << m_sensorHeight
                                 << "@" << m_sensorBits;
    return ss.str();
}

std::string Recorder::cameraSettingsStr() const
{
    if (!isCameraOpen())
        return "";

    std::stringstream ss;
    ss << "       FrameRate: " << frameRate() << " Hz (max)\n"
       << "    ExposureTime: " << exposureTime() << " ms\n"
       << "     PixelFormat: " << pixelFormat() << "\n"
       << "         Buffers: " << m_numBuffers << "\n"
       << "      PacketSize: " << packetSize() << " bytes\n"
       << "       Bandwidth: " << bandwidth() << " MB/s";
    return ss.str();
}

std::string Recorder::apiVersionStr() const
{
    unsigned long major, minor;
    PvVersion(&major, &minor);

    std::stringstream ss;
    ss << major << "." << minor;
    return ss.str();
}

std::string Recorder::lastError() const
{
    return m_errorStr;
}

void Recorder::setError(const std::string &msg) const
{
    m_errorStr = msg;
}

void Recorder::setPvError(const std::string &msg, tPvErr code) const
{
    std::stringstream ss;
    ss << msg << " PvApi: " << PvErrorMessage(code)
              << ". [" << PvErrorCodeStr(code) << "]";
    m_errorStr = ss.str();
}

void Recorder::clearError() const
{
    m_errorStr.clear();
}

void Recorder::clearCameraInfo()
{
    std::memset(&m_camInfo, 0, sizeof(m_camInfo));
}
