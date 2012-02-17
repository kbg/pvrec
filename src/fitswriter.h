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

#ifndef FITSWRITER_H
#define FITSWRITER_H

#include <string>
#include <fitsio.h>

class FitsWriter
{
public:
    enum PixelType { Uint8, Int16 };

    FitsWriter(const std::string &fname, PixelType pixelType,
               int width, int height, int count, bool clobber = false);
    virtual ~FitsWriter();

    bool open(const std::string &fname, PixelType pixelType,
              int width, int height, int count, bool clobber = false);
    void close();
    bool isOpen() const;

    bool writeFrame(long index, unsigned char *data);
    bool writeKey(int datatype, const char *keyname, void *value,
                  const char *comment);

    std::string lastError() const;

protected:
    void setError(const std::string &msg, int code = 0) const;
    void clearError() const;

private:
    mutable std::string m_errorStr;
    std::string m_fname;
    PixelType m_pixelType;
    int m_width;
    int m_height;
    int m_count;
    fitsfile *m_file;
    bool m_clobber;
};

#endif // FITSWRITER_H
