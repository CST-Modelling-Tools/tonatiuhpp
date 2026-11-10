#pragma once

#include <Inventor/SbVec2f.h>
#include "libraries/Coin3D/UserMField.h"


class TONATIUH_LIBRARIES MFVec2: public UserMField
{
    typedef UserMField inherited;

    SO_MFIELD_HEADER(MFVec2, SbVec2f, const SbVec2f&);

public:
    static void initClass(void);

    void setValues(int start, int num, const float xy[][2]);
    void set1Value(int idx, float x, float y);
    void set1Value(int idx, const float xy[2]);
    void setValue(float x, float y);
    void setValue(const float xy[2]);

    FieldEditor* getEditor() const;

private:
    FieldEditor* m_editor;
};

// GUI adapter registration; core includes no QWidget and never constructs one.
using TonatiuhMFVec2EditorCreator = FieldEditor* (*)(const QStringList&);
TONATIUH_LIBRARIES void tonatiuhSetMFVec2EditorCreator(TonatiuhMFVec2EditorCreator);
