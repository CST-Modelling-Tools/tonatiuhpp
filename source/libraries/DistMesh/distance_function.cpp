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

namespace {
using DistMeshPoints = Eigen::Ref<Eigen::ArrayXXd const>;

struct RectangularDistance {
    Eigen::Ref<Eigen::ArrayXXd const> rectangle;
    Eigen::ArrayXd operator()(DistMeshPoints const points) const {
        Eigen::ArrayXXd result = (points.col(0) - rectangle(0, 0))
            .min(rectangle(1, 0) - points.col(0));

        for (int dim = 1; dim < points.cols(); ++dim) {
            result = result
                .min((points.col(dim) - rectangle(0, dim)))
                .min(rectangle(1, dim) - points.col(dim));
        }

        return -result;
    }
};

struct RectangleDistance {
    Eigen::Ref<Eigen::ArrayXXd const> rectangle;
    Eigen::ArrayXd operator()(DistMeshPoints const points) const {
        // distances to all 4 sides of rectangle
        auto d1 = rectangle(0, 1) - points.col(1);
        auto d2 = -rectangle(1, 1) + points.col(1);
        auto d3 = rectangle(0, 0) - points.col(0);
        auto d4 = -rectangle(1, 0) + points.col(0);

        // distances to all 4 corners of rectangle
        auto d5 = (d1.square() + d3.square()).sqrt();
        auto d6 = (d1.square() + d4.square()).sqrt();
        auto d7 = (d2.square() + d3.square()).sqrt();
        auto d8 = (d2.square() + d4.square()).sqrt();

        // distance to nearest side of rectangle
        Eigen::ArrayXd d = -(-d1).min(-d2).min(-d3).min(-d4);

        // check if smallest distance is to one of the corners
        d = (d1 > 0.0 && d3 > 0.0).select(d5, d);
        d = (d1 > 0.0 && d4 > 0.0).select(d6, d);
        d = (d2 > 0.0 && d3 > 0.0).select(d7, d);
        d = (d2 > 0.0 && d4 > 0.0).select(d8, d);

        return d;
    }
};

struct EllipticalDistance {
    Eigen::Ref<Eigen::ArrayXd const> radii;
    Eigen::Ref<Eigen::ArrayXd const> midpoint;
    Eigen::ArrayXd operator()(DistMeshPoints const points) const {
        if (midpoint.rows() == points.cols()) {
            if (radii.rows() == points.cols()) {
                return ((points.rowwise() - midpoint.transpose()).rowwise() / radii.transpose())
                    .square().rowwise().sum().sqrt() - 1.0;
            }
            else {
                return (points.rowwise() - midpoint.transpose())
                    .square().rowwise().sum().sqrt() - 1.0;
            }
        }
        else {
            if (radii.rows() == points.cols()) {
                return (points.rowwise() / radii.transpose())
                    .square().rowwise().sum().sqrt() - 1.0;
            }
            else {
                return points.square().rowwise().sum().sqrt() - 1.0;
            }
        }
    }
};

struct CircularDistance {
    double radius;
    Eigen::Ref<Eigen::ArrayXd const> midpoint;
    Eigen::ArrayXd operator()(DistMeshPoints const points) const {
        if (midpoint.rows() == points.cols()) {
            return (points.rowwise() - midpoint.transpose())
                .square().rowwise().sum().sqrt() - radius;
        }
        else {
            return points.square().rowwise().sum().sqrt() - radius;
        }
    }
};

struct PolygonDistance {
    Eigen::Ref<Eigen::ArrayXXd const> polygon;
    Eigen::ArrayXd operator()(DistMeshPoints const points) const {
        Eigen::ArrayXXd v(points.rows(), 2);
        Eigen::ArrayXXd w(points.rows(), 2);
        Eigen::ArrayXXd c1(points.rows(), 1);
        Eigen::ArrayXXd c2(points.rows(), 1);
        Eigen::ArrayXXd distance(points.rows(), polygon.rows());

        for (int i = 0, j = polygon.rows() - 1;
            i < polygon.rows(); j = i++) {
            v.rowwise() = polygon.row(i) - polygon.row(j);
            w = points.rowwise() - polygon.row(j);

            c1 = v.col(0) * w.col(0) + v.col(1) * w.col(1);
            c2 = v.col(0) * v.col(0) + v.col(1) * v.col(1);

            distance.col(i) =
                (c1 <= 0.0).select(
                    (points.rowwise() - polygon.row(j)).square().rowwise().sum().sqrt(),
                (c1 >= c2).select(
                    (points.rowwise() - polygon.row(i)).square().rowwise().sum().sqrt(),
                    (points - ((v.colwise() * (c1 / c2).col(0)).rowwise() + polygon.row(j)))
                        .square().rowwise().sum().sqrt()
                ));
        }

        return (1.0 - 2.0 * distmesh::utils::pointsInsidePoly(points, polygon)) *
            distance.rowwise().minCoeff();
    }
};
} // namespace

// creates distance function for a nd rectangular domain
distmesh::Functional distmesh::distanceFunction::rectangular(
    Eigen::Ref<Eigen::ArrayXXd const> const rectangle) {
    return distmesh::Functional(RectangularDistance{rectangle});
}

// creates the true distance function for a 2d rectangular domain
distmesh::Functional distmesh::distanceFunction::rectangle(
    Eigen::Ref<Eigen::ArrayXXd const> const rectangle) {
    return distmesh::Functional(RectangleDistance{rectangle});
}

// creates distance function for elliptical domains
distmesh::Functional distmesh::distanceFunction::elliptical(
    Eigen::Ref<Eigen::ArrayXd const> const radii,
    Eigen::Ref<Eigen::ArrayXd const> const midpoint) {
    return distmesh::Functional(EllipticalDistance{radii, midpoint});
}

// creates the true distance function for circular domains
distmesh::Functional distmesh::distanceFunction::circular(
    double const radius,
    Eigen::Ref<Eigen::ArrayXd const> const midpoint) {
    return distmesh::Functional(CircularDistance{radius, midpoint});
}

// creates distance function for a 2d domain described by polygon
// https://stackoverflow.com/questions/10983872/distance-from-a-point-to-a-polygon
distmesh::Functional distmesh::distanceFunction::polygon(
    Eigen::Ref<Eigen::ArrayXXd const> const polygon) {
    return distmesh::Functional(PolygonDistance{polygon});
}
