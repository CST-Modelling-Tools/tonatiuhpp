#include "ScientificFactoryCatalog.h"

#include "kernel/air/AirExponential.h"
#include "kernel/air/AirPolynomial.h"
#include "kernel/air/AirVacuum.h"
#include "kernel/material/MaterialAbsorber.h"
#include "kernel/material/MaterialFresnelUnpolarized.h"
#include "kernel/material/MaterialRough.h"
#include "kernel/material/MaterialTransparent.h"
#include "kernel/material/MaterialVirtual.h"
#include "kernel/profiles/ProfileBox.h"
#include "kernel/profiles/ProfileCircular.h"
#include "kernel/profiles/ProfilePolygon.h"
#include "kernel/profiles/ProfileRectangular.h"
#include "kernel/profiles/ProfileRegular.h"
#include "kernel/profiles/ProfileTriangle.h"
#include "kernel/shape/ShapeCone.h"
#include "kernel/shape/ShapeCube.h"
#include "kernel/shape/ShapeCylinder.h"
#include "kernel/shape/ShapeParabolic.h"
#include "kernel/shape/ShapePlanar.h"
#include "kernel/shape/ShapeSphere.h"
#include "kernel/sun/SunShapePillbox.h"
#include "kernel/trackers/TrackerArmature1A.h"
#include "kernel/trackers/TrackerArmature2A.h"
#include "kernel/trackers/TrackerArmature2AwD.h"

namespace ScientificFactoryCatalog
{
std::vector<std::unique_ptr<TFactory>> createBuiltInFactories()
{
    std::vector<std::unique_ptr<TFactory>> factories;
    factories.reserve(24);

    // Preserve the established registration order and public factory names.
    factories.emplace_back(std::make_unique<SunFactoryT<SunShapePillbox>>());

    factories.emplace_back(std::make_unique<AirFactoryT<AirVacuum>>());
    factories.emplace_back(std::make_unique<AirFactoryT<AirExponential>>());
    factories.emplace_back(std::make_unique<AirFactoryT<AirPolynomial>>());

    factories.emplace_back(std::make_unique<ShapeFactoryT<ShapePlanar>>());
    factories.emplace_back(std::make_unique<ShapeFactoryT<ShapeCube>>());
    factories.emplace_back(std::make_unique<ShapeFactoryT<ShapeCone>>());
    factories.emplace_back(std::make_unique<ShapeFactoryT<ShapeCylinder>>());
    factories.emplace_back(std::make_unique<ShapeFactoryT<ShapeSphere>>());
    factories.emplace_back(std::make_unique<ShapeFactoryT<ShapeParabolic>>());

    factories.emplace_back(std::make_unique<ProfileFactoryT<ProfileBox>>());
    factories.emplace_back(std::make_unique<ProfileFactoryT<ProfileRectangular>>());
    factories.emplace_back(std::make_unique<ProfileFactoryT<ProfileCircular>>());
    factories.emplace_back(std::make_unique<ProfileFactoryT<ProfileRegular>>());
    factories.emplace_back(std::make_unique<ProfileFactoryT<ProfileTriangle>>());
    factories.emplace_back(std::make_unique<ProfileFactoryT<ProfilePolygon>>());

    factories.emplace_back(std::make_unique<MaterialFactoryT<MaterialAbsorber>>());
    factories.emplace_back(std::make_unique<MaterialFactoryT<MaterialVirtual>>());
    factories.emplace_back(std::make_unique<MaterialFactoryT<MaterialTransparent>>());
    factories.emplace_back(std::make_unique<MaterialFactoryT<MaterialFresnelUnpolarized>>());
    factories.emplace_back(std::make_unique<MaterialFactoryT<MaterialRough>>());

    factories.emplace_back(std::make_unique<TrackerFactoryT<TrackerArmature1A>>());
    factories.emplace_back(std::make_unique<TrackerFactoryT<TrackerArmature2A>>());
    factories.emplace_back(std::make_unique<TrackerFactoryT<TrackerArmature2AwD>>());

    return factories;
}
} // namespace ScientificFactoryCatalog
