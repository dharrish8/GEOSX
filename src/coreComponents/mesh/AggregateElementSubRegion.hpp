/*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Copyright (c) 2019, Lawrence Livermore National Security, LLC.
 *
 * Produced at the Lawrence Livermore National Laboratory
 *
 * LLNL-CODE-746361
 *
 * All rights reserved. See COPYRIGHT for details.
 *
 * This file is part of the GEOSX Simulation Framework.
 *
 * GEOSX is a free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License (as published by the
 * Free Software Foundation) version 2.1 dated February 1999.
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#ifndef AGGREGATECELLSUBREGION_HPP_
#define AGGREGATECELLSUBREGION_HPP_

#include "ElementSubRegionBase.hpp"
#include "InterObjectRelation.hpp"

namespace geosx
{

class AggregateElementSubRegion : public ElementSubRegionBase
{
public:

  using NodeMapType=OneToOneRelation;

  static const string CatalogName()
  { return "AggregateCell"; }

  virtual const string getCatalogName() const override
  {
    return AggregateElementSubRegion::CatalogName();
  }

  virtual R1Tensor const & calculateElementCenter( localIndex k,
                                     const NodeManager& nodeManager,
                                     const bool useReferencePos = true) const override;

  virtual void CalculateCellVolumes( array1d<localIndex> const & indices,
                                     array1d<R1Tensor> const & X ) override
  {
      //TODO ?
  }

  virtual void setupRelatedObjectsInRelations( MeshLevel const * const mesh ) override;

  struct viewKeyStruct : ObjectManagerBase::viewKeyStruct
  {
    static constexpr auto elementVolumeString          = "elementVolume";
    static constexpr auto fineElementsListString       = "fineElements";
  };

  /*!
   * @brief returns the element to node relations.
   * @details The aggregates are elements composed of 1 node.
   * @param[in] k the index of the element.
   */
  virtual arraySlice1dRval<localIndex const> nodeList( localIndex const k ) const override
  { 
    arraySlice1dRval<localIndex> list(0);
    list[0] =k;
    return list;
  }

  /*!
   * @brief returns the element to node relations.
   * @details The aggregates are elements composed of 1 node.
   * @param[in] k the index of the element.
   */
  virtual arraySlice1dRval<localIndex> nodeList( localIndex const k ) override
  {
    arraySlice1dRval<localIndex> list(0);
    list[0] =k;
    return list;
  }

private:
  /// The elements to nodes relation is one to one relation.
  NodeMapType  m_toNodesRelation;
};
}

#endif