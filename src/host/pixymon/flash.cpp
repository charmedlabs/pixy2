//
// begin license header
//
// This file is part of Pixy CMUcam5 or "Pixy" for short
//
// All Pixy source code is provided under the terms of the
// GNU General Public License v2 (http://www.gnu.org/licenses/gpl-2.0.html).
// Those wishing to use Pixy source code, software and/or
// technologies under different licensing terms should contact us at
// cmucam@cs.cmu.edu. Such licensing terms are available for
// all portions of the Pixy codebase presented here.
//
// end license header
//

#include <QFile>
#include <QStringList>
#include <stdexcept>
#include "flash.h"
#include "reader.h"


Flash::Flash() : m_chirp(false, true) // not hinterested, client
{
    ChirpProc sectorSizeProc;
    uint32_t len;
    int32_t res;
    uint16_t *version;

    if (m_link.open()<0)
        throw std::runtime_error("Cannot initialize USB for flash programming.");
    m_chirp.setLink(&m_link);

    sectorSizeProc = m_chirp.getProc("flash_sectorSize");
    m_programProc = m_chirp.getProc("flash_program");
    m_reset = m_chirp.getProc("flash_reset");
    m_getHardwareVersion = m_chirp.getProc("getHardwareVersion");

    if (sectorSizeProc<0 || m_programProc<0 || m_reset<0 || m_getHardwareVersion<0)
        throw std::runtime_error("Cannot get flash procedures.");

    if (m_chirp.callSync(sectorSizeProc, END_OUT_ARGS,
                     &m_sectorSize, END_IN_ARGS)<0)
        throw std::runtime_error("Cannot get flash sector size.");

    if (m_chirp.callSync(m_getHardwareVersion, END_OUT_ARGS,
                     &res, &len, &version, END_IN_ARGS)<0 || res<0)
        throw std::runtime_error("Cannot get hardware version.");

    memcpy(m_hardwareVersion, version, 3*sizeof(uint16_t));

    m_buf = new char[m_sectorSize];
}

Flash::~Flash()
{
    delete[] m_buf;
}

void Flash::program(const QString &filename)
{
#if 0
    QFile file(filename);
    uint32_t len;
    uint32_t addr;
    int32_t response;

    if (!file.open(QIODevice::ReadOnly))
        throw std::runtime_error((QString("Cannot open file ") + filename + QString(".")).toStdString());

    for(addr=0x14000000; !file.atEnd(); addr+=len)
    {
        len =(uint32_t)file.read(m_buf, m_sectorSize);
        m_chirp.callSync(m_programProc, UINT32(addr), UINTS8(len, m_buf), END_OUT_ARGS,
                         &response, END_IN_ARGS);
        if (response==-1)
            throw std::runtime_error("Invalid address range.");
        else if (response==-3)
            throw std::runtime_error("Error during verify.");
        else if (response<0)
            throw std::runtime_error("Error during programming.");
    }

#else
    IReader *reader;
    unsigned long addr, len;
    int32_t res, response;

    checkHardwareVersion(m_hardwareVersion, filename);

    reader = createReader(filename);
    while(1)
    {
        res = reader->read((unsigned char *)m_buf, m_sectorSize, &addr, &len);
        if (len)
        {
            if (m_chirp.callSync(m_programProc, UINT32(addr), UINTS8(len, m_buf), END_OUT_ARGS,
                             &response, END_IN_ARGS)<0)
                throw std::runtime_error("communication error during programming.");
            if (response==-1)
                throw std::runtime_error("invalid address range.");
            else if (response==-3)
                throw std::runtime_error("during verify.");
            else if (response<-100)
            {
                QString str = "I/O: " + QString::number(-response-100) + ".";
                throw std::runtime_error(str.toStdString());
            }
            else if (response<0)
            {
                QString str = "programming: " + QString::number(response) + ".";
                throw std::runtime_error(str.toStdString());
            }
        }
        if (res<0)
            break;
    }
#endif
    // reset Pixy
    if (m_chirp.callSync(m_reset, END_OUT_ARGS,
                         &response, END_IN_ARGS)<0)
        throw std::runtime_error("Unable to reset.");
    destroyReader(reader);
}

void Flash::checkHardwareVersion(const uint16_t hardwareVersion[], const QString &filename, ushort firmwareVersion[], QString *firmwareType)
{
    QFile file;
    QStringList words;
    int i, op;
    uint num;
    bool ok = true;

    file.setFileName(filename);
    if (!file.open(QIODevice::ReadOnly))
        throw std::runtime_error((QString("Cannot open file ") + filename + QString(".")).toStdString());
    QString line = file.readLine();
    words = line.split(':');
    if (words.size()<2)
        throw std::runtime_error("Parse error when reading firmware file.");
    words = words[0].split(',');
    if (words.size()<10)
        throw std::runtime_error("Firmware header is not present or is corrupt.");

    if (firmwareType)
        *firmwareType = words[0];
    if (firmwareVersion)
    {
        firmwareVersion[0] = words[1].toInt(&ok);
        if (!ok)
            throw std::runtime_error("Parse error when reading firmware header values.");
        firmwareVersion[1] = words[2].toInt(&ok);
        if (!ok)
            throw std::runtime_error("Parse error when reading firmware header values.");
        firmwareVersion[2] = words[3].toInt(&ok);
        if (!ok)
            throw std::runtime_error("Parse error when reading firmware header values.");
    }

    for (i=0; i<3; i++)
    {
        op = words[i*2+4].toInt(&ok);
        if (!ok)
            throw std::runtime_error("Parse error when reading firmware header values.");
        num = words[i*2+5].toInt(&ok);
        if (!ok)
            throw std::runtime_error("Parse error when reading firmware header values.");

        if (op==0)
        {
            if (num<hardwareVersion[i])
                throw std::runtime_error("This firmware is incompatible with your Pixy hardware version (your Pixy is too advanced).");
        }
        else if (op==1)
        {
            if (num!=hardwareVersion[i])
                throw std::runtime_error("This firmware is incompatible with your Pixy hardware version.");
        }
        else
            throw std::runtime_error("Unsupported firmware header value.");
    }

    file.close();
}

