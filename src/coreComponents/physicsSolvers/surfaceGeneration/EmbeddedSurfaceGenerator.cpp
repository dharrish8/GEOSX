/*
 * ------------------------------------------------------------------------------------------------------------
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 * Copyright (c) 2018-2020 Lawrence Livermore National Security LLC
 * Copyright (c) 2018-2020 The Board of Trustees of the Leland Stanford Junior University
 * Copyright (c) 2018-2020 Total, S.A
 * Copyright (c) 2019-     GEOSX Contributors
 * All rights reserved
 *
 * See top level LICENSE, COPYRIGHT, CONTRIBUTORS, NOTICE, and ACKNOWLEDGEMENTS files for details.
 * ------------------------------------------------------------------------------------------------------------
 */

/**
 * @file EmbeddedSurfaceGenerator.cpp
 */

#include "EmbeddedSurfaceGenerator.hpp"

#include "mpiCommunications/CommunicationTools.hpp"
#include "mpiCommunications/NeighborCommunicator.hpp"
#include "mpiCommunications/SpatialPartition.hpp"
#include "finiteElement/FiniteElementDiscretizationManager.hpp"
#include "finiteVolume/FiniteVolumeManager.hpp"
#include "finiteVolume/FluxApproximationBase.hpp"
#include "managers/NumericalMethodsManager.hpp"
#include "mesh/SurfaceElementRegion.hpp"
#include "mesh/ExtrinsicMeshData.hpp"
#include "meshUtilities/ComputationalGeometry.hpp"
#include "physicsSolvers/solidMechanics/SolidMechanicsLagrangianFEMKernels.hpp"
#include "meshUtilities/SimpleGeometricObjects/GeometricObjectManager.hpp"
#include "meshUtilities/SimpleGeometricObjects/BoundedPlane.hpp"

#ifdef USE_GEOSX_PTP
#include "physicsSolvers/GEOSX_PTP/ParallelTopologyChange.hpp"
#endif
#include <set>

namespace geosx
{
using namespace dataRepository;
using namespace constitutive;

EmbeddedSurfaceGenerator::EmbeddedSurfaceGenerator( const std::string & name,
                                                    Group * const parent ):
  SolverBase( name, parent )
{
  registerWrapper( viewKeyStruct::solidMaterialNameString, &m_solidMaterialNames )->
    setInputFlag( InputFlags::REQUIRED )->
    setDescription( "Name of the solid material used in solid mechanic solver" );

  registerWrapper( viewKeyStruct::fractureRegionNameString, &m_fractureRegionName )->
    setInputFlag( dataRepository::InputFlags::OPTIONAL )->
    setApplyDefaultValue( "FractureRegion" );

  this->getWrapper< string >( viewKeyStruct::discretizationString )->
    setInputFlag( InputFlags::FALSE );

}

EmbeddedSurfaceGenerator::~EmbeddedSurfaceGenerator()
{}

void EmbeddedSurfaceGenerator::RegisterDataOnMesh( Group * const GEOSX_UNUSED_PARAM( MeshBodies ) )
{}

void EmbeddedSurfaceGenerator::InitializePostSubGroups( Group * const problemManager )
{
  /*
   * Here we generate embedded elements for fractures (or faults) that already exist in the domain and
   * were specified in the input file.
   */

  // Get domain
  DomainPartition * domain = problemManager->GetGroup< DomainPartition >( dataRepository::keys::domain );
  // Get geometric object manager
  GeometricObjectManager * geometricObjManager = problemManager->GetGroup< GeometricObjectManager >( "Geometry" );

  // Get meshLevel
  Group * const meshBodies = domain->getMeshBodies();
  MeshBody * const meshBody   = meshBodies->GetGroup< MeshBody >( 0 );
  MeshLevel * const meshLevel  = meshBody->GetGroup< MeshLevel >( 0 );

  // Get managers
  ElementRegionManager * const elemManager = meshLevel->getElemManager();
  NodeManager * const nodeManager = meshLevel->getNodeManager();
  EdgeManager * const edgeManager = meshLevel->getEdgeManager();
  arrayView2d< real64 const, nodes::REFERENCE_POSITION_USD > const & nodesCoord = nodeManager->referencePosition();

  // Get EmbeddedSurfaceSubRegions
  SurfaceElementRegion * const embeddedSurfaceRegion =
    elemManager->GetRegion< SurfaceElementRegion >( this->m_fractureRegionName );
  EmbeddedSurfaceSubRegion * const embeddedSurfaceSubRegion =
    embeddedSurfaceRegion->GetSubRegion< EmbeddedSurfaceSubRegion >( 0 );

  // Loop over all the fracture planes
  geometricObjManager->forSubGroups< BoundedPlane >( [&]( BoundedPlane & fracture )
  {
    /* 1. Find out if an element is cut by the fracture or not.
     * Loop over all the elements and for each one of them loop over the nodes and compute the
     * dot product between the distance between the plane center and the node and the normal
     * vector defining the plane. If two scalar products have different signs the plane cuts the
     * cell. If a nodes gives a 0 dot product it has to be neglected or the method won't work.
     */
    R1Tensor planeCenter  = fracture.getCenter();
    R1Tensor normalVector = fracture.getNormal();
    // Initialize variables
    globalIndex nodeIndex;
    integer isPositive, isNegative;
    R1Tensor distVec;

    elemManager->forElementSubRegionsComplete< CellElementSubRegion >(
      [&]( localIndex const er, localIndex const esr, ElementRegionBase &, CellElementSubRegion & subRegion )
    {
      arrayView2d< localIndex const, cells::NODE_MAP_USD > const cellToNodes = subRegion.nodeList();
      FixedOneToManyRelation const & cellToEdges = subRegion.edgeList();
      for( localIndex cellIndex =0; cellIndex<subRegion.size(); cellIndex++ )
      {
        isPositive = 0;
        isNegative = 0;
        for( localIndex kn =0; kn<subRegion.numNodesPerElement(); kn++ )
        {
          nodeIndex = cellToNodes[cellIndex][kn];
          distVec  = nodesCoord[nodeIndex];
          distVec -= planeCenter;
          // check if the dot product is zero
          if( Dot( distVec, normalVector ) > 0 )
          {
            isPositive = 1;
          }
          else if( Dot( distVec, normalVector ) < 0 )
          {
            isNegative = 1;
          }
        } // end loop over nodes
        if( isPositive * isNegative == 1 )
        {

          bool added = embeddedSurfaceSubRegion->AddNewEmbeddedSurface( cellIndex,
                                                                        esr,
                                                                        er,
                                                                        *nodeManager,
                                                                        *edgeManager,
                                                                        cellToEdges,
                                                                        &fracture );
          if( added )
          {
            GEOSX_LOG_LEVEL_RANK_0( 2, "Element " << cellIndex << " is fractured" );
          }
        }
      } // end loop over cells
    } );// end loop over subregions
  } );// end loop over thick planes

  ElementRegionManager::ElementViewAccessor< arrayView1d< integer const > > const & cellElemGhostRank =
    elemManager->ConstructArrayViewAccessor< integer, 1 >( ObjectManagerBase::viewKeyStruct::ghostRankString );

  embeddedSurfaceSubRegion->inheritGhostRank( cellElemGhostRank );

  // Sum across all ranks
  localIndex localNum = embeddedSurfaceSubRegion->size();
  localIndex totalNum = 0;
  MpiWrapper::allReduce( &localNum,
                         &totalNum,
                         1,
                         MPI_SUM,
                         MPI_COMM_GEOSX );

  GEOSX_LOG_LEVEL_RANK_0( 1, "Number of embedded surface elements: " << totalNum );

  // Populate EdgeManager for embedded surfaces.
  EdgeManager & embSurfEdgeManager = meshLevel->getEmbdSurfEdgeManager();

  EmbeddedSurfaceSubRegion::EdgeMapType & embSurfToEdgeMap = embeddedSurfaceSubRegion->edgeList();
  EmbeddedSurfaceSubRegion::NodeMapType & embSurfToNodeMap = embeddedSurfaceSubRegion->nodeList();

  localIndex numOfPoints = nodeManager->embSurfNodesPosition().size( 0 );

  embSurfEdgeManager.BuildEdges( numOfPoints, embSurfToNodeMap.toViewConst(), embSurfToEdgeMap );
}

void EmbeddedSurfaceGenerator::InitializePostInitialConditions_PreSubGroups( Group * const GEOSX_UNUSED_PARAM ( problemManager ) )
{
  // I don't think there is  much to do here.
}


void EmbeddedSurfaceGenerator::postRestartInitialization( Group * const GEOSX_UNUSED_PARAM( domain0 ) )
{
  // Not sure about this for now.
  std::cout << "postRestartInitialization \n";
}


real64 EmbeddedSurfaceGenerator::SolverStep( real64 const & GEOSX_UNUSED_PARAM( time_n ),
                                             real64 const & GEOSX_UNUSED_PARAM( dt ),
                                             const int GEOSX_UNUSED_PARAM( cycleNumber ),
                                             DomainPartition & domain )
{
  real64 rval = 0;
  /*
   * This should be the method that generates new fracture elements based on the propagation criterion of choice.
   */

  // Add the embedded elements to the fracture stencil.
  addToFractureStencil( domain );

  return rval;
}

void EmbeddedSurfaceGenerator::addToFractureStencil( DomainPartition & domain )
{
  // Add embedded elements to the fracture Stencil
  NumericalMethodsManager & numericalMethodManager = domain.getNumericalMethodManager();

  FiniteVolumeManager & fvManager = numericalMethodManager.getFiniteVolumeManager();

  for( auto & mesh : domain.getMeshBodies()->GetSubGroups() )
  {
    MeshLevel * meshLevel = Group::group_cast< MeshBody * >( mesh.second )->getMeshLevel( 0 );

    for( localIndex a=0; a<fvManager.numSubGroups(); ++a )
    {
      FluxApproximationBase * const fluxApprox = fvManager.GetGroup< FluxApproximationBase >( a );
      if( fluxApprox!=nullptr )
      {
        fluxApprox->addEDFracToFractureStencil( *meshLevel,
                                                this->m_fractureRegionName );
      }
    }
  }

}

REGISTER_CATALOG_ENTRY( SolverBase,
                        EmbeddedSurfaceGenerator,
                        std::string const &, dataRepository::Group * const )

} /* namespace geosx */
