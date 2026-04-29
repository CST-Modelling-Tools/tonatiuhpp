#pragma once

#include <QMap>
#include <QString>

#include "kernel/photons/PhotonsAbstract.h"

class QFile;
class Photon;

class PhotonsFile: public PhotonsAbstract
{

public:
    PhotonsFile();
    ~PhotonsFile();

    static QStringList getParameterNames() {return {"ExportDirectory", "ExportFile", "FileSize"};}
    void setParameter(QString name, QString value);

    bool startExport();
    ulong savePhotons(const std::vector<Photon>& photons);
    void setPhotonPower(double p) {m_photonPower = p;}
    bool endExport();
    bool hasExportError() const {return m_exportFailed;}

    NAME_ICON_FUNCTIONS("File", ":/PhotonsFile.png")

private:
    bool prepareDirectory();
    bool closeCurrentFile();
    bool openOutputFile(QString fileName);
    ulong writePhotons(const std::vector<Photon>& photon, ulong nBegin, ulong nEnd);

    QString m_dirName;
    QString m_fileName;
    QFile* m_file;
    QString m_filePath;
    bool m_exportFailed;
    bool m_oneFile;
    ulong m_nPhotonsPerFile;

    int m_fileCurrent;
    ulong m_exportedPhotons;
    double m_photonPower;
    QVector<InstanceNode*> m_surfaces; //? map
	QVector<Transform> m_surfaceWorldToObject;
};



#include "PhotonsFileWidget.h"

class PhotonsFileFactory:
    public QObject,
    public PhotonsFactoryT<PhotonsFile, PhotonsFileWidget>
{
    Q_OBJECT
    Q_INTERFACES(PhotonsFactory)
    Q_PLUGIN_METADATA(IID "tonatiuh.PhotonsFactory")
};
