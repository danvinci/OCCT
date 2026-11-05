// Copyright (c) 2026 OPEN CASCADE SAS
//
// This file is part of Open CASCADE Technology software library.
//
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License version 2.1 as published
// by the Free Software Foundation, with special exception defined in the file
// OCCT_LGPL_EXCEPTION.txt. Consult the file LICENSE_LGPL_21.txt included in OCCT
// distribution for complete text of the license and disclaimer of any warranty.
//
// Alternatively, this file may be used under the terms of Open CASCADE
// commercial license or contractual agreement.

#include <GeomAdaptor_Curve.hxx>
#include <GeomAdaptor_Surface.hxx>
#include <Geom_BSplineSurface.hxx>
#include <Geom_BezierCurve.hxx>
#include <IntCurveSurface_HInter.hxx>
#include <IntCurveSurface_IntersectionPoint.hxx>
#include <IntCurveSurface_PolyhedronCache.hxx>
#include <NCollection_Array1.hxx>
#include <NCollection_Array2.hxx>

#include <gtest/gtest.h>

#include <cmath>
#include <memory>
#include <utility>

namespace
{
// A cubic B-spline surface over [0, 2] x [0, 2], only C1 along u = 1 and v = 1: four C2 patches
occ::handle<GeomAdaptor_Surface> makeFourPatchSurface()
{
  constexpr int    THE_NB_POLES = 6;
  constexpr double THE_LAST_U   = 2.0;
  const double     aStep        = THE_LAST_U / (THE_NB_POLES - 1);
  constexpr double THE_HEIGHT   = 0.1;

  NCollection_Array2<gp_Pnt> aPoles(1, THE_NB_POLES, 1, THE_NB_POLES);
  for (int aRow = 1; aRow <= THE_NB_POLES; ++aRow)
  {
    for (int aCol = 1; aCol <= THE_NB_POLES; ++aCol)
    {
      const double aX    = aStep * (aRow - 1);
      const double aY    = aStep * (aCol - 1);
      const double aZ    = THE_HEIGHT * std::sin(aX) * std::cos(aY);
      aPoles(aRow, aCol) = gp_Pnt(aX, aY, aZ);
    }
  }
  NCollection_Array1<double> aKnots(1, 3);
  aKnots(1) = 0.0;
  aKnots(2) = 1.0;
  aKnots(3) = THE_LAST_U;
  NCollection_Array1<int> aMults(1, 3);
  aMults(1) = 4;
  aMults(2) = 2;
  aMults(3) = 4;
  const occ::handle<Geom_BSplineSurface> aBSpline =
    new Geom_BSplineSurface(aPoles, aKnots, aKnots, aMults, aMults, 3, 3);
  const occ::handle<GeomAdaptor_Surface> aSurface = new GeomAdaptor_Surface(aBSpline);
  return aSurface;
}

// A parabola through the surface at (theX, theY), neither a line nor a conic to the intersector
occ::handle<GeomAdaptor_Curve> makeCrossingCurve(const double theX, const double theY)
{
  constexpr double           THE_SPREAD = 0.1;
  NCollection_Array1<gp_Pnt> aPoles(1, 3);
  aPoles(1)                                    = gp_Pnt(theX - THE_SPREAD, theY - THE_SPREAD, -1.0);
  aPoles(2)                                    = gp_Pnt(theX + THE_SPREAD, theY, 0.0);
  aPoles(3)                                    = gp_Pnt(theX, theY + THE_SPREAD, 1.0);
  const occ::handle<Geom_BezierCurve>  aBezier = new Geom_BezierCurve(aPoles);
  const occ::handle<GeomAdaptor_Curve> aCurve  = new GeomAdaptor_Curve(aBezier);
  return aCurve;
}

void expectSamePoint(const IntCurveSurface_IntersectionPoint& theOne,
                     const IntCurveSurface_IntersectionPoint& theOther)
{
  EXPECT_EQ(theOne.U(), theOther.U());
  EXPECT_EQ(theOne.V(), theOther.V());
  EXPECT_EQ(theOne.W(), theOther.W());
}

void expectSamePoints(const IntCurveSurface_HInter& theOne, const IntCurveSurface_HInter& theOther)
{
  ASSERT_TRUE(theOne.IsDone());
  ASSERT_TRUE(theOther.IsDone());
  ASSERT_EQ(theOne.NbPoints(), theOther.NbPoints());
  for (int anIndex = 1; anIndex <= theOne.NbPoints(); ++anIndex)
  {
    expectSamePoint(theOne.Point(anIndex), theOther.Point(anIndex));
  }
}
} // namespace

// Every curve finds the points it finds without a cache, and the second curve builds nothing new
TEST(IntCurveSurface_PolyhedronCache, CurvesShareThePolyhedraAndFindTheSamePoints)
{
  const occ::handle<GeomAdaptor_Surface>             aSurface = makeFourPatchSurface();
  const occ::handle<IntCurveSurface_PolyhedronCache> aCache = new IntCurveSurface_PolyhedronCache();

  const occ::handle<GeomAdaptor_Curve> aFirst  = makeCrossingCurve(0.5, 0.5);
  const occ::handle<GeomAdaptor_Curve> aSecond = makeCrossingCurve(1.5, 1.5);

  IntCurveSurface_HInter aCached;
  aCached.SetPolyhedronCache(aCache);
  aCached.Perform(aFirst, aSurface);
  EXPECT_EQ(aCache->Extent(), 4);
  EXPECT_GE(aCached.NbPoints(), 1);

  IntCurveSurface_HInter aPlain;
  aPlain.Perform(aFirst, aSurface);
  expectSamePoints(aCached, aPlain);

  aCached.Perform(aSecond, aSurface);
  EXPECT_EQ(aCache->Extent(), 4);
  EXPECT_GE(aCached.NbPoints(), 1);

  aPlain.Perform(aSecond, aSurface);
  expectSamePoints(aCached, aPlain);
}

TEST(IntCurveSurface_PolyhedronCache, FindsOnlyTheExactBounds)
{
  const occ::handle<GeomAdaptor_Surface>             aSurface = makeFourPatchSurface();
  const occ::handle<IntCurveSurface_PolyhedronCache> aCache = new IntCurveSurface_PolyhedronCache();
  EXPECT_EQ(aCache->Find(0.0, 0.0, 1.0, 1.0), nullptr);

  constexpr int THE_NB_SAMPLES = 5;
  auto          aPolyhedron    = std::make_unique<IntCurveSurface_ThePolyhedronOfHInter>(aSurface,
                                                                             THE_NB_SAMPLES,
                                                                             THE_NB_SAMPLES,
                                                                             0.0,
                                                                             0.0,
                                                                             1.0,
                                                                             1.0);
  const IntCurveSurface_ThePolyhedronOfHInter& aKept =
    aCache->Add(0.0, 0.0, 1.0, 1.0, std::move(aPolyhedron));
  EXPECT_EQ(aCache->Find(0.0, 0.0, 1.0, 1.0), &aKept);
  EXPECT_EQ(aCache->Find(0.0, 0.0, 1.0, 2.0), nullptr);
}
