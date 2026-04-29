#include "PhotonsFile.h"

#include <algorithm>
#include <iostream>

#include <QDataStream>
#include <QDebug>
#include <QTextStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>

#include "kernel/run/InstanceNode.h"


PhotonsFile::PhotonsFile():
    PhotonsAbstract(),
    m_dirName(""),
    m_fileName("PhotonMap"),
    m_oneFile(true),
    m_nPhotonsPerFile(-1),
    m_fileCurrent(1),
    m_exportedPhotons(0),
    m_photonPower(0.)
{

}

void PhotonsFile::setParameter(QString name, QString value)
{
    QStringList parameters = getParameterNames();

    if (name == parameters[0])
        m_dirName = value;
    else if (name == parameters[1])
        m_fileName = value;
    else if (name == parameters[2])
    {
        m_oneFile = value.toDouble() < 0;
        m_nPhotonsPerFile = value.toULong();
    }
}

bool PhotonsFile::startExport()
{
    if (m_exportedPhotons > 0) return true;

    // remove existing files
    while (true) {
        try {
            QDir dir(m_dirName);
            QFileInfoList infoList;
            if (m_oneFile) {
                QString fileName = dir.absoluteFilePath(m_fileName + ".dat");
                infoList << QFileInfo(fileName);
            } else {
                QStringList filters(m_fileName + "_*.dat");
                infoList = dir.entryInfoList(filters);
            }

            for (QFileInfo& fileInfo : infoList) {
                QFile file(fileInfo.absoluteFilePath());
                if (file.exists() && !file.remove())
                    throw QString("Error deleting %1.\nThe file is in use. Please, close it before continuing.")
                        .arg(fileInfo.absoluteFilePath());
            }
            return true;
        }
        catch (QString message) {
            QMessageBox::warning(0, "Tonatiuh", message);
        }
    }
}

void PhotonsFile::savePhotons(const std::vector<Photon>& photons)
{
    if (photons.empty())
        return;

    QDir dir(m_dirName);
    if (m_oneFile || m_nPhotonsPerFile == 0)
	{
        QString fileName = dir.absoluteFilePath(m_fileName + ".dat");
        writePhotons(fileName, photons, 0, photons.size());
	}
	else
    {
        ulong nBegin = 0;
        while (nBegin < photons.size()) {
            m_fileCurrent = int(m_exportedPhotons/m_nPhotonsPerFile) + 1;
            ulong nFile = m_exportedPhotons % m_nPhotonsPerFile;
            ulong nEnd = nBegin + std::min<ulong>(m_nPhotonsPerFile - nFile, photons.size() - nBegin);
            QString fileName = QString("%1_%2.dat").arg(m_fileName, QString::number(m_fileCurrent));
            fileName = dir.absoluteFilePath(fileName);
            writePhotons(fileName, photons, nBegin, nEnd);
            nBegin = nEnd;
        }
    }
}

void PhotonsFile::endExport()
{
    QDir dir(m_dirName);
    QString fileName = dir.absoluteFilePath(m_fileName + "_parameters.txt");
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Could not open photon parameters file" << fileName << file.errorString();
        return;
    }
    QTextStream out(&file);

    out << "START PARAMETERS\n";
    out << "id\n";
    if (m_saveCoordinates) {
        out << "x\n";
        out << "y\n";
        out << "z\n";
    }
    if (m_saveSurfaceSide)
        out << "side\n";
    if (m_savePhotonsID) {
        out << "previous ID\n";
        out << "next ID\n";
    }
    if (m_saveSurfaceID)
        out << "surface ID\n";
    out << "END PARAMETERS\n";


    out << "START SURFACES\n";
    for (int n = 0; n < m_surfaces.size(); n++)
        out << QString("%1 %2\n").arg(QString::number(n + 1), m_surfaces[n]->getURL());
    out << "END SURFACES\n";

    out << m_photonPower;
}

void PhotonsFile::writePhotons(QString fileName, const std::vector<Photon>& photons, ulong nBegin, ulong nEnd)
{
    if (nBegin >= nEnd)
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        qWarning() << "Could not open photon output file" << fileName << file.errorString();
        return;
    }
    QDataStream out(&file);

    ulong nMax = photons.size();
    double previousPhotonID = 0;
    for (ulong n = nBegin; n < nEnd; ++n)
    {
        const Photon& photon = photons[n];
        ulong urlId = 0;
        if (photon.surface) {
            urlId = m_surfaces.indexOf(photon.surface) + 1;
            if (urlId == 0) {
                m_surfaces << photon.surface;
                m_surfaceWorldToObject << photon.surface->getTransform().inversed();
                urlId = m_surfaces.size();
            }
        }

        // id
        out << double(++m_exportedPhotons);

        if (m_saveCoordinates) {
            vec3d pos = photon.pos;
            if (!m_saveCoordinatesGlobal && urlId > 0)
                pos = m_surfaceWorldToObject[urlId - 1].transformPoint(pos);
            out << pos.x << pos.y << pos.z;
        }

        if (m_saveSurfaceSide)
            out << double(photon.isFront);

        if (m_savePhotonsID) {
            if (photon.id < 1)	previousPhotonID = 0;
            out << previousPhotonID;
            previousPhotonID = m_exportedPhotons;

            if (n + 1 < nMax && photons[n + 1].id > 0)
                out << double(m_exportedPhotons + 1);
            else
                out << 0.;
        }

        if (m_saveSurfaceID)
            out << double(urlId);
    }

    if (out.status() != QDataStream::Ok)
        qWarning() << "Error writing photon output file" << fileName;
}
