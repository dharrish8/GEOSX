/*
 * ------------------------------------------------------------------------------------------------------------
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 * Copyright (c) 2018-2019 Lawrence Livermore National Security LLC
 * Copyright (c) 2018-2019 The Board of Trustees of the Leland Stanford Junior University
 * Copyright (c) 2018-2019 Total, S.A
 * Copyright (c) 2019-     GEOSX Contributors
 * All right reserved
 *
 * See top level LICENSE, COPYRIGHT, CONTRIBUTORS, NOTICE, and ACKNOWLEDGEMENTS files for details.
 * ------------------------------------------------------------------------------------------------------------
 */

/*
 * @file InternalWellGenerator.cpp
 *
 */

#include "InternalWellGenerator.hpp"

#include "mpiCommunications/CommunicationTools.hpp"
#include "managers/DomainPartition.hpp"
#include "mesh/MeshBody.hpp"
#include "WellElementRegion.hpp"
#include "WellElementSubRegion.hpp"
#include "PerforationData.hpp"
#include "Perforation.hpp"

namespace geosx
{
using namespace dataRepository;

InternalWellGenerator::InternalWellGenerator( string const & name, Group * const parent ):
  WellGeneratorBase( name, parent )
{
  registerWrapper(viewKeyStruct::nodeCoords, &m_inputPolyNodeCoords, false )->
    setInputFlag(InputFlags::REQUIRED)->
    setSizedFromParent(0)->
    setDescription("physical coordinates of the well polyline nodes");

  registerWrapper(viewKeyStruct::segmentConn, &m_segmentToPolyNodeMap, false )->
    setInputFlag(InputFlags::REQUIRED)->
    setSizedFromParent(0)->
    setDescription("connectivity of the polyline segments");

}

InternalWellGenerator::~InternalWellGenerator()
{
  // TODO Auto-generated destructor stub
}

void InternalWellGenerator::PostProcessInput()
{
  GEOS_ERROR_IF( getName().find("well") == std::string::npos,
                 "Currently, the well generator must contain the word well in its name ");

  GEOS_ERROR_IF( m_inputPolyNodeCoords.size(1) != m_nDims,
                 "Invalid number of physical coordinates in " << viewKeyStruct::nodeCoords << " for well " << getName() ); 

  GEOS_ERROR_IF( m_segmentToPolyNodeMap.size(1) != 2,
                 "Invalid size in " << viewKeyStruct::segmentConn << " for well " << getName() ); 

  GEOS_ERROR_IF( m_inputPolyNodeCoords.size(0)-1 != m_segmentToPolyNodeMap.size(0),
                 "Incompatible sizes of " << viewKeyStruct::nodeCoords << " and " << viewKeyStruct::segmentConn << " in well " << getName() ); 
 
  GEOS_ERROR_IF( m_crossSectionArea <= 0,
                 "Invalid " << viewKeyStruct::crossSectionArea << " in well " << getName() );

  GEOS_ERROR_IF( m_wellRegionName.empty(), 
                 "Invalid well region name in well " << getName() );

  GEOS_ERROR_IF( m_meshBodyName.empty(), 
                 "Invalid mesh name in well " << getName() );

  GEOS_ERROR_IF( m_wellControlsName.empty(), 
                 "Invalid well constraint name in well " << getName() );


  // TODO: add more checks here
  // TODO: check that the connectivity of the well is valid
  // TODO: check that with no branching we can go from top to bottom and touch all the elements
}

void InternalWellGenerator::GeneratePolyLine()
{
  // convert the 2D array to an 1D array of R1Tensor
  m_polyNodeCoords.resize( m_inputPolyNodeCoords.size(0) );
  for (globalIndex inode = 0; inode < m_inputPolyNodeCoords.size(0); ++inode)
  {
    R1Tensor coords = { m_inputPolyNodeCoords[inode][0], 
                        m_inputPolyNodeCoords[inode][1], 
                        m_inputPolyNodeCoords[inode][2] };
    m_polyNodeCoords[inode] = coords;
  }
}

REGISTER_CATALOG_ENTRY( MeshGeneratorBase, InternalWellGenerator, std::string const &, Group * const )
}
