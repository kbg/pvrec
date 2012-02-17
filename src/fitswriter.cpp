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

#include "fitswriter.h"
#include <sstream>
#include <cassert>

FitsWriter::FitsWriter(const std::string &fname, PixelType pixelType,
                       int width, int height, int count, bool clobber)
    : m_pixelType(Uint8),
      m_width(0),
      m_height(0),
      m_count(0),
      m_file(0),
      m_clobber(false)
{
    open(fname, pixelType, width, height, count, clobber);
}

FitsWriter::~FitsWriter()
{
    close();
}

bool FitsWriter::open(const std::string &fname, PixelType pixelType,
                      int width, int height, int count, bool clobber)
{
    clearError();

    if (m_file) {
        setError("File already opened.");
        return false;
    }

    assert(m_fname.empty());
    assert(m_clobber == false);
    assert(m_pixelType == Uint8);
    assert(m_width == 0);
    assert(m_height == 0);
    assert(m_count == 0);

    if (width <= 0 || height <= 0 || count <= 0) {
        setError("Invalid width, height or count.");
        return false;
    }    

    int status = 0;
    std::string fnameClobber = clobber ? ("!" + fname) : fname;
    fits_create_file(&m_file, fnameClobber.c_str(), &status);
    if (status != 0) {
        setError("Cannot create the file '" + fname + "'.", status);
        return false;
    }

    long naxes[3];
    naxes[0] = width;
    naxes[1] = height;
    naxes[2] = count;
    int imageType = (pixelType == Int16) ? SHORT_IMG : BYTE_IMG;
    fits_create_img(m_file, imageType, 3, naxes, &status);
    if (status != 0) {
        setError("Cannot allocate file space.", status);
        close();
        return false;
    }

    // try to remove the 2 default comments entries from the header
    fits_delete_key(m_file, "COMMENT", &status);
    fits_delete_key(m_file, "COMMENT", &status);

    // write time stamp to the header
    fits_write_date(m_file, &status);
    if (status != 0) {
        setError("Cannot write date.", status);
        close();
        return false;
    }

    m_fname = fname;
    m_clobber = clobber;
    m_pixelType = pixelType;
    m_width = width;
    m_height = height;
    m_count = count;

    return true;
}

void FitsWriter::close()
{
    if (!m_file)
        return;

    int status = 0;
    fits_close_file(m_file, &status);

    m_fname.clear();
    m_pixelType = Uint8;
    m_width = 0;
    m_height = 0;
    m_count = 0;
    m_file = 0;
    m_clobber = false;
}

bool FitsWriter::isOpen() const
{
    return m_file != 0;
}

bool FitsWriter::writeFrame(long index, unsigned char *data)
{
    clearError();

    if (!isOpen()) {
        setError("Cannot write frame, file not open.");
        return false;
    }

    if (index < 1 || index > m_count) {
        setError("Frame index out of bounds.");
        return false;
    }

    int status = 0;
    int dataType = (m_pixelType == Int16) ? TSHORT : TBYTE;
    long fpixel[3] = { 1, 1, 0 };
    fpixel[2] += index;
    LONGLONG nelem = m_width * m_height;

    fits_write_pix(m_file, dataType, fpixel, nelem, data, &status);
    if (status != 0) {
        setError("Cannot write frame.", status);
        return false;
    }

    return true;
}

bool FitsWriter::writeKey(int datatype, const char *keyname, void *value,
                          const char *comment)
{
    clearError();

    int status = 0;
    fits_write_key(m_file, datatype, keyname, value, comment, &status);
    if (status != 0) {
        setError("Cannot write header entry.", status);
        return false;
    }

    return true;
}

std::string FitsWriter::lastError() const
{
    return m_errorStr;
}


void FitsWriter::setError(const std::string &msg, int code) const
{
    std::stringstream ss;
    ss << msg;

    if (code != 0) {
        char fitsioMsg[31];  // message has max 30 chars
        fits_get_errstatus(code, fitsioMsg);
        ss << " FITSIO: " << fitsioMsg << ".";
    }

    m_errorStr = ss.str();
}

void FitsWriter::clearError() const
{
    m_errorStr.clear();
}
