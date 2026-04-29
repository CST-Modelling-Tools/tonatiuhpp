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

void PhotonsBuffer::addPhotons(const std::vector<Photon>& photons)
{
    if (photons.empty())
        return;

    if (m_photonsMax == 0) {
        m_photons.insert(m_photons.end(), photons.begin(), photons.end());
        return;
    }

    ulong nBegin = 0;
    while (nBegin < photons.size()) {
        if (m_photons.size() >= m_photonsMax)
            flush();

        ulong space = m_photonsMax - m_photons.size();
        ulong nCopy = std::min<ulong>(space, photons.size() - nBegin);
        m_photons.insert(m_photons.end(), photons.begin() + nBegin, photons.begin() + nBegin + nCopy);
        nBegin += nCopy;
    }
}

void PhotonsBuffer::endExport(double p)
{
    flush();
    if (m_exporter)
    {
        m_exporter->setPhotonPower(p);
        m_exporter->endExport();
    }
}

bool PhotonsBuffer::setExporter(PhotonsAbstract* exporter)
{
    if (!exporter) return false;
    m_exporter = exporter;
    return m_exporter->startExport();
}

void PhotonsBuffer::flush()
{
    if (m_photons.empty())
        return;

    if (m_exporter)
        m_exporter->savePhotons(m_photons);
    m_photons.clear();
}
