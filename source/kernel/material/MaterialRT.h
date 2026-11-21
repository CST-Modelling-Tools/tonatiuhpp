#pragma once

#include "kernel/node/TNode.h"
#include "libraries/math/3D/vec3d.h"

#include <cstdint>

struct DifferentialGeometry;
class Random;
class Ray;

// All supported materials report probabilities; only the shared kernel makes
// the categorical Russian-roulette decision. Invalid scattering is an error.
enum class MaterialInteraction : std::uint8_t {
    SpecularReflection,
    DiffuseReflection,
    Transmission,
    Absorbed
};

struct MaterialProbabilities {
    double specular = 0.;
    double diffuse = 0.;
    double transmission = 0.;
    // Physical absorptivity is the nonnegative remainder.
};

struct MaterialInteractionState {
    vec3d microfacetNormal;
    double incidentIndex = 1.;
    double transmittedIndex = 1.;
    double cosineTransmitted = 0.;
};

class TONATIUH_KERNEL MaterialRT: public TNode
{
    SO_NODE_ABSTRACT_HEADER(MaterialRT);

public:
    static void initClass();

    // Intentionally NON-VIRTUAL: the interaction decision is impossible to
    // override through the production MaterialRT* interface. For every hit,
    // the kernel validates P and samples exactly one categorical event.
    MaterialInteraction interact(const Ray& incoming, const DifferentialGeometry& geometry,
                                 Random& random, Ray& outgoing) const;

    NAME_ICON_FUNCTIONS("X", ":/MaterialX.png")

protected:
    // A material may prepare a microfacet normal before selecting a Fresnel
    // event, but must not choose the event itself. State stays on the stack.
    virtual MaterialProbabilities probabilities(
        const Ray&, const DifferentialGeometry&, Random&, MaterialInteractionState&) const = 0;
    // Called only for the selected surviving branch. The kernel forces
    // outgoing.origin to the actual surface hit; scatter supplies direction.
    virtual void scatter(MaterialInteraction, const Ray&, const DifferentialGeometry&,
                         Random&, const MaterialInteractionState&, Ray&) const = 0;
};


#include "kernel/node/TFactory.h"

class MaterialFactory: public TFactory
{
public:
    virtual MaterialRT* create() const = 0;
};

Q_DECLARE_INTERFACE(MaterialFactory, "tonatiuh.MaterialFactory")


template<class T>
class MaterialFactoryT: public MaterialFactory
{
public:
    QString name() const {return T::getClassName();}
    QIcon icon() const {return QIcon(T::getClassIcon());}
    void init() const {T::initClass();}
    T* create() const {return new T;}
};
