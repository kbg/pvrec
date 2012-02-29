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

#include "cmdopts.h"
#include <getopt.h>
#include <sstream>
#include <iostream>
using std::cerr;
using std::endl;

static const int DefaultNumFrames = 1;
static const float DefaultFrameRate = 20.0;
static const double DefaultExposureTime = 15.0;
static const int DefaultPixelBits = 8;
static const int DefaultCameraId = 0;
static const std::string DefaultPixelFormat = "Mono8";
static const int DefaultNumBuffers = 10;
static const unsigned int DefaultPacketSize = 0;
static const double DefaultBandwidth = 115.0;

template <class T>
bool fromString(T &value, const std::string &str) {
    std::istringstream ss(str);
    ss >> value;
    return !ss.fail();
}

CmdLineOptions::CmdLineOptions(int argc, char **argv)
    : m_argc(argc),
      m_argv(argv),
      numFrames(DefaultNumFrames),
      frameRate(DefaultFrameRate),
      exposureTime(DefaultExposureTime),
      pixelFormat(DefaultPixelFormat),
      cameraId(DefaultCameraId),
      packetSize(DefaultPacketSize),
      bandwidth(DefaultBandwidth),
      numBuffers(DefaultNumBuffers),
      force(false),
      list(false),
      info(false)
{
    if (argc > 0)
        m_appName = argv[0];
}

CmdLineOptions::Result CmdLineOptions::parse()
{
    static const char *short_opts = "n:r:e:b:c:N:m:B:fliVh";
    static const struct option long_opts[] = {
        { "count", required_argument, 0, 'n' },
        { "framerate", required_argument, 0, 'r' },
        { "exposure", required_argument, 0, 'e' },
        { "bits", required_argument, 0, 'b' },
        { "camera", required_argument, 0, 'c' },
        { "buffers", required_argument, 0, 'N' },
        { "mtu", required_argument, 0, 'm' },
        { "bandwidth", required_argument, 0, 'B' },
        { "force", no_argument, 0, 'f' },
        { "list", no_argument, 0, 'l' },
        { "info", no_argument, 0, 'i' },
        { "version", no_argument, 0, 'V' },
        { "help", no_argument, 0, 'h' },
        { 0, 0, 0, 0 }
    };

    while (true)
    {
        int idx = 0;
        int c = getopt_long(m_argc, m_argv, short_opts, long_opts, &idx);
        if (c == -1)
            break;

        switch (c)
        {
        case 'n':
            if (!fromString(numFrames, optarg)) {
                cerr << m_appName << ": -n must be an integer." << endl;
                return Error;
            }
            if (numFrames <= 0) {
                cerr << m_appName << ": -n must be greater than 0." << endl;
                return Error;
            }
            break;
        case 'r':
            if (!fromString(frameRate, optarg)) {
                cerr << m_appName << ": -r must be a number." << endl;
                return Error;
            }
            if (frameRate <= 0) {
                cerr << m_appName << ": -r must be greater than 0." << endl;
                return Error;
            }
            break;
        case 'e':
            if (!fromString(exposureTime, optarg)) {
                cerr << m_appName << ": -e must be a number." << endl;
                return Error;
            }
            if (frameRate <= 0) {
                cerr << m_appName << ": -e must be greater than 0." << endl;
                return Error;
            }
            break;
        case 'b':
            int pixelBits;
            if (!fromString(pixelBits, optarg)) {
                cerr << m_appName << ": -b must be an integer." << endl;
                return Error;
            }
            if (pixelBits == 8)
                pixelFormat = "Mono8";
            else if (pixelBits == 16)
                pixelFormat = "Mono16";
            else {
                cerr << m_appName << ": -b must be 8 or 16" << endl;
                return Error;
            }
            break;
        case 'c':
            if (!fromString(cameraId, optarg)) {
                cerr << m_appName << ": -c must be an unsigned integer."
                     << endl;
                return Error;
            }
            break;
        case 'N':
            if (!fromString(numBuffers, optarg)) {
                cerr << m_appName << ": -N must be an integer." << endl;
                return Error;
            }
            if (numBuffers < 1) {
                cerr << m_appName << ": -N must be at least 1." << endl;
                return Error;
            }
            break;
        case 'm':
            if (!fromString(packetSize, optarg)) {
                cerr << m_appName << ": -m must be an unsigned integer."
                     << endl;
                return Error;
            }
            break;
        case 'B':
            if (!fromString(bandwidth, optarg)) {
                cerr << m_appName << ": -B must be a number." << endl;
                return Error;
            }
            if (bandwidth <= 0) {
                cerr << m_appName << ": -B must be greater than 0." << endl;
                return Error;
            }
            break;
        case 'f':
            force = true;
            break;
        case 'l':
            list = true;
            break;
        case 'i':
            info = true;
            break;
        case 'V':
            return Version;
        case 'h':
            return Help;
        case '?':
        default:
            return Error;
        }
    }

    // no filename needed for --list or --info
    if (list || info)
        return Ok;

    if (optind >= m_argc) {
        cerr << m_appName << ": no filename specified." << endl;
        return Error;
    }

    if (optind + 1 != m_argc) {
        cerr << m_appName << ": too many arguments." << endl;
        return Error;
    }

    fname = m_argv[optind];

    return Ok;
}

std::string CmdLineOptions::usage() const
{
    std::stringstream ss;
    ss << "Usage: " << m_appName << " [options] filename";
    return ss.str();
}

std::string CmdLineOptions::help() const
{
    std::stringstream ss;
    ss << usage() << "\n\n"
       << "Options:\n"
       << "  -n, --count       Number of frames to record (default: " << DefaultNumFrames << ")\n"
       << "  -r, --framerate   Maximum frame rate in Hz (default: " << DefaultFrameRate << ")\n"
       << "  -e, --exposure    Exposure time in ms (default: " << DefaultExposureTime << ")\n"
       << "  -b, --bits        Bits per pixel, 8 or 16 (default: " << DefaultPixelBits << ")\n"
       << "  -c, --camera      Select camera by its unique ID (default: auto)\n"
       << "  -N, --buffers     Number of frame buffers (default: " << DefaultNumBuffers << ")\n"
       << "  -m, --mtu         Packet size (default: auto)\n"
       << "  -B, --bandwidth   Stream bandwidth in MB/s (default: " << DefaultBandwidth << ")\n"
       << "  -f, --force       Overwrite the output file if it already exists\n"
       << "  -l, --list        List available cameras and quit\n"
       << "  -i, --info        Show informations on the available cameras and quit\n"
       << "  -V, --version     Show program version and quit\n"
       << "  -h, --help        Show this help message and quit";
    return ss.str();
}

std::string CmdLineOptions::help_hint() const
{
    std::stringstream ss;
    ss << m_appName << ": `--help' gives usage information.";
    return ss.str();
}
