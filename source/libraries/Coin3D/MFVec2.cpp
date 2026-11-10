#include "MFVec2.h"

#include <atomic>


namespace {
std::atomic<TonatiuhMFVec2EditorCreator> s_editorCreator{nullptr};
}

void tonatiuhSetMFVec2EditorCreator(TonatiuhMFVec2EditorCreator creator)
{
    s_editorCreator.store(creator);
}

SO_MFIELD_SOURCE_MALLOC(MFVec2, SbVec2f, const SbVec2f&);


void MFVec2::initClass()
{
    SO_MFIELD_INIT_CLASS(MFVec2, inherited);
}

SbBool MFVec2::read1Value(SoInput* in, int idx)
{
    return in->read(values[idx][0]) &&
           in->read(values[idx][1]);
}

void MFVec2::write1Value(SoOutput* out, int idx) const
{
    const SbVec2f& v = (*this)[idx];

    out->write(v[0]);
    if (!out->isBinary()) out->write(' ');
    out->write(v[1]);
}

void MFVec2::setValues(int start, int numarg, const float xy[][2])
{
    int last = start + numarg;
    if (last > maxNum)
        allocValues(last);
    else if (last > num)
        num = last;

    for (int i = 0; i < numarg; ++i)
        values[start + i] = SbVec2f(xy[i]);

    valueChanged();
}

void MFVec2::set1Value(int idx, float x, float y)
{
    set1Value(idx, SbVec2f(x, y));
}

void MFVec2::set1Value(int idx, const float xy[2])
{
    set1Value(idx, SbVec2f(xy));
}

void MFVec2::setValue(float x, float y)
{
    setValue(SbVec2f(x, y));
}

void MFVec2::setValue(const float xy[2])
{
    if (!xy) setValue(SbVec2f(xy));
    else setNum(0);
}

FieldEditor* MFVec2::getEditor() const
{
    auto creator = s_editorCreator.load();
    return creator ? creator(getNames()) : nullptr;
}
