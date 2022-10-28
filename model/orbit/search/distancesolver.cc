/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2022 Universidade de Vigo
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Miguel Rodríguez Pérez <miguel@det.uvigo.gal>
 */

#include "distancesolver.h"

#include "../satpos/planet.h"
#include "ns3/circular-orbit.h"
#include "../circular-orbit-impl.h"

#include <boost/optional/optional.hpp>
#include <boost/units/quantity.hpp>
#include <boost/units/pow.hpp>
#include <boost/units/operators.hpp>
#include <boost/math/constants/constants.hpp>

#include <boost/units/systems/si/time.hpp>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_roots.h>
#include <gsl/gsl_math.h>

using namespace icarus::satpos::planet::constants;

namespace ns3 {
namespace icarus {
namespace orbit {

namespace {
using boost::units::quantity;
using namespace boost::units::si;

class GroundObserver
{
  using plane_angle = boost::units::si::plane_angle;
  using length = boost::units::si::length;
  using time = boost::units::si::time;

  using vector3d = std::tuple<quantity<length>, quantity<length>, quantity<length>>;

public:
  constexpr GroundObserver (quantity<plane_angle> latitude, quantity<plane_angle> longitude,
                            quantity<length> radius) noexcept
      : latitude (latitude), longitude (longitude), radius (radius)
  {
  }

  constexpr vector3d
  operator() (boost::units::quantity<time> t) const noexcept
  {
    using namespace boost::math::double_constants;

    const auto apparent_longitude = longitude + Earth.getRotationRate () * t;

    const auto x = radius * cos (apparent_longitude) * cos (latitude);
    const auto y = radius * sin (apparent_longitude) * cos (latitude);
    const auto z = radius * sin (latitude);

    return std::make_tuple (x, y, z);
  }

private:
  const quantity<plane_angle> latitude, longitude;
  const quantity<length> radius;
};

template <typename T>
constexpr auto
sq_distance (std::tuple<T, T, T> a, std::tuple<T, T, T> b) -> auto
{
  T a1, a2, a3, b1, b2, b3;

  std::tie (a1, a2, a3) = a;
  std::tie (b1, b2, b3) = b;

  return pow<2> (b1 - a1) + pow<2> (b2 - a2) + pow<2> (b3 - a3);
}

class DistanceSolver
{
  using Satellite = CircularOrbitMobilityModelImpl;
  using time = boost::units::si::time;

public:
  DistanceSolver (quantity<time> min, quantity<time> max, const GroundObserver &observer,
                  const Satellite &sat, quantity<length> target_distance) noexcept
      : min (min),
        max (max),
        obs (observer),
        sat (sat),
        target_sq (pow<2> (target_distance)),
        T (gsl_root_fsolver_brent),
        s (gsl_root_fsolver_alloc (T))
  {
    old_handler = gsl_set_error_handler_off ();
  }
  ~DistanceSolver () noexcept
  {
    gsl_root_fsolver_free (s);
    gsl_set_error_handler (old_handler);
  }

  boost::optional<quantity<time>>
  operator() () noexcept
  {
    solve ();

    return solution;
  }

private:
  const quantity<time> min, max;
  const GroundObserver &obs;
  const Satellite &sat;
  const quantity<boost::units::multiply_typeof_helper<length, length>::type> target_sq;

  const gsl_root_fsolver_type *T;
  gsl_root_fsolver *s;
  bool run = false;
  boost::optional<quantity<time>> solution{};
  gsl_error_handler_t *old_handler = nullptr;

  static double
  distance_function_rep (double x, void *params) noexcept
  {
    DistanceSolver *self = static_cast<DistanceSolver *> (params);
    return self->distance_function (x);
  }

  void
  solve () noexcept
  {
    if (run)
      return;

    run = true;
    double x_lower = min.value (), x_upper = max.value ();

    gsl_function F;
    F.function = &DistanceSolver::distance_function_rep;
    F.params = this;
    gsl_root_fsolver_set (s, &F, min.value (), max.value ());

    int iter = 0;
    const int max_iter = 1000;
    int status;

    do
      {
        iter++;
        status = gsl_root_fsolver_iterate (s);
        double r = gsl_root_fsolver_root (s);
        x_lower = gsl_root_fsolver_x_lower (s);
        x_upper = gsl_root_fsolver_x_upper (s);
        status = gsl_root_test_interval (x_lower, x_upper, 0, 0.000001);

        if (status == GSL_SUCCESS)
          {
            /* We simply check that the distance drops below target near the
            found root. This is to ensure that the result is not just a point
            where the distance gets close to 0, but without even crossing the
            target threshold. */
            if (std::min (distance_function (r - 1), distance_function (r + 1)) < 0)
              {
                solution = r * second;
              }
            break;
          }
    } while (status == GSL_CONTINUE && iter < max_iter);
  }

  double
  distance_function (double x) const noexcept
  {
    quantity<time> t (x * seconds);

    return (sq_distance (sat.getCartesianPositionRightAscensionDeclination (t), obs (t)) -
            target_sq)
        .value ();
  }
};

} // namespace

boost::optional<quantity<boost::units::si::time>>
findNextCross (quantity<boost::units::si::time> now, CircularOrbitMobilityModelImpl satellite,
               quantity<length> distance, quantity<plane_angle> latitude,
               quantity<plane_angle> longitude, quantity<length> radius)
{

  const auto orbitalPeriod = satellite.getOrbitalPeriod ();
  const GroundObserver observer (latitude, longitude, radius);

  // Find the first cross, going half a period at a time
  auto sol = DistanceSolver (now, now + orbitalPeriod, observer, satellite, distance) ();
  // If there is a solution, check whether there is a previous one
  if (sol)
    {
      if (now < *sol - 1 * second)
        {
          const auto previous =
              DistanceSolver (now, *sol - 1 * second, observer, satellite, distance) ();
          sol = previous ? previous : sol;
        }
    }

  return sol;
}

} // namespace orbit
} // namespace icarus
} // namespace ns3
