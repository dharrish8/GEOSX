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

/**
 * @file Group.hpp
 */

#ifndef GEOSX_DATAREPOSITORY_GROUP_HPP_
#define GEOSX_DATAREPOSITORY_GROUP_HPP_

#include "InputFlags.hpp"
#include "ObjectCatalog.hpp"
#include "MappedVector.hpp"
#include "RestartFlags.hpp"
#include "Wrapper.hpp"
#include "xmlWrapper.hpp"

#include <iostream>

#ifndef USE_DYNAMIC_CASTING
/// macro definition to specify whether or not to use dynamic_cast
#define USE_DYNAMIC_CASTING 1;
#endif

#ifndef NOCHARTOSTRING_KEYLOOKUP
/// macro definition to enable/disable char * lookups
#define NOCHARTOSTRING_KEYLOOKUP 0
#endif

/* Forward declaration of axom::sidre::Group */
namespace axom
{
namespace sidre
{
class Group;
}
}

/**
 * namespace to encapsulate GEOSX
 */
namespace geosx
{
namespace dataRepository
{

/// the default key type for the hierarchy.
using keyType = string;

/// the default index type for the hierarchy.
using indexType = localIndex;

/**
 * @class Group
 *
 * The Group class serves as a "node" in a hierarchy of the dataRepository. The data structure is built as a
 * hierarchy of Group objects, or objects derived from group objects.
 */
class Group
{
public:
  /// the type of MappedVector to use for the subGroup collection
  using subGroupMap = MappedVector< Group, Group *, keyType, indexType >;

  /// type of the MappedVector to use for the collection of wrappers.
  using wrapperMap = MappedVector< WrapperBase, WrapperBase *, keyType, indexType >;

  /**
   * @name constructors, destructor, copy, move, assignments
   */
  ///@{

  /**
   * @param[in] name the name of this object manager
   * @param[in]
   */
  explicit Group( std::string const & name,
                  Group * const parent );


  /**
   * @brief move constructor
   * @param[in] source source Group
   */
  Group( Group && source );

  /**
   *
   */
  virtual ~Group();



  Group() = delete;
  Group( Group const & source ) = delete;
  Group & operator=( Group const & ) = delete;
  Group & operator=( Group && ) = delete;

  ///@}


  /**
   * @name Static Factory Catalog Functions
   */
  ///@{

  using CatalogInterface = cxx_utilities::CatalogInterface< Group, std::string const &, Group * const >;
  static CatalogInterface::CatalogType & GetCatalog();
  ///@}


  /// returns typeid(*this)
  virtual const std::type_info & get_typeid() const
  {
    return typeid(*this);
  }

  /**
   * @brief Check a type_info against the type_info of this Group
   * @param typeToCheck value to check against
   * @return true of types are the same, false if not
   */
  bool CheckTypeID( std::type_info const & typeToCheck ) const
  {
    return typeToCheck == get_typeid() ? true : false;
  }


  /**
   * @brief Register a new subgroup
   * @tparam T        The type of the group to register
   * @param name      The repository name of the group
   * @param newObject A unique_ptr to the object that is being registered.
   * @return pointer to the subgroup
   *
   * This registration function takes an Group or class derived from
   * Group and registers it as a subgroup of this Group. The
   * data repository takes ownership of the object that is passed in through
   * a unique_ptr.
   */
  template< typename T = Group >
  T * RegisterGroup( std::string const & name, std::unique_ptr< Group > newObject );

  /**
   *
   * @brief Register a new subgroup
   * @tparam T        The type of the group to register
   * @param name      The repository name of the group
   * @param newObject Pointer to the object tp register
   * @param takeOwnership flag to indicate whether or not the repository should
   *                      take ownership of the group.
   * @return pointer to the subgroup
   *
   * This registration function creates and registers a Group or class
   * derived from Group and registers it as a subgroup of this
   * Group. The data repository takes ownership of the new object if
   * \p takeOwnership is true.
   */
  template< typename T = Group >
  T * RegisterGroup( std::string const & name,
                     T * newObject,
                     bool const takeOwnership );

  /**
   * @brief      Register a new subgroup
   * @tparam T   The type of the group to register
   * @param name The repository name of the group
   * @return     pointer to the subgroup
   *
   * This registration function creates and registers a Group or class
   * derived from Group and registers it as a subgroup of this
   * Group. The data repository takes ownership of the new object.
   */
  template< typename T = Group >
  T * RegisterGroup( std::string const & name )
  {
    return RegisterGroup< T >( name, std::move( std::make_unique< T >( name, this )) );
  }

  /**
   * @brief          Register a new subgroup
   * @tparam T       The type of the group to register
   * @param keyIndex A KeyIndex object that will be used to specify the name of
   *                 the new group.
   * @return         pointer to the subgroup
   *
   * This registration function creates and registers a Group or class
   * derived from Group and registers it as a subgroup of this
   * Group. The data repository takes ownership of the new object.
   */
  template< typename T = Group >
  T * RegisterGroup( subGroupMap::KeyIndex & keyIndex )
  {
    T * rval = RegisterGroup< T >( keyIndex.Key(), std::move( std::make_unique< T >( keyIndex.Key(), this )) );
    keyIndex.setIndex( this->m_subGroups.getIndex( keyIndex.Key()) );
    return rval;
  }

  /**
   * @brief             Register a new subgroup
   * @tparam T          The type of the group to register
   * @tparam TBASE      The type of the group that contains an object catalog
   *                    for the creation of a new \p T
   * @param name        The repository name of the new group
   * @param catalogName The catalog name of the type
   * @return            A pointer to the subgroup
   *
   * This registration function creates and registers a Group or class
   * derived from Group and registers it as a subgroup of this
   * Group. The \p TBASE type provide the object catalog that is used
   * to create the new group. The data repository takes ownership of the new
   * object.
   *
   */
  template< typename T = Group, typename TBASE = Group >
  T * RegisterGroup( std::string const & name, std::string const & catalogName )
  {
    std::unique_ptr< TBASE > newGroup = TBASE::CatalogInterface::Factory( catalogName, name, this );
    return RegisterGroup< T >( name, std::move( newGroup ) );
  }

  /**
   * @brief       Downcast a Group *
   * @tparam T    Pointer to the type to downcast into
   * @param group A pointer the group to be casted
   * @return a    Pointer to \p T that refers to the downcasted group
   */
  template< typename T >
  static T group_cast( Group * group )
  {
#ifdef USE_DYNAMIC_CASTING
    return dynamic_cast< T >( group );
#else
    return static_cast< T >( group );
#endif
  }

  /**
   * @brief       Downcast a Group const *
   * @tparam T    Pointer to the type to downcast into
   * @param group A pointer the group to be casted
   * @return a    Pointer to \p T that refers to the downcasted group
   */
  template< typename T >
  static T group_cast( Group const * group )
  {
#ifdef USE_DYNAMIC_CASTING
    return dynamic_cast< T >( group );
#else
    return static_cast< T >( group );
#endif
  }

  /**
   * @brief       Downcast this Group
   * @tparam T    Pointer to the type to downcast into
   * @return a    Pointer to \p T that refers to the this
   */
  template< typename T >
  T group_cast()
  {
#ifdef USE_DYNAMIC_CASTING
    return dynamic_cast< T >( this );
#else
    return static_cast< T >( this );
#endif
  }

  /**
   * @brief       Downcast this Group
   * @tparam T    Pointer to the type to downcast into
   * @return a    Pointer to \p T that refers to the this
   */
  template< typename T >
  T group_cast() const
  {
#ifdef USE_DYNAMIC_CASTING
    return dynamic_cast< T >( this );
#else
    return static_cast< T >( this );
#endif
  }


  /**
   * @brief Get a Group from the repository using direct index
   * @param index integral index to use for as a lookup
   * @return A pointer to \p T that refers to the group retrieved from the
   *         repository.
   */
  template< typename T = Group >
  T * GetGroup( localIndex index )
  {
    return group_cast< T * >( m_subGroups[index] );
  }

  /**
   * @brief Get a Group from the repository using direct index
   * @param index integral index to use for as a lookup
   * @return A pointer to const \p T that refers to the group retrieved from the
   *         repository.
   */
  template< typename T = Group >
  T const * GetGroup( localIndex index ) const
  {
    return group_cast< T const * >( m_subGroups[index] );
  }

  /**
   * @brief Get a Group from the repository using string lookup
   * @param name Name of the group to retrieve
   * @return A pointer to \p T that refers to the group retrieved from the
   *         repository.
   */
  template< typename T = Group >
  T * GetGroup( string const & name )
  {
    return group_cast< T * >( m_subGroups[name] );
  }

  /**
   * @brief Get a Group from the repository using string lookup
   * @param name Name of the group to retrieve
   * @return A pointer to const \p T that refers to the group retrieved from the
   *         repository.
   */
  template< typename T = Group >
  T const * GetGroup( string const & name ) const
  {
    return group_cast< T const * >( m_subGroups[name] );
  }

  /**
   * @brief Get a Group from the repository using KeyIndex lookup
   * @param key the KeyIndex to use for the lookup
   * @return A pointer to \p T that refers to the group retrieved from the
   *         repository.
   */
  template< typename T = Group >
  T * GetGroup( subGroupMap::KeyIndex & key )
  {
    return group_cast< T * >( m_subGroups[key] );
  }

  /**
   * @brief Get a Group from the repository using KeyIndex lookup
   * @param key the KeyIndex to use for the lookup
   * @return A pointer to const \p T that refers to the group retrieved from
   *         the repository.
   */
  template< typename T = Group >
  T const * GetGroup( subGroupMap::KeyIndex & key ) const
  {
    return group_cast< T const * >( m_subGroups[key] );
  }

  /**
   * @brief Get a Group from the repository using KeyIndex lookup
   * @param key the KeyIndex to use for the lookup. Note that
   *            const-correctness may be broken if the key is incorrect.
   * @return A pointer to \p T that refers to the group retrieved from the
   *         repository.
   */
  template< typename T = Group >
  T * GetGroup( subGroupMap::KeyIndex const & key )
  {
    return group_cast< T * >( m_subGroups[key] );
  }

  /**
   * @brief Get a Group from the repository using KeyIndex lookup
   * @param key the KeyIndex to use for the lookup. Note that
   *            const-correctness may be broken if the key is incorrect.
   * @return A pointer to const \p T that refers to the group retrieved from
   *         the repository.
   */
  template< typename T = Group >
  T const * GetGroup( subGroupMap::KeyIndex const & key ) const
  {
    return group_cast< T const * >( m_subGroups[key] );
  }

  /**
   * @brief This will grab the pointer to an object in the data structure
   * @param[in] path a unix-style string (absolute, relative paths valid)
   * @return A pointer to const \p T that refers to the target group.
   */
  template< typename T = Group >
  T const * GetGroupByPath( string const & path ) const
  {
    // needed for getting root correctly with GetGroupByPath("/");
    if( path.empty())
    {
      return group_cast< T const * >( this );
    }

    size_t directoryMarker = path.find( '/' );

    if( directoryMarker == std::string::npos )
    {
      // Target should be a child of this group
      return this->GetGroup< T >( path );
    }
    else
    {
      // Split the path
      string const child = path.substr( 0, directoryMarker );
      string const subPath = path.substr( directoryMarker+1, path.size());

      if( directoryMarker == 0 )            // From root
      {
        if( this->getParent() == nullptr )  // At root
        {
          return this->GetGroupByPath< T >( subPath );
        }
        else                               // Not at root
        {
          return this->getParent()->GetGroupByPath< T >( path );
        }
      }
      else if( child[0] == '.' )
      {
        if( child[1] == '.' )               // '../' = Reverse path
        {
          return this->getParent()->GetGroupByPath< T >( subPath );
        }
        else                               // './' = This path
        {
          return this->GetGroupByPath< T >( subPath );
        }
      }
      else
      {
        return m_subGroups[child]->GetGroupByPath< T >( subPath );
      }
    }
  }

  /**
   * @brief This will grab the pointer to an object in the data structure
   * @param[in] path a unix-style string (absolute, relative paths valid)
   * @return A pointer to \p T that refers to the target group.
   */
  template< typename T = Group >
  T * GetGroupByPath( string const & path )
  {
    return const_cast< T * >(const_cast< Group const * >(this)->GetGroupByPath< T >( path ));
  }

  /**
   * @brief Get the subgroups object
   * @return a reference to the sub-group map.
   */
  subGroupMap & GetSubGroups()
  {
    return m_subGroups;
  }

  /**
   * @brief Get the subgroups object
   * @return a reference to const that points to the sub-group map.
   */
  subGroupMap const & GetSubGroups() const
  {
    return m_subGroups;
  }

  /**
   * @brief return the number of sub groups in this Group
   * @return number of sub groups in this Group
   */
  localIndex numSubGroups() const { return m_subGroups.size(); }

  /**
   * @name FUNCTION GROUP for applyLambdaToGroup()
   * @brief These functions apply the specified lambda function to a container if the container can be
   *        casted to the templated type/s.
   * @tparam CASTTYPE the first type that will be used in the attempted casting of container.
   * @tparam CASTTYPES a variadic list of types that will be used in the attempted casting of
   *                    container.
   * @tparam LAMBDA the type of lambda function to call in the function
   * @param[in] container A pointer to the container which will be passed to the lambda function
   * @param[in] lambda the lambda function to call in the function
   * @return A boolean to indicate whether the lambda was successfully applied to the container.
   *
   * This function is useful when trying to apply a lambda that passes a pointer to an container,
   * but it is desired that the lambda is only executed if the container can be casted to a certain
   * type. The variadic list consisting of CASTTYPE/S will be used recursively to check if the
   * container is able to be casted to the one of these types. The first type in the CASTTYPE/S list
   * will be used to execute the lambda, and the function will return true.
   */
  ///@{

  /** \cond SKIPME */
  template< typename CONTAINERTYPE, typename LAMBDA >
  static bool applyLambdaToContainer( CONTAINERTYPE const * const GEOSX_UNUSED_ARG( group ), LAMBDA && GEOSX_UNUSED_ARG( lambda ) )
  { return false; }

  template< typename CONTAINERTYPE, typename LAMBDA >
  static bool applyLambdaToContainer( CONTAINERTYPE * const GEOSX_UNUSED_ARG( group ), LAMBDA && GEOSX_UNUSED_ARG( lambda ) )
  { return false; }
  /** \endcond */

  template< typename CONTAINERTYPE, typename CASTTYPE, typename ... CASTTYPES, typename LAMBDA >
  static bool applyLambdaToContainer( CONTAINERTYPE const * const container, LAMBDA && lambda )
  {
    bool rval = false;
    CASTTYPE const * const castedContainer = dynamic_cast< CASTTYPE const * >( container );
    if( castedContainer!= nullptr )
    {
      lambda( castedContainer );
      rval = true;
    }
    else
    {
      rval = applyLambdaToContainer< CONTAINERTYPE, CASTTYPES... >( container, std::forward< LAMBDA >( lambda ) );
    }
    return rval;
  }

  template< typename CONTAINERTYPE, typename CASTTYPE, typename ... CASTTYPES, typename LAMBDA >
  static bool applyLambdaToContainer( CONTAINERTYPE * const container, LAMBDA && lambda )
  {
    bool rval = false;
    CASTTYPE * const castedContainer = dynamic_cast< CASTTYPE * >( container );
    if( castedContainer!= nullptr )
    {
      lambda( castedContainer );
      rval = true;
    }
    else
    {
      rval = applyLambdaToContainer< CONTAINERTYPE, CASTTYPES... >( container, std::forward< LAMBDA >( lambda ) );
    }
    return rval;
  }
  ///@}


  /**
   * @name FUNCTION GROUP for forSubGroups()
   * @brief These functions apply the specified lambda function to a group if the group can be
   *        casted to the templated type/s.
   * @tparam GROUPTYPE The first type that will be used in the attempted casting of group.
   * @tparam GROUPTYPES A variadic list of types that will be used in the attempted casting of
   *                    group.
   * @tparam LAMBDA The type of lambda function to call in the function
   * @param[in] lambda The lambda function to call in the function
   * @param[in] subgroupNames Optional list of subgroup names to apply the lambda to
   *
   * These functions loop over sub-groups and executes a lambda that uses the sub-group as an
   * argument. The lambda is only executed if the group can be casted to a certain type specified
   * by the GROUPTYPE/S pack. The variadic list consisting of GROUPTYPE/S will be used recursively
   * to check if the group is able to be casted to the one of these types. The first type in the
   * GROUPTYPE/S list will be used to execute the lambda, and the next sub-group will be processed.
   */
  ///@{
  template< typename GROUPTYPE = Group, typename ... GROUPTYPES, typename LAMBDA >
  void forSubGroups( LAMBDA lambda )
  {
    for( auto & subGroupIter : m_subGroups )
    {
      applyLambdaToContainer< Group, GROUPTYPE, GROUPTYPES... >( subGroupIter.second, [&]( auto * const castedSubGroup )
          {
            lambda( castedSubGroup );
          } );
    }
  }

  template< typename GROUPTYPE = Group, typename ... GROUPTYPES, typename LAMBDA >
  void forSubGroups( LAMBDA lambda ) const
  {
    for( auto const & subGroupIter : m_subGroups )
    {
      applyLambdaToContainer< Group, GROUPTYPE, GROUPTYPES... >( subGroupIter.second, [&]( auto const * const castedSubGroup )
          {
            lambda( castedSubGroup );
          } );
    }
  }

  template< typename GROUPTYPE = Group, typename ... GROUPTYPES, typename LAMBDA >
  void forSubGroups( string_array const & subgroupNames, LAMBDA lambda )
  {
    for( string const & subgroupName : subgroupNames )
    {
      applyLambdaToContainer< Group, GROUPTYPE, GROUPTYPES... >( GetGroup( subgroupName ), [&]( auto * const castedSubGroup )
          {
            lambda( castedSubGroup );
          } );
    }
  }


  template< typename GROUPTYPE = Group, typename ... GROUPTYPES, typename LAMBDA >
  void forSubGroups( string_array const & subgroupNames, LAMBDA lambda ) const
  {
    for( string const & subgroupName : subgroupNames )
    {
      applyLambdaToContainer< Group, GROUPTYPE, GROUPTYPES... >( GetGroup( subgroupName ), [&]( auto const * const castedSubGroup )
          {
            lambda( castedSubGroup );
          } );
    }
  }
  ///@}

  /**
   * @name FUNCTION GROUP for forWrappers()
   * @brief These functions apply the specified lambda function to a Wrapper if the Wrapper can be
   *        casted to the templated type/s.
   * @tparam TYPE The first type that will be used in the attempted casting of Wrapper.
   * @tparam TYPES A variadic list of types that will be used in the attempted casting of Wrapper.
   * @tparam LAMBDA The type of lambda function to call in the function
   * @param[in] lambda The lambda function to call in the function
   *
   * These functions loop over the Wrappers contained in this group, and executes a lambda that
   * uses the Wrapper as an argument. The lambda is only executed if the Wrapper can be casted to
   * a certain type specified by the TYPE/S pack. The variadic list consisting of
   * TYPE/S will be used recursively to check if the Wrapper is able to be casted to the
   * one of these types. The first type in the WRAPPERTYPE/S list will be used to execute the
   * lambda, and the next Wrapper will be processed.
   */
  ///@{
  template< typename LAMBDA >
  void forWrappers( LAMBDA lambda )
  {
    for( auto & wrapperIter : m_wrappers )
    {
      lambda( *wrapperIter.second );
    }
  }

  template< typename LAMBDA >
  void forWrappers( LAMBDA lambda ) const
  {
    for( auto const & wrapperIter : m_wrappers )
    {
      lambda( *wrapperIter.second );
    }
  }

  template< typename TYPE, typename ... TYPES, typename LAMBDA >
  void forWrappers( LAMBDA lambda )
  {
    for( auto & wrapperIter : m_wrappers )
    {
      applyLambdaToContainer< WrapperBase, Wrapper< TYPE >, Wrapper< TYPES >... >( wrapperIter.second,
                                                                                   std::forward< LAMBDA >( lambda ));
    }
  }

  template< typename TYPE, typename ... TYPES, typename LAMBDA >
  void forWrappers( LAMBDA lambda ) const
  {
    for( auto const & wrapperIter : m_wrappers )
    {
      applyLambdaToContainer< WrapperBase, Wrapper< TYPE >, Wrapper< TYPES >... >( wrapperIter.second,
                                                                                   std::forward< LAMBDA >( lambda ));
    }
  }
  ///@}


  void Initialize( Group * const group );

  virtual void InitializationOrder( string_array & order );


  void InitializePostInitialConditions( Group * const group );

  template< typename T, typename TBASE=T >
  Wrapper< TBASE > *
  registerWrapper( std::string const & name,
                   wrapperMap::KeyIndex::index_type * const rkey = nullptr );

  template< typename T, typename TBASE=T >
  Wrapper< TBASE > *
  registerWrapper( Group::wrapperMap::KeyIndex & viewKey );


  WrapperBase * registerWrapper( std::string const & name,
                                 rtTypes::TypeIDs const & type );

  template< typename T >
  Wrapper< T > * registerWrapper( std::string const & name,
                                  std::unique_ptr< T > newObject );

  template< typename T >
  Wrapper< T > * registerWrapper( std::string const & name,
                                  T * newObject,
                                  bool takeOwnership );

  /**
   * @brief Register a Wrapper into this Group
   * @param[in] name the key name to use for this new wrapper
   * @param[in] wrapper a pointer to the new wrapper
   * @return a WrapperBase pointer that holds the address of the new wrapper
   */
  WrapperBase * registerWrapper( string const & name,
                                 WrapperBase * const wrapper );

  void deregisterWrapper( string const & name );


  void PrintDataHierarchy( integer indent = 0 );

  virtual Group * CreateChild( string const & childKey, string const & childName );

  /**
   * @brief Recursively read values using ProcessInputFile() from the input
   *        file and put them into the wrapped values for this group.
   * @param[in] targetNode the XML node that to extract input values from.
   */
  void ProcessInputFileRecursive( xmlWrapper::xmlNode & targetNode );

  /**
   * @brief Recursively call PostProcessInput() to apply post processing after
   * reading input values.
   */
  void PostProcessInputRecursive();

  /**
   * This function is used to build a complete datastructure for schema generation
   */
  void GenerateDataStructureSkeleton( integer const level )
  {
    ExpandObjectCatalogs();
    std::string indent( level*2, ' ' );

    for( auto const & subGroupIter : m_subGroups )
    {
      std::cout << indent << subGroupIter.second->getName() << std::endl;
      subGroupIter.second->GenerateDataStructureSkeleton( level + 1 );
    }
  }

  /**
   * This function is used to expand any catalogs in the data structure
   */
  virtual void ExpandObjectCatalogs() {}

  /**
   * This function is used to inform the schema generator of any
   * deviations between the xml and GEOS data structures.
   */
  virtual void SetSchemaDeviations( xmlWrapper::xmlNode GEOSX_UNUSED_ARG( schemaRoot ),
                                    xmlWrapper::xmlNode GEOSX_UNUSED_ARG( schemaParent ),
                                    integer GEOSX_UNUSED_ARG( documentationType ) ) {}

  virtual void RegisterDataOnMeshRecursive( Group * const MeshBodies );

  virtual void RegisterDataOnMesh( Group * const GEOSX_UNUSED_ARG( MeshBody ) ) {}

  virtual localIndex PackSize( string_array const & wrapperNames,
                               integer const recursive ) const;

  virtual localIndex PackSize( string_array const & wrapperNames,
                               arrayView1d< localIndex const > const & packList,
                               integer const recursive ) const;

  virtual localIndex Pack( buffer_unit_type * & buffer,
                           string_array const & wrapperNames,
                           integer const recursive ) const;

  virtual localIndex Pack( buffer_unit_type * & buffer,
                           string_array const & wrapperNames,
                           arrayView1d< localIndex const > const & packList,
                           integer const recursive ) const;

  virtual localIndex Unpack( buffer_unit_type const * & buffer,
                             arrayView1d< localIndex > & packList,
                             integer const recursive );


  //***********************************************************************************************

  WrapperBase const * getWrapperBase( indexType const index ) const
  { return m_wrappers[index]; }

  WrapperBase * getWrapperBase( indexType const index )
  { return m_wrappers[index]; }

  WrapperBase const * getWrapperBase( std::string const & name ) const
  { return m_wrappers[name]; }

  WrapperBase * getWrapperBase( std::string const & name )
  { return m_wrappers[name]; }

  WrapperBase const * getWrapperBase( wrapperMap::KeyIndex const & keyIndex ) const
  { return m_wrappers[keyIndex]; }

  WrapperBase * getWrapperBase( wrapperMap::KeyIndex const & keyIndex )
  { return m_wrappers[keyIndex]; }


  template< typename T, typename LOOKUP_TYPE >
  Wrapper< T > const * getWrapper( LOOKUP_TYPE const & index ) const
  {
#ifdef USE_DYNAMIC_CASTING
    return dynamic_cast< Wrapper< T > const * >( (m_wrappers[index]) );
#else
    return static_cast< Wrapper< T > const * >( (m_wrappers[index]) );
#endif
  }

  template< typename T, typename LOOKUP_TYPE >
  Wrapper< T > * getWrapper( LOOKUP_TYPE const & index )
  { return const_cast< Wrapper< T > * >( const_cast< Group const * >(this)->getWrapper< T >( index ) ); }

  template< typename T >
  Wrapper< T > const * getWrapper( char const * const key ) const
  { return getWrapper< T >( string( key ) ); }

  template< typename T >
  Wrapper< T > * getWrapper( char const * const key )
  { return getWrapper< T >( string( key ) ); }


  indexType getWrapperIndex( std::string const & name ) const
  {
    return m_wrappers.getIndex( name );
  }


  template< typename T, typename WRAPPEDTYPE=T, typename LOOKUP_TYPE >
  typename std::enable_if< std::is_same< T, WRAPPEDTYPE >::value, T const & >::type
  getReference( LOOKUP_TYPE const & lookup ) const
  {
    Wrapper< WRAPPEDTYPE > const * wrapper = getWrapper< WRAPPEDTYPE >( lookup );
    if( wrapper == nullptr )
    {
      if( hasView( lookup ) )
      {
        GEOS_ERROR( "call to getWrapper results in nullptr but a view exists. Most likely given the incorrect type. lookup : " << lookup );
      }
      GEOS_ERROR( "call to getWrapper results in nullptr and a view does not exist. lookup : " << lookup );
    }

    return wrapper->reference();
  }

  template< typename T, typename WRAPPEDTYPE=T, typename LOOKUP_TYPE >
  typename std::enable_if< !std::is_same< T, WRAPPEDTYPE >::value, T const & >::type
  getReference( LOOKUP_TYPE const & lookup ) const
  {
    static_assert( std::is_base_of< WRAPPEDTYPE, T >::value, "incorrect template arguments" );
    Wrapper< WRAPPEDTYPE > const * wrapper = getWrapper< WRAPPEDTYPE >( lookup );
    if( wrapper == nullptr )
    {
      if( hasView( lookup ) )
      {
        GEOS_ERROR( "call to getWrapper results in nullptr but a view exists. Most likely given the incorrect type. lookup : " << lookup );
      }
      GEOS_ERROR( "call to getWrapper results in nullptr and a view does not exist. lookup : " << lookup );
    }

#ifdef USE_DYNAMIC_CASTING
    return dynamic_cast< T const & >( wrapper->reference() );
#else
    return static_cast< T const & >( wrapper->reference() );
#endif
  }


  template< typename T, typename WRAPPEDTYPE=T, typename LOOKUP_TYPE >
  T & getReference( LOOKUP_TYPE const & lookup )
  { return const_cast< T & >( const_cast< const Group * >(this)->template getReference< T, WRAPPEDTYPE, LOOKUP_TYPE >( lookup ) ); }

  template< typename T, typename WRAPPEDTYPE=T >
  T const & getReference( char const * const name ) const
  { return getReference< T, WRAPPEDTYPE >( string( name ) ); }

  template< typename T, typename WRAPPEDTYPE=T >
  T & getReference( char const * const name )
  { return const_cast< T & >( const_cast< const Group * >(this)->getReference< T, WRAPPEDTYPE >( name ) ); }



  template< typename T, typename LOOKUP_TYPE >
  T const * getPointer( LOOKUP_TYPE const & lookup ) const
  {
    T const * rval = nullptr;
    Wrapper< T > const * wrapper = getWrapper< T >( lookup );
    if( wrapper != nullptr )
    {
      rval = wrapper->getPointer();
    }
    return rval;
  }

  template< typename T, typename LOOKUP_TYPE >
  T * getPointer( LOOKUP_TYPE const & lookup )
  { return const_cast< T * >( const_cast< Group const * >(this)->getPointer< T >( lookup )); }

  template< typename T >
  T const * getPointer( char const * const name ) const
  { return getPointer< T >( string( name ) ); }

  template< typename T >
  T * getPointer( char const * const name )
  { return getPointer< T >( string( name ) ); }


  bool hasGroup( std::string const & name ) const
  {
    return (m_subGroups[name] != nullptr);
  }

  template< typename LOOKUP_TYPE >
  bool hasView( LOOKUP_TYPE const & lookup ) const
  {
    return (m_wrappers[lookup] != nullptr);
  }

  inline const string getName() const
  {
    return m_name;
  }


  virtual void resize( localIndex const newsize );

  virtual void reserve( indexType const newsize );


  inline localIndex capacity() const
  {
    return m_capacity;
  }

  inline localIndex size() const
  {
    return m_size;
  }

  axom::sidre::Group * getSidreGroup()
  {
#ifdef GEOSX_USE_ATK
    return m_sidreGroup;
#else
    return nullptr;
#endif
  }

  axom::sidre::Group const * getSidreGroup() const
  {
#ifdef GEOSX_USE_ATK
    return m_sidreGroup;
#else
    return nullptr;
#endif
  }

  static axom::sidre::Group * setSidreGroup( string const & name,
                                             Group * const parent );

  Group * getParent()             { return m_parent; }
  Group const * getParent() const { return m_parent; }

  Group * setParent( Group * const parent )
  {
    m_parent = parent;
#ifdef GEOSX_USE_ATK
    m_sidreGroup = m_parent->getSidreGroup();
#endif

    return m_parent;
  }

  localIndex getIndexInParent() const
  {
    return m_parent->GetSubGroups().getIndex( this->m_name );
  }


  wrapperMap const & wrappers() const
  {
    return m_wrappers;
  }

  wrapperMap & wrappers()
  {
    return m_wrappers;
  }

  RestartFlags getRestartFlags() const { return m_restart_flags; }

  void setRestartFlags( RestartFlags flags ) { m_restart_flags = flags; }

  InputFlags getInputFlags() const { return m_input_flags; }

  void setInputFlags( InputFlags flags ) { m_input_flags = flags; }

  void prepareToWrite();

  void finishWriting() const;

  void prepareToRead();

  void finishReading();

  void postRestartInitializationRecursive( Group * const domain );


protected:
  /**
   * @brief Post processing of the input values.
   */
  virtual void PostProcessInput() {}

  virtual void InitializePreSubGroups( Group * const GEOSX_UNUSED_ARG( group ) ) {}

  virtual void InitializePostSubGroups( Group * const GEOSX_UNUSED_ARG( group ) ) {}

  virtual void InitializePostInitialConditions_PreSubGroups( Group * const GEOSX_UNUSED_ARG( group ) ) {}

  virtual void InitializePostInitialConditions_PostSubGroups( Group * const GEOSX_UNUSED_ARG( group ) ) {}

  virtual void postRestartInitialization( Group * const GEOSX_UNUSED_ARG( domain ) ) {}

private:
  /**
   * @brief Read values from the input file and put them into the
   *        wrapped values for this group.
   * @param[in] targetNode the XML node that to extract input values from.
   */
  virtual void ProcessInputFile( xmlWrapper::xmlNode const & targetNode );

  /// the parent of this group
  Group * m_parent = nullptr;

  /// the container for all wrappers
  wrapperMap m_wrappers;

  /// The container for all sub-groups
  subGroupMap m_subGroups;

#ifdef GEOSX_USE_ATK
  /// Pointer to the sidre group that mirrors this group
  axom::sidre::Group * m_sidreGroup;
#endif

  indexType m_size;             ///< The size/length wrappers in this group
  indexType m_capacity;         ///< The capacity for wrappers in this group
  RestartFlags m_restart_flags; ///< Restart flag for this group...and
                                ///< subsequently all wrappers in this group
  InputFlags m_input_flags;     ///< Input flag for this group
  string m_name;                ///< the repository name of this group. This
                                ///< is the key in the parent group.

};

using GroupKey = Group::subGroupMap::KeyIndex;
using ViewKey = Group::wrapperMap::KeyIndex;



template< typename T >
T * Group::RegisterGroup( std::string const & name,
                          std::unique_ptr< Group > newObject )
{
#ifdef USE_DYNAMIC_CASTING
  return dynamic_cast< T * >( m_subGroups.insert( name, newObject.release(), true ) );
#else
  return static_cast< T * >( m_subGroups.insert( name, newObject.release(), true ) );
#endif
}


template< typename T >
T * Group::RegisterGroup( std::string const & name,
                          T * newObject,
                          bool const takeOwnership )
{
#ifdef USE_DYNAMIC_CASTING
  return dynamic_cast< T * >( m_subGroups.insert( name, newObject, takeOwnership ) );
#else
  return static_cast< T * >( m_subGroups.insert( name, newObject, takeOwnership ) );
#endif
}


template< typename T, typename TBASE >
Wrapper< TBASE > * Group::registerWrapper( std::string const & name,
                                           ViewKey::index_type * const rkey )
{
  m_wrappers.insert( name,
                     (Wrapper< TBASE >::template Factory< T >( name, this ) ).release(),
                     true );

  if( rkey != nullptr )
  {
    *rkey = m_wrappers.getIndex( name );
  }
  Wrapper< TBASE > * const rval = getWrapper< TBASE >( name );
  if( rval->sizedFromParent() == 1 && rval->shouldResize())
  {
    rval->resize( this->size());
  }
  return rval;
}

template< typename T, typename TBASE >
Wrapper< TBASE > * Group::registerWrapper( ViewKey & viewKey )
{
  ViewKey::index_type index;
  Wrapper< TBASE > * const rval = registerWrapper< T, TBASE >( viewKey.Key(), &index );
  viewKey.setIndex( index );

  return rval;
}


template< typename T >
Wrapper< T > * Group::registerWrapper( std::string const & name,
                                       std::unique_ptr< T > newObject )
{
  m_wrappers.insert( name,
                     new Wrapper< T >( name, this, newObject.release(), true ),
                     true );

  Wrapper< T > * const rval = getWrapper< T >( name );
  if( rval->sizedFromParent() == 1 )
  {
    rval->resize( this->size());
  }
  return rval;
}



template< typename T >
Wrapper< T > * Group::registerWrapper( std::string const & name,
                                       T * newObject,
                                       bool takeOwnership )
{
  m_wrappers.insert( name,
                     new Wrapper< T >( name, this, newObject, takeOwnership ),
                     true );

  Wrapper< T > * const rval = getWrapper< T >( name );
  if( rval->sizedFromParent() == 1 && rval->shouldResize())
  {
    rval->resize( this->size());
  }
  return rval;
}


} /* end namespace dataRepository */
} /* end namespace geosx */

#endif /* GEOSX_DATAREPOSITORY_GROUP_HPP_ */
