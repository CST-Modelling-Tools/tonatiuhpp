#include "PhotonsBuffer.h"
#include "PhotonsAbstract.h"

#include <algorithm>

PhotonsBuffer::PhotonsBuffer(ulong size, ulong sizeReserve):
    m_photonsMax(size),
    m_exporter(0)
{
    if (sizeReserve > 0)
        m_photons.reserve(sizeReserve);
}

bool PhotonsBuffer::addPhotons(const std::vector<Photon>& photons)
{
    if (photons.empty())
        return true;

    if (m_photonsMax == 0) {
        m_photons.insert(m_photons.end(), photons.begin(), photons.end());
        return true;
    }

    ulong nBegin = 0;
    while (nBegin < photons.size()) {
        if (m_photons.size() >= m_photonsMax && !flush()) {
            m_photons.insert(m_photons.end(), photons.begin() + nBegin, photons.end());
            return false;
        }

        ulong space = m_photonsMax - m_photons.size();
        ulong nCopy = std::min<ulong>(space, photons.size() - nBegin);
        m_photons.insert(m_photons.end(), photons.begin() + nBegin, photons.begin() + nBegin + nCopy);
        nBegin += nCopy;
    }

    return true;
}

bool PhotonsBuffer::endExport(double p)
{
    if (!flush())
        return false;

    if (m_exporter)
    {
        m_exporter->setPhotonPower(p);
        return m_exporter->endExport();
    }

    return true;
}

bool PhotonsBuffer::setExporter(PhotonsAbstract* exporter)
{
    if (!exporter) return false;
    m_exporter = exporter;
    return m_exporter->startExport();
}

bool PhotonsBuffer::flush()
{
    if (m_photons.empty())
        return true;

    if (!m_exporter) {
        m_photons.clear();
        return true;
    }

    ulong saved = m_exporter->savePhotons(m_photons);
    if (saved > m_photons.size())
        saved = m_photons.size();
    if (saved > 0)
        m_photons.erase(m_photons.begin(), m_photons.begin() + saved);

    return m_photons.empty();
}
