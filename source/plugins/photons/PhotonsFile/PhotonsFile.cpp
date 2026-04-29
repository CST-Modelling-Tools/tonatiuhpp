#include "PhotonsFile.h"

#include <algorithm>
#include <iostream>

#include <QDataStream>
#include <QDebug>
#include <QByteArray>
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
    m_file(0),
    m_exportFailed(false),
    m_oneFile(true),
    m_nPhotonsPerFile(-1),
    m_fileCurrent(1),
    m_exportedPhotons(0),
    m_photonPower(0.)
{

}

PhotonsFile::~PhotonsFile()
{
    closeCurrentFile();
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
    if (m_exportedPhotons > 0) {
        if (m_exportFailed)
            return false;
        if (!prepareDirectory()) {
            m_exportFailed = true;
            return false;
        }
        return true;
    }

    closeCurrentFile();
    m_exportFailed = false;
    m_fileCurrent = 1;
    m_surfaces.clear();
    m_surfaceWorldToObject.clear();

    if (!prepareDirectory()) {
        m_exportFailed = true;
        return false;
    }

    QDir dir(m_dirName);
    QFileInfoList infoList;
    if (m_oneFile) {
        QString fileName = dir.absoluteFilePath(m_fileName + ".dat");
        infoList << QFileInfo(fileName);
    } else {
        QStringList filters(m_fileName + "_*.dat");
        infoList = dir.entryInfoList(filters, QDir::Files);
    }

    for (QFileInfo& fileInfo : infoList) {
        QFile file(fileInfo.absoluteFilePath());
        if (file.exists() && !file.remove()) {
            QString message = QString("Error deleting %1.\nThe file is in use or the directory is not writable. Please close it before continuing.")
                .arg(fileInfo.absoluteFilePath());
            qWarning() << message;
            QMessageBox::warning(0, "Tonatiuh", message);
            m_exportFailed = true;
            return false;
        }
    }

    return true;
}

bool PhotonsFile::prepareDirectory()
{
    QDir dir(m_dirName);
    if (!dir.exists() && !dir.mkpath(".")) {
        QString message = QString("Could not create photon export directory:\n%1").arg(dir.absolutePath());
        qWarning() << message;
        QMessageBox::warning(0, "Tonatiuh", message);
        return false;
    }

    QString probeName = dir.absoluteFilePath(".tonatiuhpp_write_test.tmp");
    QFile::remove(probeName);
    QFile probe(probeName);
    if (!probe.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QString message = QString("Could not write to photon export directory:\n%1\n%2").arg(dir.absolutePath(), probe.errorString());
        qWarning() << message;
        QMessageBox::warning(0, "Tonatiuh", message);
        return false;
    }

    if (probe.write("1", 1) != 1 || !probe.flush()) {
        QString message = QString("Could not write to photon export directory:\n%1\n%2").arg(dir.absolutePath(), probe.errorString());
        qWarning() << message;
        QMessageBox::warning(0, "Tonatiuh", message);
        probe.close();
        QFile::remove(probeName);
        return false;
    }

    probe.close();
    bool probeClosed = probe.error() == QFile::NoError;
    bool probeRemoved = QFile::remove(probeName);
    if (!probeClosed || !probeRemoved) {
        QString message = QString("Could not finalize photon export directory check:\n%1\n%2").arg(dir.absolutePath(), probe.errorString());
        qWarning() << message;
        QMessageBox::warning(0, "Tonatiuh", message);
        return false;
    }

    return true;
}

bool PhotonsFile::closeCurrentFile()
{
    if (!m_file)
        return true;

    bool ok = true;
    QString filePath = m_filePath;
    if (m_file->isOpen() && !m_file->flush()) {
        qWarning() << "Could not flush photon output file" << filePath << m_file->errorString();
        ok = false;
    }

    if (m_file->isOpen())
        m_file->close();
    if (m_file->error() != QFile::NoError) {
        qWarning() << "Could not close photon output file" << filePath << m_file->errorString();
        ok = false;
    }

    delete m_file;
    m_file = 0;
    m_filePath.clear();

    if (!ok)
        m_exportFailed = true;
    return ok;
}

bool PhotonsFile::openOutputFile(QString fileName)
{
    if (m_file && m_file->isOpen() && m_filePath == fileName)
        return true;

    if (!closeCurrentFile())
        return false;

    m_file = new QFile(fileName);
    m_filePath = fileName;
    if (!m_file->open(QIODevice::WriteOnly | QIODevice::Append)) {
        qWarning() << "Could not open photon output file" << fileName << m_file->errorString();
        delete m_file;
        m_file = 0;
        m_filePath.clear();
        m_exportFailed = true;
        return false;
    }

    return true;
}

ulong PhotonsFile::savePhotons(const std::vector<Photon>& photons)
{
    if (m_exportFailed)
        return 0;

    if (photons.empty())
        return 0;

    QDir dir(m_dirName);
    if (m_oneFile || m_nPhotonsPerFile == 0)
	{
        QString fileName = dir.absoluteFilePath(m_fileName + ".dat");
        if (!openOutputFile(fileName))
            return 0;
        return writePhotons(photons, 0, photons.size());
	}
	else
    {
        ulong writtenTotal = 0;
        ulong nBegin = 0;
        while (nBegin < photons.size()) {
            ulong rangeBegin = nBegin;
            m_fileCurrent = int(m_exportedPhotons/m_nPhotonsPerFile) + 1;
            ulong nFile = m_exportedPhotons % m_nPhotonsPerFile;
            ulong nEnd = rangeBegin + std::min<ulong>(m_nPhotonsPerFile - nFile, photons.size() - rangeBegin);
            QString fileName = QString("%1_%2.dat").arg(m_fileName, QString::number(m_fileCurrent));
            fileName = dir.absoluteFilePath(fileName);
            if (!openOutputFile(fileName))
                return writtenTotal;
            ulong written = writePhotons(photons, rangeBegin, nEnd);
            if (written < nEnd - rangeBegin)
            {
                writtenTotal += written;
                return writtenTotal;
            }
            if (m_exportedPhotons % m_nPhotonsPerFile == 0 && !closeCurrentFile())
                return writtenTotal;
            writtenTotal += written;
            nBegin += written;
        }
        return writtenTotal;
    }
}

bool PhotonsFile::endExport()
{
    bool dataFileClosed = closeCurrentFile();
    if (m_exportFailed)
        return false;

    if (!dataFileClosed)
        return false;

    QDir dir(m_dirName);
    QString fileName = dir.absoluteFilePath(m_fileName + "_parameters.txt");
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Could not open photon parameters file" << fileName << file.errorString();
        return false;
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

    if (out.status() != QTextStream::Ok) {
        qWarning() << "Error writing photon parameters file" << fileName;
        return false;
    }

    if (!file.flush()) {
        qWarning() << "Could not flush photon parameters file" << fileName << file.errorString();
        return false;
    }

    file.close();
    if (file.error() != QFile::NoError) {
        qWarning() << "Could not close photon parameters file" << fileName << file.errorString();
        return false;
    }

    return true;
}

ulong PhotonsFile::writePhotons(const std::vector<Photon>& photons, ulong nBegin, ulong nEnd)
{
    if (nBegin >= nEnd)
        return 0;

    if (!m_file || !m_file->isOpen()) {
        qWarning() << "Photon output file is not open";
        m_exportFailed = true;
        return 0;
    }

    QByteArray bytes;
    bytes.reserve(int(std::min<ulong>(nEnd - nBegin, 4096) * 80));
    QDataStream out(&bytes, QIODevice::WriteOnly);

    ulong nMax = photons.size();
    ulong written = 0;
    ulong exportedPhotons = m_exportedPhotons;
    double previousPhotonID = 0;
    QVector<InstanceNode*> surfaces = m_surfaces;
    QVector<Transform> surfaceWorldToObject = m_surfaceWorldToObject;

    for (ulong n = nBegin; n < nEnd; ++n)
    {
        const Photon& photon = photons[n];
        ulong urlId = 0;
        if (photon.surface) {
            urlId = surfaces.indexOf(photon.surface) + 1;
            if (urlId == 0) {
                surfaces << photon.surface;
                surfaceWorldToObject << photon.surface->getTransform().inversed();
                urlId = surfaces.size();
            }
        }

        // id
        double photonID = double(exportedPhotons + written + 1);
        out << photonID;

        if (m_saveCoordinates) {
            vec3d pos = photon.pos;
            if (!m_saveCoordinatesGlobal && urlId > 0)
                pos = surfaceWorldToObject[urlId - 1].transformPoint(pos);
            out << pos.x << pos.y << pos.z;
        }

        if (m_saveSurfaceSide)
            out << double(photon.isFront);

        if (m_savePhotonsID) {
            if (photon.id < 1)	previousPhotonID = 0;
            out << previousPhotonID;
            previousPhotonID = photonID;

            if (n + 1 < nMax && photons[n + 1].id > 0)
                out << double(photonID + 1);
            else
                out << 0.;
        }

        if (m_saveSurfaceID)
            out << double(urlId);

        if (out.status() != QDataStream::Ok) {
            qWarning() << "Error serializing photon output for" << m_filePath;
            m_exportFailed = true;
            return 0;
        }

        ++written;
    }

    qint64 fileStart = m_file->pos();
    qint64 bytesWritten = m_file->write(bytes);
    if (bytesWritten != bytes.size()) {
        qWarning() << "Could not write photon output file" << m_filePath << m_file->errorString();
        if (fileStart >= 0) {
            if (!m_file->resize(fileStart) || !m_file->seek(fileStart))
                qWarning() << "Could not restore photon output file after a failed write" << m_filePath << m_file->errorString();
        }
        m_exportFailed = true;
        return 0;
    }

    m_surfaces = surfaces;
    m_surfaceWorldToObject = surfaceWorldToObject;
    m_exportedPhotons += written;
    return written;
}
