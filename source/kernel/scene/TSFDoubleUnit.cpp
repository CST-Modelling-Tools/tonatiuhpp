#include "TSFDoubleUnit.h"

#include <QString>
#include "libraries/math/gcf.h"


SO_SFIELD_SOURCE(TSFDoubleUnit, double, double)
//SO_SFIELD_CONSTRUCTOR_SOURCE(_class_);
//SO_SFIELD_VALUE_SOURCE(TSFDoubleUnit, double, double)
//SO_SFIELD_REQUIRED_SOURCE(TSFDoubleUnit)


void TSFDoubleUnit::initClass(void)
{
    SO_SFIELD_INIT_CLASS(TSFDoubleUnit, SoSField);
}

/*
TSFDoubleUnit::TSFDoubleUnit()
{
    unit = Default;
}

TSFDoubleUnit::~TSFDoubleUnit()
{

}
*/

SbBool TSFDoubleUnit::readValue(SoInput* in)
{
    SbString r;
    if (!in->read(r)) return FALSE;

    QString s = r.getString();

    double f;
    if (s.endsWith("d")) {
        s = s.left(s.length() - 1);
        unit = Degree;
        f = gcf::degree;
    } else {
        unit = Default;
        f = 1.;
    }

    bool ok;
    double v = s.toDouble(&ok);
    if (ok) {
        setValue(v*f);
        return TRUE;
    }
    return FALSE;
}

void TSFDoubleUnit::writeValue(SoOutput* out) const
{
    out->setFloatPrecision(6);
    if (unit == Default)
        out->write(getValue());
    else if (unit == Degree) {
        out->write(float(getValue()/gcf::degree));
        out->write("d");
    } else // unknown
        out->write(getValue());
}
