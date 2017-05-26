/*
 * HypoElasticLinear.cpp
 *
 *  Created on: Aug 4, 2016
 *      Author: rrsettgast
 */

#include "LinearElasticIsotropic.hpp"

namespace geosx
{
using namespace dataRepository;
using namespace cxx_utilities;
namespace constitutive
{


LinearElasticIsotropic::LinearElasticIsotropic( std::string const & name, ManagedGroup * const parent ):
  ConstitutiveBase(name, parent ),
  m_YoungsModulus(this->GetGroup(keys::parameterData).RegisterViewWrapper<real64>(keys::youngsModulus).reference()),
  m_BulkModulus(this->GetGroup(keys::parameterData).RegisterViewWrapper<real64>(keys::bulkModulus).reference()),
  m_ShearModulus(this->GetGroup(keys::parameterData).RegisterViewWrapper<real64>(keys::shearModulus).reference()),
  m_PoissonRatio(this->GetGroup(keys::parameterData).RegisterViewWrapper<real64>(keys::poissonRatio).reference()),
  m_Density(this->GetGroup(keys::parameterData).RegisterViewWrapper<real64>(keys::density).reference()),
  m_devStress(this->GetGroup(keys::stateData).RegisterViewWrapper<r2Sym_array>(keys::deviatorStress).reference()),
  m_meanStress(this->GetGroup(keys::stateData).RegisterViewWrapper<real64_array>(keys::meanStress).reference())

{
  // TODO Auto-generated constructor stub

}

LinearElasticIsotropic::~LinearElasticIsotropic()
{
  // TODO Auto-generated destructor stub
}

void LinearElasticIsotropic::FillDocumentationNode( ManagedGroup * const group )
{

  DocumentationNode * const docNode = this->getDocumentationNode();

  docNode->setName(this->CatalogName());
  docNode->setSchemaType("Node");
  docNode->setShortDescription("Linear Elastic Isotropic Constitutive Relation");

  ManagedGroup & parameterData = this->GetGroup( keys::parameterData );
  DocumentationNode * const parameterDocNode = parameterData.getDocumentationNode();
  parameterDocNode->setSchemaType("Node");
  parameterDocNode->setShortDescription("Parameters for Linear Elastic Isotropic Constitutive Relation");

  parameterDocNode->AllocateChildNode( keys::youngsModulus,
                                       keys::youngsModulus,
                                       -1,
                                       "real64",
                                       "real64",
                                       "Young's Elastic Modulus",
                                       "Young's Elastic Modulus",
                                       "-1",
                                       "",
                                       1,
                                       1,
                                       0 );

  parameterDocNode->AllocateChildNode( keys::bulkModulus,
                                       keys::bulkModulus,
                                       -1,
                                       "real64",
                                       "real64",
                                       "Elastic Bulk Modulus",
                                       "Elastic Bulk Modulus",
                                       "-1",
                                       "",
                                       1,
                                       1,
                                       0 );

  parameterDocNode->AllocateChildNode( keys::shearModulus,
                                       keys::shearModulus,
                                       -1,
                                       "real64",
                                       "real64",
                                       "Elastic Bulk Modulus",
                                       "Elastic Bulk Modulus",
                                       "-1",
                                       "",
                                       1,
                                       1,
                                       0 );

  parameterDocNode->AllocateChildNode( keys::poissonRatio,
                                       keys::poissonRatio,
                                       -1,
                                       "real64",
                                       "real64",
                                       "Elastic Poisson's Ratio",
                                       "Elastic Poisson's Ratio",
                                       "-1",
                                       "",
                                       1,
                                       1,
                                       0 );

  parameterDocNode->AllocateChildNode( keys::density,
                                       keys::density,
                                       -1,
                                       "real64",
                                       "real64",
                                       "density",
                                       "density",
                                       "-1",
                                       "",
                                       1,
                                       1,
                                       0 );


  ManagedGroup & stateData     = this->GetGroup( keys::stateData );
  DocumentationNode * const stateDocNode = stateData.getDocumentationNode();
  stateDocNode->setSchemaType("Node");
  stateDocNode->setShortDescription("State for Linear Elastic Isotropic Constitutive Relation");

  stateDocNode->AllocateChildNode( keys::deviatorStress,
                                   keys::deviatorStress,
                                   -1,
                                   "r2Sym_array",
                                   "r2Sym_array",
                                   "Stress",
                                   "Stress",
                                   "0",
                                   "",
                                   1,
                                   0,
                                   0 );
}

void LinearElasticIsotropic::ReadXML_PostProcess()
{
  ManagedGroup & parameterData = this->GetGroup( keys::parameterData );
  real64 & nu = *( parameterData.getData<real64>(keys::poissonRatio) );
  real64 & E  = *( parameterData.getData<real64>(keys::youngsModulus) );
  real64 & K  = *( parameterData.getData<real64>(keys::bulkModulus) );
  real64 & G  = *( parameterData.getData<real64>(keys::shearModulus) );

  int numConstantsSpecified = 0;
  if( nu >= 0.0 )
  {
    ++numConstantsSpecified;
  }
  if( E >= 0.0 )
  {
    ++numConstantsSpecified;
  }
  if( K >= 0.0 )
  {
    ++numConstantsSpecified;
  }
  if( G >= 0.0 )
  {
    ++numConstantsSpecified;
  }

  if( numConstantsSpecified == 2 )
  {
    if( nu >= 0.0 && E >= 0.0 )
    {
      K = E / (3 * ( 1 - 2*nu ) );
      G = E / (2 * ( 1 + nu ) );
    }
    else if( !( K >= 0.0 && G >= 0.0 ) )
    {
      string const message = "A specific pair of elastic constants is required. Either (K,G) or (E,nu)";
      SLIC_ERROR(message);
    }
    else
    {
      E = 9 * K * G / ( 3 * K + G );
      nu = ( 3 * K - 2 * G ) / ( 2 * ( 3 * K + G ) );
    }
  }
  else
  {
    string const message = std::to_string(numConstantsSpecified) + " Elastic Constants Specified. Must specify 2 constants!";
    SLIC_ERROR(message);
  }
}

void LinearElasticIsotropic::StateUpdate( dataRepository::ManagedGroup const * const input,
                                          dataRepository::ManagedGroup const * const parameters,
                                          dataRepository::ManagedGroup * const stateVariables,
                                          integer const systemAssembleFlag ) const
{

  index_t numberOfMaterialPoints = stateVariables->size();
  ViewWrapper<real64_array>::rtype_const K = parameters->getData<real64_array>(std::string("BulkModulus"));
  ViewWrapper<real64_array>::rtype_const G = parameters->getData<real64_array>(std::string("ShearModulus"));

  ViewWrapper<real64_array>::rtype meanStress = stateVariables->getData<real64_array>(std::string("MeanStress"));
  ViewWrapper<real64_array>::rtype S11 = stateVariables->getData<real64_array>(std::string("S11"));
  ViewWrapper<real64_array>::rtype S22 = stateVariables->getData<real64_array>(std::string("S22"));
  ViewWrapper<real64_array>::rtype S33 = stateVariables->getData<real64_array>(std::string("S33"));
  ViewWrapper<real64_array>::rtype S23 = stateVariables->getData<real64_array>(std::string("S23"));
  ViewWrapper<real64_array>::rtype S13 = stateVariables->getData<real64_array>(std::string("S13"));
  ViewWrapper<real64_array>::rtype S12 = stateVariables->getData<real64_array>(std::string("S12"));

  ViewWrapper<real64_array>::rtype_const D11 = input->getData<real64_array>(std::string("D11"));
  ViewWrapper<real64_array>::rtype_const D22 = input->getData<real64_array>(std::string("D22"));
  ViewWrapper<real64_array>::rtype_const D33 = input->getData<real64_array>(std::string("D33"));
  ViewWrapper<real64_array>::rtype_const D23 = input->getData<real64_array>(std::string("D23"));
  ViewWrapper<real64_array>::rtype_const D13 = input->getData<real64_array>(std::string("D13"));
  ViewWrapper<real64_array>::rtype_const D12 = input->getData<real64_array>(std::string("D12"));

  for( index_t i=0 ; i<numberOfMaterialPoints ; ++i )
  {
    real volumeStrain = ( D11[i] + D22[i] + D33[i] );
    meanStress[i] += volumeStrain * K[i];

    S11[i] += ( D11[i] - volumeStrain/3.0 ) * 2.0 * G[i];
    S22[i] += ( D22[i] - volumeStrain/3.0 ) * 2.0 * G[i];
    S33[i] += ( D33[i] - volumeStrain/3.0 ) * 2.0 * G[i];
    S23[i] += ( D23[i] ) * 2.0 * G[i];
    S13[i] += ( D13[i] ) * 2.0 * G[i];
    S12[i] += ( D12[i] ) * 2.0 * G[i];

  }

  if ( systemAssembleFlag == 1 )
  {
    ViewWrapper<real64_array>::rtype K11 = stateVariables->getData<real64_array>(std::string("K11"));
    ViewWrapper<real64_array>::rtype K22 = stateVariables->getData<real64_array>(std::string("K22"));
    ViewWrapper<real64_array>::rtype K33 = stateVariables->getData<real64_array>(std::string("K33"));
    ViewWrapper<real64_array>::rtype K23 = stateVariables->getData<real64_array>(std::string("K23"));
    ViewWrapper<real64_array>::rtype K13 = stateVariables->getData<real64_array>(std::string("K13"));
    ViewWrapper<real64_array>::rtype K12 = stateVariables->getData<real64_array>(std::string("K12"));
    ViewWrapper<real64_array>::rtype K44 = stateVariables->getData<real64_array>(std::string("K44"));
    ViewWrapper<real64_array>::rtype K55 = stateVariables->getData<real64_array>(std::string("K55"));
    ViewWrapper<real64_array>::rtype K66 = stateVariables->getData<real64_array>(std::string("K66"));

    for( index_t i=0 ; i<numberOfMaterialPoints ; ++i )
    {
      real Stiffness[6][6] = {{ K[i]+4.0/3.0*G[i], K[i]-2.0/3.0*G[i], K[i]-2.0/3.0*G[i], 0,               0,        0 },
                              { K[i]-2.0/3.0*G[i], K[i]+4.0/3.0*G[i], K[i]-2.0/3.0*G[i], 0,               0,        0 },
                              { K[i]-2.0/3.0*G[i], K[i]-2.0/3.0*G[i], K[i]+4.0/3.0*G[i], 0,               0,        0 },
                              {                 0,                 0,                 0, 2.0*G[i],        0,        0 },
                              {                 0,                 0,                 0,        0, 2.0*G[i],        0 },
                              {                 0,                 0,                 0,        0 ,       0, 2.0*G[i] } };

      K11[i] = Stiffness[0][0];
      K22[i] = Stiffness[1][1];
      K33[i] = Stiffness[2][2];
      K44[i] = Stiffness[3][3];
      K55[i] = Stiffness[4][4];
      K66[i] = Stiffness[5][5];
      K23[i] = Stiffness[1][2];
      K13[i] = Stiffness[0][2];
      K12[i] = Stiffness[0][1];
    }
  }


}

R2SymTensor LinearElasticIsotropic::StateUpdatePoint( R2SymTensor const & D,
                                               R2Tensor const & Rot,
                                               int32 const i,
                                               integer const systemAssembleFlag )
{

  
//  auto & params = this->GetGroup(keys::parameterData);
//  auto & state = this->GetGroup(keys::stateData);
//  std::cout<<&m_BulkModulus<<" , "<<this->GetGroup(keys::parameterData).getData<real64>(keys::bulkModulus)<<std::endl;
//  std::cout<<&m_meanStress<<" , "<<&(this->GetGroup(keys::stateData).getData<real64_array>(keys::meanStress))<<std::endl;
//  
  
  real64 volumeStrain = D.Trace();
  m_meanStress[i] += volumeStrain * m_BulkModulus;

  R2SymTensor temp = D;
  temp.PlusIdentity(-volumeStrain / 3.0);
  temp *= 2.0 * m_ShearModulus;
  m_devStress[i] += temp;


  temp.QijAjkQlk(m_devStress[i],Rot);
  m_devStress[i] = temp;

  temp.PlusIdentity(m_meanStress[i]);
  return temp;
}



REGISTER_CATALOG_ENTRY( ConstitutiveBase, LinearElasticIsotropic, std::string const &, ManagedGroup * const )
}
} /* namespace geosx */
