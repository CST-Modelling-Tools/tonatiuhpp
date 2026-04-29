#pragma once

#include "Photon.h"

class PhotonsAbstract;


class TONATIUH_KERNEL PhotonsBuffer
{
public:
    PhotonsBuffer(ulong size, ulong sizeReserve = 0);

    bool addPhotons(const std::vector<Photon>& photons);
    const std::vector<Photon>& getPhotons() const {return m_photons;} // for flux and screen
    bool hasRetainedPhotons() const {return !m_photons.empty();}
    bool hasExportFailed() const {return m_exportFailed;}
    bool endExport(double p);

    bool setExporter(PhotonsAbstract* exporter);
    PhotonsAbstract* getExporter() const {return m_exporter;}

private:
    bool flush();

    std::vector<Photon> m_photons; // buffer, std is faster than QVector
    ulong m_photonsMax;

    PhotonsAbstract* m_exporter;
    bool m_exportFailed;
};
