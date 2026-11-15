// --------------------------------------------------------------------
// This file is part of libDistMesh.
//
// libDistMesh is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// libDistMesh is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with libDistMesh. If not, see <http://www.gnu.org/licenses/>.
//
// Copyright (C) 2015 Patrik Gebhardt
// Contact: patrik.gebhardt@rub.de
// --------------------------------------------------------------------

#include "DistMesh/distmesh.h"

#include <stdexcept>

namespace {
using DistMeshPoints = Eigen::Ref<Eigen::ArrayXXd const>;
using DistMeshFunction = distmesh::Functional::function_t;

enum class BinaryOperation { Add, Subtract, Multiply, Divide, Minimum, Maximum };

struct NegateFunction {
    DistMeshFunction function;
    Eigen::ArrayXd operator()(DistMeshPoints const points) const {
        return -function(points);
    }
};

struct BinaryFunction {
    DistMeshFunction lhs;
    DistMeshFunction rhs;
    BinaryOperation operation;

    Eigen::ArrayXd operator()(DistMeshPoints const points) const {
        switch (operation) {
        case BinaryOperation::Add:      return lhs(points) + rhs(points);
        case BinaryOperation::Subtract: return lhs(points) - rhs(points);
        case BinaryOperation::Multiply: return lhs(points) * rhs(points);
        case BinaryOperation::Divide:   return lhs(points) / rhs(points);
        case BinaryOperation::Minimum:  return lhs(points).min(rhs(points));
        case BinaryOperation::Maximum:  return lhs(points).max(rhs(points));
        }
        throw std::logic_error("Unknown DistMesh binary operation");
    }
};

struct ScalarFunction {
    DistMeshFunction function;
    double scalar;
    BinaryOperation operation;
    bool scalarOnLeft;

    Eigen::ArrayXd operator()(DistMeshPoints const points) const {
        if (scalarOnLeft) {
            switch (operation) {
            case BinaryOperation::Add:      return scalar + function(points);
            case BinaryOperation::Subtract: return scalar - function(points);
            case BinaryOperation::Multiply: return scalar * function(points);
            case BinaryOperation::Divide:   return scalar / function(points);
            default: break;
            }
        } else {
            switch (operation) {
            case BinaryOperation::Add:      return function(points) + scalar;
            case BinaryOperation::Subtract: return function(points) - scalar;
            case BinaryOperation::Multiply: return function(points) * scalar;
            case BinaryOperation::Divide:   return function(points) / scalar;
            default: break;
            }
        }
        throw std::logic_error("Unknown DistMesh scalar operation");
    }
};

struct AbsoluteFunction {
    DistMeshFunction function;
    Eigen::ArrayXd operator()(DistMeshPoints const points) const {
        return function(points).abs();
    }
};

struct ShiftFunction {
    DistMeshFunction function;
    Eigen::Ref<Eigen::ArrayXd const> offset;
    Eigen::ArrayXd operator()(DistMeshPoints const points) const {
        return function(points.rowwise() - offset.transpose());
    }
};

struct Rotate2DFunction {
    DistMeshFunction function;
    double angle;
    Eigen::ArrayXd operator()(DistMeshPoints const points) const {
        Eigen::ArrayXXd transformedPoints(points.rows(), points.cols());
        transformedPoints.col(0) = points.col(0) * std::cos(angle) + points.col(1) * std::sin(angle);
        transformedPoints.col(1) = -points.col(0) * std::sin(angle) + points.col(1) * std::cos(angle);

        return function(transformedPoints);
    }
};
} // namespace

// assignment operator
distmesh::Functional& distmesh::Functional::operator=(Functional const& rhs) {
    this->function() = rhs.function();
    return *this;
}

distmesh::Functional& distmesh::Functional::operator=(Functional&& rhs) {
    this->function() = std::move(rhs.function());
    return *this;
}

Eigen::ArrayXd distmesh::Functional::operator()(DistMeshPoints const points) const {
    return this->function()(points);
}

distmesh::Functional distmesh::Functional::operator-() const {
    return Functional(NegateFunction{this->function()});
}

distmesh::Functional& distmesh::Functional::operator+=(Functional const& rhs) {
    auto const func = this->function();
    this->function() = BinaryFunction{func, rhs.function(), BinaryOperation::Add};
    return *this;
}

distmesh::Functional& distmesh::Functional::operator+=(double const rhs) {
    auto const func = this->function();
    this->function() = ScalarFunction{func, rhs, BinaryOperation::Add, false};
    return *this;
}

distmesh::Functional& distmesh::Functional::operator-=(Functional const& rhs) {
    auto const func = this->function();
    this->function() = BinaryFunction{func, rhs.function(), BinaryOperation::Subtract};
    return *this;
}

distmesh::Functional& distmesh::Functional::operator-=(double const rhs) {
    auto const func = this->function();
    this->function() = ScalarFunction{func, rhs, BinaryOperation::Subtract, false};
    return *this;
}

distmesh::Functional& distmesh::Functional::operator*=(Functional const& rhs) {
    auto const func = this->function();
    this->function() = BinaryFunction{func, rhs.function(), BinaryOperation::Multiply};
    return *this;
}

distmesh::Functional& distmesh::Functional::operator*=(double const rhs) {
    auto const func = this->function();
    this->function() = ScalarFunction{func, rhs, BinaryOperation::Multiply, false};
    return *this;
}

distmesh::Functional& distmesh::Functional::operator/=(Functional const& rhs) {
    auto const func = this->function();
    this->function() = BinaryFunction{func, rhs.function(), BinaryOperation::Divide};
    return *this;
}

distmesh::Functional& distmesh::Functional::operator/=(double const rhs) {
    auto const func = this->function();
    this->function() = ScalarFunction{func, rhs, BinaryOperation::Divide, false};
    return *this;
}

namespace distmesh {
Functional operator+(Functional const& lhs, Functional const& rhs) {
    return Functional(BinaryFunction{lhs.function(), rhs.function(), BinaryOperation::Add});
}
Functional operator+(Functional const& lhs, double const rhs) {
    return Functional(ScalarFunction{lhs.function(), rhs, BinaryOperation::Add, false});
}
Functional operator+(double const lhs, Functional const& rhs) {
    return Functional(ScalarFunction{rhs.function(), lhs, BinaryOperation::Add, true});
}
Functional operator-(Functional const& lhs, Functional const& rhs) {
    return Functional(BinaryFunction{lhs.function(), rhs.function(), BinaryOperation::Subtract});
}
Functional operator-(Functional const& lhs, double const rhs) {
    return Functional(ScalarFunction{lhs.function(), rhs, BinaryOperation::Subtract, false});
}
Functional operator-(double const lhs, Functional const& rhs) {
    return Functional(ScalarFunction{rhs.function(), lhs, BinaryOperation::Subtract, true});
}
Functional operator*(Functional const& lhs, Functional const& rhs) {
    return Functional(BinaryFunction{lhs.function(), rhs.function(), BinaryOperation::Multiply});
}
Functional operator*(Functional const& lhs, double const rhs) {
    return Functional(ScalarFunction{lhs.function(), rhs, BinaryOperation::Multiply, false});
}
Functional operator*(double const lhs, Functional const& rhs) {
    return Functional(ScalarFunction{rhs.function(), lhs, BinaryOperation::Multiply, true});
}
Functional operator/(Functional const& lhs, Functional const& rhs) {
    return Functional(BinaryFunction{lhs.function(), rhs.function(), BinaryOperation::Divide});
}
Functional operator/(Functional const& lhs, double const rhs) {
    return Functional(ScalarFunction{lhs.function(), rhs, BinaryOperation::Divide, false});
}
Functional operator/(double const lhs, Functional const& rhs) {
    return Functional(ScalarFunction{rhs.function(), lhs, BinaryOperation::Divide, true});
}
} // namespace distmesh

distmesh::Functional distmesh::Functional::min(Functional const& rhs) const {
    return Functional(BinaryFunction{this->function(), rhs.function(), BinaryOperation::Minimum});
}

distmesh::Functional distmesh::Functional::max(Functional const& rhs) const {
    return Functional(BinaryFunction{this->function(), rhs.function(), BinaryOperation::Maximum});
}

distmesh::Functional distmesh::Functional::abs() const {
    return Functional(AbsoluteFunction{this->function()});
}

distmesh::Functional distmesh::Functional::shift(Eigen::Ref<Eigen::ArrayXd const> const offset) const {
    return Functional(ShiftFunction{this->function(), offset});
}

distmesh::Functional distmesh::Functional::rotate2D(double const angle) const {
    return Functional(Rotate2DFunction{this->function(), angle});
}
