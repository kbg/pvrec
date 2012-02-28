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

#ifndef CMDOPTS_H
#define CMDOPTS_H

#include <string>

class CmdLineOptions
{
public:
    enum Result { Ok, Error, Help, Version };

    CmdLineOptions(int argc, char **argv);

    Result parse();
    std::string usage() const;
    std::string help() const;
    std::string help_hint() const;

public:
    std::string fname;
    int numFrames;
    float frameRate;
    double exposureTime;
    std::string pixelFormat;
    unsigned int cameraId;
    unsigned int packetSize;
    double bandwidth;
    int numBuffers;
    bool force;

private:
    int m_argc;
    char **m_argv;
    std::string m_appName;
};

#endif // CMDOPTS_H
