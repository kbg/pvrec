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

#ifndef PVREC_RECORDER_H
#define PVREC_RECORDER_H

#include <string>
#include <vector>
#include <deque>
#include <fitsio.h>
#include <PvApi.h>

class Recorder
{
public:
    explicit Recorder(int numBuffers = 3);
    virtual ~Recorder();

    bool openCamera(unsigned long camId = 0);
    void closeCamera();
    bool isCameraOpen() const;

    bool record(const std::string &fname, int numFrames, bool clobber = false);

    typedef std::vector<unsigned long> IndexVector;
    IndexVector droppedFrames() const;
    IndexVector missingDataFrames() const;

    bool setFrameRate(float frameRate);
    float frameRate() const;

    bool setExposureTime(double exposureTime);
    double exposureTime() const;

    bool setPixelFormat(const std::string &pixelFormat);
    std::string pixelFormat() const;

    bool setPacketSize(unsigned int packetSize);
    unsigned int packetSize() const;

    bool setBandwidth(double bandwidth);
    double bandwidth() const;

    int sensorWidth() const;
    int sensorHeight() const;
    int sensorBits() const;
    size_t numBuffers() const;
    std::string ipAddress() const;
    tPvCameraInfoEx cameraInfo() const;

    typedef std::vector<tPvCameraInfoEx> CameraInfoVector;
    CameraInfoVector availableCameras(int timeout = 3000) const;

    std::string apiVersionStr() const;
    std::string lastError() const;

protected:
    bool initCamera();
    void allocateFrames(int numBuffers, size_t bufferSize);
    void freeFrames();
    void setError(const std::string &msg) const;
    void setPvError(const std::string &msg, tPvErr code) const;
    void clearError() const;
    void clearCameraInfo();

private:
    mutable std::string m_errorStr;
    tPvHandle m_device;
    tPvCameraInfoEx m_camInfo;
    std::string m_ipAddress;
    std::string m_ethAddress;
    int m_sensorBits;
    int m_sensorWidth;
    int m_sensorHeight;
    int m_numBuffers;
    size_t m_frameBufferSize;
    typedef std::deque<tPvFrame *> FrameQueue;
    FrameQueue m_frameQueue;
    IndexVector m_droppedFrames;
    IndexVector m_missingDataFrames;
};

#endif // PVREC_RECORDER_H
