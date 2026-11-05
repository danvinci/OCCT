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

#ifndef _IntCurveSurface_PolyhedronCache_HeaderFile
#define _IntCurveSurface_PolyhedronCache_HeaderFile

#include <IntCurveSurface_ThePolyhedronOfHInter.hxx>
#include <Standard_Transient.hxx>
#include <Standard_Type.hxx>

#include <array>
#include <map>
#include <memory>

//! Polyhedra of one surface's patches, each kept under its exact parameter bounds, for
//! intersecting many curves with that surface. A polyhedron depends only on the surface and the
//! bounds, so a kept one gives the intersection a new one would.
//! Not thread-safe: each thread uses a cache of its own.
class IntCurveSurface_PolyhedronCache : public Standard_Transient
{
  DEFINE_STANDARD_RTTIEXT(IntCurveSurface_PolyhedronCache, Standard_Transient)
public:
  //! Returns the polyhedron kept for the bounds, or nullptr if there is none.
  Standard_EXPORT const IntCurveSurface_ThePolyhedronOfHInter* Find(const double theU1,
                                                                    const double theV1,
                                                                    const double theU2,
                                                                    const double theV2) const;

  //! Keeps the polyhedron under the bounds and returns it.
  Standard_EXPORT const IntCurveSurface_ThePolyhedronOfHInter& Add(
    const double                                           theU1,
    const double                                           theV1,
    const double                                           theU2,
    const double                                           theV2,
    std::unique_ptr<IntCurveSurface_ThePolyhedronOfHInter> thePolyhedron);

  //! Returns the number of polyhedra kept.
  int Extent() const { return static_cast<int>(myPolyhedra.size()); }

private:
  std::map<std::array<double, 4>, std::unique_ptr<IntCurveSurface_ThePolyhedronOfHInter>>
    myPolyhedra;
};

#endif // _IntCurveSurface_PolyhedronCache_HeaderFile
