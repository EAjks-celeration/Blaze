//=================================================================================================
/*!
//  \file blaze/math/expressions/SMatSMatMultExpr.h
//  \brief Header file for the sparse matrix/sparse matrix multiplication expression
//
//  Copyright (C) 2013 Klaus Iglberger - All Rights Reserved
//
//  This file is part of the Blaze library. You can redistribute it and/or modify it under
//  the terms of the New (Revised) BSD License. Redistribution and use in source and binary
//  forms, with or without modification, are permitted provided that the following conditions
//  are met:
//
//  1. Redistributions of source code must retain the above copyright notice, this list of
//     conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice, this list
//     of conditions and the following disclaimer in the documentation and/or other materials
//     provided with the distribution.
//  3. Neither the names of the Blaze development group nor the names of its contributors
//     may be used to endorse or promote products derived from this software without specific
//     prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
//  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
//  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
//  SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
//  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
//  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
//  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
//  DAMAGE.
*/
//=================================================================================================

#ifndef _BLAZE_MATH_EXPRESSIONS_SMATSMATMULTEXPR_H_
#define _BLAZE_MATH_EXPRESSIONS_SMATSMATMULTEXPR_H_


//*************************************************************************************************
// Includes
//*************************************************************************************************

#include <algorithm>
#include <stdexcept>
#include <vector>
#include <blaze/math/constraints/MatMatMultExpr.h>
#include <blaze/math/constraints/SparseMatrix.h>
#include <blaze/math/constraints/StorageOrder.h>
#include <blaze/math/constraints/Symmetric.h>
#include <blaze/math/dense/DynamicVector.h>
#include <blaze/math/expressions/Computation.h>
#include <blaze/math/expressions/Forward.h>
#include <blaze/math/expressions/MatMatMultExpr.h>
#include <blaze/math/expressions/SparseMatrix.h>
#include <blaze/math/Infinity.h>
#include <blaze/math/shims/IsDefault.h>
#include <blaze/math/shims/Reset.h>
#include <blaze/math/shims/Serial.h>
#include <blaze/math/traits/ColumnExprTrait.h>
#include <blaze/math/traits/MultExprTrait.h>
#include <blaze/math/traits/MultTrait.h>
#include <blaze/math/traits/RowExprTrait.h>
#include <blaze/math/traits/SMatDVecMultExprTrait.h>
#include <blaze/math/traits/SMatSVecMultExprTrait.h>
#include <blaze/math/traits/SubmatrixExprTrait.h>
#include <blaze/math/traits/TDVecSMatMultExprTrait.h>
#include <blaze/math/traits/TSVecSMatMultExprTrait.h>
#include <blaze/math/typetraits/Columns.h>
#include <blaze/math/typetraits/IsColumnVector.h>
#include <blaze/math/typetraits/IsComputation.h>
#include <blaze/math/typetraits/IsDenseVector.h>
#include <blaze/math/typetraits/IsExpression.h>
#include <blaze/math/typetraits/IsLower.h>
#include <blaze/math/typetraits/IsResizable.h>
#include <blaze/math/typetraits/IsRowMajorMatrix.h>
#include <blaze/math/typetraits/IsRowVector.h>
#include <blaze/math/typetraits/IsSparseMatrix.h>
#include <blaze/math/typetraits/IsSparseVector.h>
#include <blaze/math/typetraits/IsUpper.h>
#include <blaze/math/typetraits/RequiresEvaluation.h>
#include <blaze/math/typetraits/Rows.h>
#include <blaze/system/Thresholds.h>
#include <blaze/util/Assert.h>
#include <blaze/util/Byte.h>
#include <blaze/util/EnableIf.h>
#include <blaze/util/InvalidType.h>
#include <blaze/util/logging/FunctionTrace.h>
#include <blaze/util/SelectType.h>
#include <blaze/util/Types.h>
#include <blaze/util/typetraits/RemoveReference.h>
#include <blaze/util/Unused.h>
#include <blaze/util/valuetraits/IsTrue.h>


namespace blaze {

//=================================================================================================
//
//  CLASS SMATSMATMULTEXPR
//
//=================================================================================================

//*************************************************************************************************
/*!\brief Expression object for sparse matrix-sparse matrix multiplications.
// \ingroup sparse_matrix_expression
//
// The SMatSMatMultExpr class represents the compile time expression for multiplications between
// row-major sparse matrices.
*/
template< typename MT1    // Type of the left-hand side sparse matrix
        , typename MT2 >  // Type of the right-hand side sparse matrix
class SMatSMatMultExpr : public SparseMatrix< SMatSMatMultExpr<MT1,MT2>, false >
                       , private MatMatMultExpr
                       , private Computation
{
 private:
   //**Type definitions****************************************************************************
   typedef typename MT1::ResultType     RT1;  //!< Result type of the left-hand side sparse matrix expression.
   typedef typename MT2::ResultType     RT2;  //!< Result type of the right-hand side sparse matrix expression.
   typedef typename MT1::CompositeType  CT1;  //!< Composite type of the left-hand side sparse matrix expression.
   typedef typename MT2::CompositeType  CT2;  //!< Composite type of the right-hand side sparse matrix expression.
   //**********************************************************************************************

   //**********************************************************************************************
   //! Compilation switch for the composite type of the left-hand side sparse matrix expression.
   enum { evaluateLeft = RequiresEvaluation<MT1>::value };
   //**********************************************************************************************

   //**********************************************************************************************
   //! Compilation switch for the composite type of the right-hand side sparse matrix expression.
   enum { evaluateRight = RequiresEvaluation<MT2>::value };
   //**********************************************************************************************

   //**********************************************************************************************
   /*! \cond BLAZE_INTERNAL */
   //! Helper structure for the explicit application of the SFINAE principle.
   /*! The UseSMPAssign struct is a helper struct for the selection of the parallel evaluation
       strategy. In case either of the two matrix operands requires an intermediate evaluation,
       the nested \value will be set to 1, otherwise it will be 0. */
   template< typename MT >
   struct UseSMPAssign {
      enum { value = ( evaluateLeft || evaluateRight ) };
   };
   /*! \endcond */
   //**********************************************************************************************

 public:
   //**Type definitions****************************************************************************
   typedef SMatSMatMultExpr<MT1,MT2>           This;           //!< Type of this SMatSMatMultExpr instance.
   typedef typename MultTrait<RT1,RT2>::Type   ResultType;     //!< Result type for expression template evaluations.
   typedef typename ResultType::OppositeType   OppositeType;   //!< Result type with opposite storage order for expression template evaluations.
   typedef typename ResultType::TransposeType  TransposeType;  //!< Transpose type for expression template evaluations.
   typedef typename ResultType::ElementType    ElementType;    //!< Resulting element type.
   typedef const ElementType                   ReturnType;     //!< Return type for expression template evaluations.
   typedef const ResultType                    CompositeType;  //!< Data type for composite expression templates.

   //! Composite type of the left-hand side sparse matrix expression.
   typedef typename SelectType< IsExpression<MT1>::value, const MT1, const MT1& >::Type  LeftOperand;

   //! Composite type of the right-hand side sparse matrix expression.
   typedef typename SelectType< IsExpression<MT2>::value, const MT2, const MT2& >::Type  RightOperand;
   //**********************************************************************************************

   //**Compilation flags***************************************************************************
   //! Compilation switch for the expression template assignment strategy.
   enum { smpAssignable = !evaluateLeft  && MT1::smpAssignable &&
                          !evaluateRight && MT2::smpAssignable };
   //**********************************************************************************************

   //**Constructor*********************************************************************************
   /*!\brief Constructor for the SMatSMatMultExpr class.
   //
   // \param lhs The left-hand side sparse matrix operand of the multiplication expression.
   // \param rhs The right-hand side sparse matrix operand of the multiplication expression.
   */
   explicit inline SMatSMatMultExpr( const MT1& lhs, const MT2& rhs )
      : lhs_( lhs )  // Left-hand side sparse matrix of the multiplication expression
      , rhs_( rhs )  // Right-hand side sparse matrix of the multiplication expression
   {
      BLAZE_INTERNAL_ASSERT( lhs.columns() == rhs.rows(), "Invalid matrix sizes" );
   }
   //**********************************************************************************************

   //**Access operator*****************************************************************************
   /*!\brief 2D-access to the matrix elements.
   //
   // \param i Access index for the row. The index has to be in the range \f$[0..M-1]\f$.
   // \param j Access index for the column. The index has to be in the range \f$[0..N-1]\f$.
   // \return The resulting value.
   */
   inline ReturnType operator()( size_t i, size_t j ) const {
      BLAZE_INTERNAL_ASSERT( i < lhs_.rows()   , "Invalid row access index"    );
      BLAZE_INTERNAL_ASSERT( j < rhs_.columns(), "Invalid column access index" );

      typedef typename RemoveReference<CT1>::Type::ConstIterator  ConstIterator;

      ElementType tmp = ElementType();

      // Early exit
      if( lhs_.columns() == 0UL )
         return tmp;

      // Fast computation in case the left-hand side sparse matrix directly provides iterators
      if( !RequiresEvaluation<MT1>::value )
      {
         // Evaluation of the left-hand side sparse matrix operand
         CT1 A( lhs_ );

         const ConstIterator end( A.end(i) );
         ConstIterator element( A.begin(i) );

         // Early exit in case row i is empty
         if( element == end )
            return tmp;

         // Calculating element (i,j)
         tmp = element->value() * rhs_(element->index(),j);
         ++element;
         for( ; element!=end; ++element )
            tmp += element->value() * rhs_(element->index(),j);
      }

      // Default computation in case the left-hand side sparse matrix doesn't provide iterators
      else {
         tmp = lhs_(i,0UL) * rhs_(0UL,j);
         for( size_t k=1UL; k<lhs_.columns(); ++k ) {
            tmp += lhs_(i,k) * rhs_(k,j);
         }
      }

      return tmp;
   }
   //**********************************************************************************************

   //**Rows function*******************************************************************************
   /*!\brief Returns the current number of rows of the matrix.
   //
   // \return The number of rows of the matrix.
   */
   inline size_t rows() const {
      return lhs_.rows();
   }
   //**********************************************************************************************

   //**Columns function****************************************************************************
   /*!\brief Returns the current number of columns of the matrix.
   //
   // \return The number of columns of the matrix.
   */
   inline size_t columns() const {
      return rhs_.columns();
   }
   //**********************************************************************************************

   //**NonZeros function***************************************************************************
   /*!\brief Returns the number of non-zero elements in the sparse matrix.
   //
   // \return The number of non-zero elements in the sparse matrix.
   */
   inline size_t nonZeros() const {
      return 0UL;
   }
   //**********************************************************************************************

   //**NonZeros function***************************************************************************
   /*!\brief Returns the number of non-zero elements in the specified row.
   //
   // \param i The index of the row.
   // \return The number of non-zero elements of row \a i.
   */
   inline size_t nonZeros( size_t i ) const {
      UNUSED_PARAMETER( i );
      return 0UL;
   }
   //**********************************************************************************************

   //**Left operand access*************************************************************************
   /*!\brief Returns the left-hand side sparse matrix operand.
   //
   // \return The left-hand side sparse matrix operand.
   */
   inline LeftOperand leftOperand() const {
      return lhs_;
   }
   //**********************************************************************************************

   //**Right operand access************************************************************************
   /*!\brief Returns the right-hand side sparse matrix operand.
   //
   // \return The right-hand side sparse matrix operand.
   */
   inline RightOperand rightOperand() const {
      return rhs_;
   }
   //**********************************************************************************************

   //**********************************************************************************************
   /*!\brief Returns whether the expression can alias with the given address \a alias.
   //
   // \param alias The alias to be checked.
   // \return \a true in case an alias effect is detected, \a false otherwise.
   */
   template< typename T >
   inline bool canAlias( const T* alias ) const {
      return ( lhs_.isAliased( alias ) || rhs_.isAliased( alias ) );
   }
   //**********************************************************************************************

   //**********************************************************************************************
   /*!\brief Returns whether the expression is aliased with the given address \a alias.
   //
   // \param alias The alias to be checked.
   // \return \a true in case an alias effect is detected, \a false otherwise.
   */
   template< typename T >
   inline bool isAliased( const T* alias ) const {
      return ( lhs_.isAliased( alias ) || rhs_.isAliased( alias ) );
   }
   //**********************************************************************************************

   //**********************************************************************************************
   /*!\brief Returns whether the expression can be used in SMP assignments.
   //
   // \return \a true in case the expression can be used in SMP assignments, \a false if not.
   */
   inline bool canSMPAssign() const {
      return ( rows() > SMP_SMATSMATMULT_THRESHOLD );
   }
   //**********************************************************************************************

 private:
   //**Member variables****************************************************************************
   LeftOperand  lhs_;  //!< Left-hand side sparse matrix of the multiplication expression.
   RightOperand rhs_;  //!< Right-hand side sparse matrix of the multiplication expression.
   //**********************************************************************************************

   //**Assignment to dense matrices****************************************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Assignment of a sparse matrix-sparse matrix multiplication to a dense matrix
   //        (\f$ C=A*B \f$).
   // \ingroup sparse_matrix
   //
   // \param lhs The target left-hand side dense matrix.
   // \param rhs The right-hand side multiplication expression to be assigned.
   // \return void
   //
   // This function implements the performance optimized assignment of a sparse matrix-sparse
   // matrix multiplication expression to a dense matrix.
   */
   template< typename MT  // Type of the target dense matrix
           , bool SO >    // Storage order of the target dense matrix
   friend inline void assign( DenseMatrix<MT,SO>& lhs, const SMatSMatMultExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == rhs.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( (~lhs).columns() == rhs.columns(), "Invalid number of columns" );

      CT1 A( serial( rhs.lhs_ ) );  // Evaluation of the left-hand side sparse matrix operand
      CT2 B( serial( rhs.rhs_ ) );  // Evaluation of the right-hand side sparse matrix operand

      BLAZE_INTERNAL_ASSERT( A.rows()    == rhs.lhs_.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( A.columns() == rhs.lhs_.columns(), "Invalid number of columns" );
      BLAZE_INTERNAL_ASSERT( B.rows()    == rhs.rhs_.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( B.columns() == rhs.rhs_.columns(), "Invalid number of columns" );
      BLAZE_INTERNAL_ASSERT( A.rows()    == (~lhs).rows()     , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( B.columns() == (~lhs).columns()  , "Invalid number of columns" );

      SMatSMatMultExpr::selectAssignKernel( ~lhs, A, B );
   }
   /*! \endcond */
   //**********************************************************************************************

   //**Default assignment to dense matrices********************************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Default assignment of a sparse matrix-sparse matrix multiplication (\f$ C=A*B \f$).
   // \ingroup sparse_matrix
   //
   // \param C The target left-hand side dense matrix.
   // \param A The left-hand side sparse matrix operand.
   // \param B The right-hand side sparse matrix operand.
   // \return void
   //
   // This function implements the default assignment kernel for the sparse matrix-sparse matrix
   // multiplication.
   */
   template< typename MT3    // Type of the left-hand side target matrix
           , typename MT4    // Type of the left-hand side matrix operand
           , typename MT5 >  // Type of the right-hand side matrix operand
   static inline void selectAssignKernel( MT3& C, const MT4& A, const MT5& B )
   {
      typedef typename MT4::ConstIterator  LeftIterator;
      typedef typename MT5::ConstIterator  RightIterator;

      for( size_t i=0UL; i<C.rows(); ++i ) {
         const LeftIterator lend( A.end(i) );
         for( LeftIterator lelem=A.begin(i); lelem!=lend; ++lelem ) {
            const RightIterator rend( B.end( lelem->index() ) );
            for( RightIterator relem=B.begin( lelem->index() ); relem!=rend; ++relem )
            {
               if( IsResizable<typename MT3::ElementType>::value &&
                   isDefault( C(i,relem->index()) ) ) {
                  C(i,relem->index()) = lelem->value() * relem->value();
               }
               else {
                  C(i,relem->index()) += lelem->value() * relem->value();
               }
            }
         }
      }
   }
   /*! \endcond */
   //**********************************************************************************************

   //**Assignment to row-major sparse matrices*****************************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Assignment of a sparse matrix-sparse matrix multiplication to a row-major sparse
   //        matrix (\f$ C=A*B \f$).
   // \ingroup sparse_matrix
   //
   // \param lhs The target left-hand side sparse matrix.
   // \param rhs The right-hand side multiplication expression to be assigned.
   // \return void
   //
   // This function implements the performance optimized assignment of a sparse matrix-sparse
   // matrix multiplication expression to a row-major sparse matrix.
   */
   template< typename MT >  // Type of the target sparse matrix
   friend inline void assign( SparseMatrix<MT,false>& lhs, const SMatSMatMultExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == rhs.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( (~lhs).columns() == rhs.columns(), "Invalid number of columns" );

      typedef typename RemoveReference<CT1>::Type::ConstIterator  LeftIterator;
      typedef typename RemoveReference<CT2>::Type::ConstIterator  RightIterator;

      CT1 A( serial( rhs.lhs_ ) );  // Evaluation of the left-hand side sparse matrix operand
      CT2 B( serial( rhs.rhs_ ) );  // Evaluation of the right-hand side sparse matrix operand

      BLAZE_INTERNAL_ASSERT( A.rows()    == rhs.lhs_.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( A.columns() == rhs.lhs_.columns(), "Invalid number of columns" );
      BLAZE_INTERNAL_ASSERT( B.rows()    == rhs.rhs_.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( B.columns() == rhs.rhs_.columns(), "Invalid number of columns" );
      BLAZE_INTERNAL_ASSERT( A.rows()    == (~lhs).rows()     , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( B.columns() == (~lhs).columns()  , "Invalid number of columns" );

      // (Over-)Estimating the number of non-zero entries in the resulting matrix
      size_t nonzeros( 0UL );

      for( size_t i=0UL; i<(~lhs).rows(); ++i ) {
         const LeftIterator lend( A.end(i) );
         for( LeftIterator lelem=A.begin(i); lelem!=lend; ++lelem ) {
            nonzeros += B.nonZeros( lelem->index() );
         }
      }

      if( nonzeros > (~lhs).rows() * (~lhs).columns() ) {
         nonzeros = (~lhs).rows() * (~lhs).columns();
      }

      (~lhs).reserve( nonzeros );
      nonzeros = 0UL;

      // Performing the matrix-matrix multiplication
      std::vector<ElementType> values ( (~lhs).columns(), ElementType() );
      std::vector<byte>        valid  ( (~lhs).columns(), 0   );
      std::vector<size_t>      indices( (~lhs).columns(), 0UL );
      size_t minIndex( inf ), maxIndex( 0UL );

      for( size_t i=0UL; i<(~lhs).rows(); ++i )
      {
         const LeftIterator lend( A.end(i) );
         for( LeftIterator lelem=A.begin(i); lelem!=lend; ++lelem )
         {
            const RightIterator rend( B.end( lelem->index() ) );
            for( RightIterator relem=B.begin( lelem->index() ); relem!=rend; ++relem )
            {
               if( !valid[relem->index()] ) {
                  values[relem->index()] = lelem->value() * relem->value();
                  valid [relem->index()] = 1;
                  indices[nonzeros] = relem->index();
                  ++nonzeros;
                  if( relem->index() < minIndex ) minIndex = relem->index();
                  if( relem->index() > maxIndex ) maxIndex = relem->index();
               }
               else {
                  values[relem->index()] += lelem->value() * relem->value();
               }
            }
         }

         BLAZE_INTERNAL_ASSERT( nonzeros <= (~lhs).columns(), "Invalid number of non-zero elements" );

         if( nonzeros > 0UL )
         {
            BLAZE_INTERNAL_ASSERT( minIndex <= maxIndex, "Invalid index detected" );

            if( ( nonzeros + nonzeros ) < ( maxIndex - minIndex ) )
            {
               std::sort( indices.begin(), indices.begin() + nonzeros );

               for( size_t j=0UL; j<nonzeros; ++j )
               {
                  const size_t index( indices[j] );
                  if( !isDefault( values[index] ) ) {
                     (~lhs).append( i, index, values[index] );
                     reset( values[index] );
                  }

                  reset( valid [index] );
               }
            }
            else {
               for( size_t j=minIndex; j<=maxIndex; ++j )
               {
                  if( !isDefault( values[j] ) ) {
                     (~lhs).append( i, j, values[j] );
                     reset( values[j] );
                  }

                  reset( valid [j] );
               }
            }

            nonzeros = 0UL;
            minIndex = inf;
            maxIndex = 0UL;
         }

         (~lhs).finalize( i );
      }
   }
   /*! \endcond */
   //**********************************************************************************************

   //**Assignment to column-major sparse matrices**************************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Assignment of a sparse matrix-sparse matrix multiplication to a column-major sparse
   //        matrix (\f$ C=A*B \f$).
   // \ingroup sparse_matrix
   //
   // \param lhs The target left-hand side sparse matrix.
   // \param rhs The right-hand side multiplication expression to be assigned.
   // \return void
   //
   // This function implements the performance optimized assignment of a sparse matrix-sparse
   // matrix multiplication expression to a column-major sparse matrix.
   */
   template< typename MT >  // Type of the target sparse matrix
   friend inline void assign( SparseMatrix<MT,true>& lhs, const SMatSMatMultExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_CONSTRAINT_MUST_NOT_BE_SYMMETRIC_MATRIX_TYPE( MT );

      BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == rhs.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( (~lhs).columns() == rhs.columns(), "Invalid number of columns" );

      BLAZE_CONSTRAINT_MUST_BE_SPARSE_MATRIX_TYPE   ( ResultType );
      BLAZE_CONSTRAINT_MUST_BE_ROW_MAJOR_MATRIX_TYPE( ResultType );

      const ResultType tmp( serial( rhs ) );
      (~lhs).reserve( tmp.nonZeros() );
      assign( ~lhs, tmp );
   }
   /*! \endcond */
   //**********************************************************************************************

   //**Addition assignment to dense matrices*******************************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Addition assignment of a sparse matrix-sparse matrix multiplication to a dense matrix
   //        (\f$ C+=A*B \f$).
   // \ingroup sparse_matrix
   //
   // \param lhs The target left-hand side dense matrix.
   // \param rhs The right-hand side multiplication expression to be added.
   // \return void
   //
   // This function implements the performance optimized addition assignment of a sparse matrix-
   // sparse matrix multiplication expression to a dense matrix.
   */
   template< typename MT  // Type of the target dense matrix
           , bool SO >    // Storage order of the target dense matarix
   friend inline void addAssign( DenseMatrix<MT,SO>& lhs, const SMatSMatMultExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == rhs.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( (~lhs).columns() == rhs.columns(), "Invalid number of columns" );

      CT1 A( serial( rhs.lhs_ ) );  // Evaluation of the left-hand side sparse matrix operand
      CT2 B( serial( rhs.rhs_ ) );  // Evaluation of the right-hand side sparse matrix operand

      BLAZE_INTERNAL_ASSERT( A.rows()    == rhs.lhs_.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( A.columns() == rhs.lhs_.columns(), "Invalid number of columns" );
      BLAZE_INTERNAL_ASSERT( B.rows()    == rhs.rhs_.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( B.columns() == rhs.rhs_.columns(), "Invalid number of columns" );
      BLAZE_INTERNAL_ASSERT( A.rows()    == (~lhs).rows()     , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( B.columns() == (~lhs).columns()  , "Invalid number of columns" );

      SMatSMatMultExpr::selectAddAssignKernel( ~lhs, A, B );
   }
   /*! \endcond */
   //**********************************************************************************************

   //**Default addition assignment to dense matrices***********************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Default addition assignment of a sparse matrix-sparse matrix multiplication
   //        (\f$ C+=A*B \f$).
   // \ingroup sparse_matrix
   //
   // \param C The target left-hand side dense matrix.
   // \param A The left-hand side sparse matrix operand.
   // \param B The right-hand side sparse matrix operand.
   // \return void
   //
   // This function implements the default addition assignment kernel for the sparse matrix-sparse
   // matrix multiplication.
   */
   template< typename MT3    // Type of the left-hand side target matrix
           , typename MT4    // Type of the left-hand side matrix operand
           , typename MT5 >  // Type of the right-hand side matrix operand
   static inline void selectAddAssignKernel( MT3& C, const MT4& A, const MT5& B )
   {
      typedef typename MT4::ConstIterator  LeftIterator;
      typedef typename MT5::ConstIterator  RightIterator;

      for( size_t i=0UL; i<C.rows(); ++i ) {
         const LeftIterator lend( A.end(i) );
         for( LeftIterator lelem=A.begin(i); lelem!=lend; ++lelem ) {
            const RightIterator rend( B.end( lelem->index() ) );
            for( RightIterator relem=B.begin( lelem->index() ); relem!=rend; ++relem ) {
               C(i,relem->index()) += lelem->value() * relem->value();
            }
         }
      }
   }
   /*! \endcond */
   //**********************************************************************************************

   //**Addition assignment to sparse matrices******************************************************
   // No special implementation for the addition assignment to sparse matrices.
   //**********************************************************************************************

   //**Subtraction assignment to dense matrices****************************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Subtraction assignment of a sparse matrix-sparse matrix multiplication to a
   //        dense matrix (\f$ C-=A*B \f$).
   // \ingroup sparse_matrix
   //
   // \param lhs The target left-hand side dense matrix.
   // \param rhs The right-hand side multiplication expression to be subtracted.
   // \return void
   //
   // This function implements the performance optimized subtraction assignment of a sparse matrix-
   // sparse matrix multiplication expression to a dense matrix.
   */
   template< typename MT  // Type of the target dense matrix
           , bool SO >    // Storage order of the target dense matrix
   friend inline void subAssign( DenseMatrix<MT,SO>& lhs, const SMatSMatMultExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == rhs.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( (~lhs).columns() == rhs.columns(), "Invalid number of columns" );

      CT1 A( serial( rhs.lhs_ ) );  // Evaluation of the left-hand side sparse matrix operand
      CT2 B( serial( rhs.rhs_ ) );  // Evaluation of the right-hand side sparse matrix operand

      BLAZE_INTERNAL_ASSERT( A.rows()    == rhs.lhs_.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( A.columns() == rhs.lhs_.columns(), "Invalid number of columns" );
      BLAZE_INTERNAL_ASSERT( B.rows()    == rhs.rhs_.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( B.columns() == rhs.rhs_.columns(), "Invalid number of columns" );
      BLAZE_INTERNAL_ASSERT( A.rows()    == (~lhs).rows()     , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( B.columns() == (~lhs).columns()  , "Invalid number of columns" );

      SMatSMatMultExpr::selectSubAssignKernel( ~lhs, A, B );
   }
   /*! \endcond */
   //**********************************************************************************************

   //**Default subtraction assignment to dense matrices********************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief Default subtraction assignment of a sparse matrix-sparse matrix multiplication
   //        (\f$ C-=A*B \f$).
   // \ingroup sparse_matrix
   //
   // \param C The target left-hand side dense matrix.
   // \param A The left-hand side sparse matrix operand.
   // \param B The right-hand side sparse matrix operand.
   // \return void
   //
   // This function implements the default subtraction assignment kernel for the sparse matrix-
   // sparse matrix multiplication.
   */
   template< typename MT3    // Type of the left-hand side target matrix
           , typename MT4    // Type of the left-hand side matrix operand
           , typename MT5 >  // Type of the right-hand side matrix operand
   static inline void selectSubAssignKernel( MT3& C, const MT4& A, const MT5& B )
   {
      typedef typename MT4::ConstIterator  LeftIterator;
      typedef typename MT5::ConstIterator  RightIterator;

      for( size_t i=0UL; i<C.rows(); ++i ) {
         const LeftIterator lend( A.end(i) );
         for( LeftIterator lelem=A.begin(i); lelem!=lend; ++lelem ) {
            const RightIterator rend( B.end( lelem->index() ) );
            for( RightIterator relem=B.begin( lelem->index() ); relem!=rend; ++relem ) {
               C(i,relem->index()) -= lelem->value() * relem->value();
            }
         }
      }
   }
   /*! \endcond */
   //**********************************************************************************************

   //**Subtraction assignment to sparse matrices***************************************************
   // No special implementation for the subtraction assignment to sparse matrices.
   //**********************************************************************************************

   //**Multiplication assignment to dense matrices*************************************************
   // No special implementation for the multiplication assignment to dense matrices.
   //**********************************************************************************************

   //**Multiplication assignment to sparse matrices************************************************
   // No special implementation for the multiplication assignment to sparse matrices.
   //**********************************************************************************************

   //**SMP assignment to dense matrices************************************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief SMP assignment of a sparse matrix-sparse matrix multiplication to a dense matrix
   //        (\f$ C=A*B \f$).
   // \ingroup sparse_matrix
   //
   // \param lhs The target left-hand side dense matrix.
   // \param rhs The right-hand side multiplication expression to be assigned.
   // \return void
   //
   // This function implements the performance optimized SMP assignment of a sparse matrix-sparse
   // matrix multiplication expression to a dense matrix. Due to the explicit application of the
   // SFINAE principle, this operator can only be selected by the compiler in case the expression
   // specific parallel evaluation strategy is selected.
   */
   template< typename MT  // Type of the target dense matrix
           , bool SO >    // Storage order of the target dense matrix
   friend inline typename EnableIf< UseSMPAssign<MT> >::Type
      smpAssign( DenseMatrix<MT,SO>& lhs, const SMatSMatMultExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == rhs.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( (~lhs).columns() == rhs.columns(), "Invalid number of columns" );

      CT1 A( rhs.lhs_ );  // Evaluation of the left-hand side sparse matrix operand
      CT2 B( rhs.rhs_ );  // Evaluation of the right-hand side sparse matrix operand

      BLAZE_INTERNAL_ASSERT( A.rows()    == rhs.lhs_.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( A.columns() == rhs.lhs_.columns(), "Invalid number of columns" );
      BLAZE_INTERNAL_ASSERT( B.rows()    == rhs.rhs_.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( B.columns() == rhs.rhs_.columns(), "Invalid number of columns" );
      BLAZE_INTERNAL_ASSERT( A.rows()    == (~lhs).rows()     , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( B.columns() == (~lhs).columns()  , "Invalid number of columns" );

      smpAssign( ~lhs, A * B );
   }
   /*! \endcond */
   //**********************************************************************************************

   //**SMP assignment to sparse matrices***********************************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief SMP assignment of a sparse matrix-sparse matrix multiplication to a sparse matrix
   //        (\f$ C=A*B \f$).
   // \ingroup sparse_matrix
   //
   // \param lhs The target left-hand side sparse matrix.
   // \param rhs The right-hand side multiplication expression to be assigned.
   // \return void
   //
   // This function implements the performance optimized assignment of a sparse matrix-sparse
   // matrix multiplication expression to a sparse matrix. Due to the explicit application of
   // the SFINAE principle, this operator can only be selected by the compiler in case the
   // expression specific parallel evaluation strategy is selected.
   */
   template< typename MT  // Type of the target sparse matrix
           , bool SO >    // Storage order of the target sparse matrix
   friend inline typename EnableIf< UseSMPAssign<MT> >::Type
      smpAssign( SparseMatrix<MT,SO>& lhs, const SMatSMatMultExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == rhs.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( (~lhs).columns() == rhs.columns(), "Invalid number of columns" );

      CT1 A( rhs.lhs_ );  // Evaluation of the left-hand side sparse matrix operand
      CT2 B( rhs.rhs_ );  // Evaluation of the right-hand side sparse matrix operand

      BLAZE_INTERNAL_ASSERT( A.rows()    == rhs.lhs_.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( A.columns() == rhs.lhs_.columns(), "Invalid number of columns" );
      BLAZE_INTERNAL_ASSERT( B.rows()    == rhs.rhs_.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( B.columns() == rhs.rhs_.columns(), "Invalid number of columns" );
      BLAZE_INTERNAL_ASSERT( A.rows()    == (~lhs).rows()     , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( B.columns() == (~lhs).columns()  , "Invalid number of columns" );

      smpAssign( ~lhs, A * B );
   }
   /*! \endcond */
   //**********************************************************************************************

   //**SMP addition assignment to dense matrices***************************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief SMP addition assignment of a sparse matrix-sparse matrix multiplication to a dense
   //        matrix (\f$ C+=A*B \f$).
   // \ingroup sparse_matrix
   //
   // \param lhs The target left-hand side dense matrix.
   // \param rhs The right-hand side multiplication expression to be added.
   // \return void
   //
   // This function implements the performance optimized SMP addition assignment of a sparse
   // matrix-sparse matrix multiplication expression to a dense matrix. Due to the explicit
   // application of the SFINAE principle, this operator can only be selected by the compiler
   // in case the expression specific parallel evaluation strategy is selected.
   */
   template< typename MT  // Type of the target dense matrix
           , bool SO >    // Storage order of the target dense matarix
   friend inline typename EnableIf< UseSMPAssign<MT> >::Type
      smpAddAssign( DenseMatrix<MT,SO>& lhs, const SMatSMatMultExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == rhs.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( (~lhs).columns() == rhs.columns(), "Invalid number of columns" );

      CT1 A( rhs.lhs_ );  // Evaluation of the left-hand side sparse matrix operand
      CT2 B( rhs.rhs_ );  // Evaluation of the right-hand side sparse matrix operand

      BLAZE_INTERNAL_ASSERT( A.rows()    == rhs.lhs_.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( A.columns() == rhs.lhs_.columns(), "Invalid number of columns" );
      BLAZE_INTERNAL_ASSERT( B.rows()    == rhs.rhs_.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( B.columns() == rhs.rhs_.columns(), "Invalid number of columns" );
      BLAZE_INTERNAL_ASSERT( A.rows()    == (~lhs).rows()     , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( B.columns() == (~lhs).columns()  , "Invalid number of columns" );

      smpAddAssign( ~lhs, A * B );
   }
   /*! \endcond */
   //**********************************************************************************************

   //**SMP addition assignment to sparse matrices**************************************************
   // No special implementation for the SMP addition assignment to sparse matrices.
   //**********************************************************************************************

   //**SMP subtraction assignment to dense matrices************************************************
   /*! \cond BLAZE_INTERNAL */
   /*!\brief SMP subtraction assignment of a sparse matrix-sparse matrix multiplication to a
   //        dense matrix (\f$ C-=A*B \f$).
   // \ingroup sparse_matrix
   //
   // \param lhs The target left-hand side dense matrix.
   // \param rhs The right-hand side multiplication expression to be subtracted.
   // \return void
   //
   // This function implements the performance optimized SMP subtraction assignment of a sparse
   // matrix-sparse matrix multiplication expression to a dense matrix. Due to the explicit
   // application of the SFINAE principle, this operator can only be selected by the compiler
   // in case the expression specific parallel evaluation strategy is selected.
   */
   template< typename MT  // Type of the target dense matrix
           , bool SO >    // Storage order of the target dense matrix
   friend inline typename EnableIf< UseSMPAssign<MT> >::Type
      smpSubAssign( DenseMatrix<MT,SO>& lhs, const SMatSMatMultExpr& rhs )
   {
      BLAZE_FUNCTION_TRACE;

      BLAZE_INTERNAL_ASSERT( (~lhs).rows()    == rhs.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( (~lhs).columns() == rhs.columns(), "Invalid number of columns" );

      CT1 A( rhs.lhs_ );  // Evaluation of the left-hand side sparse matrix operand
      CT2 B( rhs.rhs_ );  // Evaluation of the right-hand side sparse matrix operand

      BLAZE_INTERNAL_ASSERT( A.rows()    == rhs.lhs_.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( A.columns() == rhs.lhs_.columns(), "Invalid number of columns" );
      BLAZE_INTERNAL_ASSERT( B.rows()    == rhs.rhs_.rows()   , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( B.columns() == rhs.rhs_.columns(), "Invalid number of columns" );
      BLAZE_INTERNAL_ASSERT( A.rows()    == (~lhs).rows()     , "Invalid number of rows"    );
      BLAZE_INTERNAL_ASSERT( B.columns() == (~lhs).columns()  , "Invalid number of columns" );

      smpSubAssign( ~lhs, A * B );
   }
   /*! \endcond */
   //**********************************************************************************************

   //**SMP subtraction assignment to sparse matrices***********************************************
   // No special implementation for the SMP subtraction assignment to sparse matrices.
   //**********************************************************************************************

   //**SMP multiplication assignment to dense matrices*********************************************
   // No special implementation for the SMP multiplication assignment to dense matrices.
   //**********************************************************************************************

   //**SMP multiplication assignment to sparse matrices********************************************
   // No special implementation for the SMP multiplication assignment to sparse matrices.
   //**********************************************************************************************

   //**Compile time checks*************************************************************************
   /*! \cond BLAZE_INTERNAL */
   BLAZE_CONSTRAINT_MUST_BE_SPARSE_MATRIX_TYPE( MT1 );
   BLAZE_CONSTRAINT_MUST_BE_ROW_MAJOR_MATRIX_TYPE( MT1 );
   BLAZE_CONSTRAINT_MUST_BE_SPARSE_MATRIX_TYPE( MT2 );
   BLAZE_CONSTRAINT_MUST_BE_ROW_MAJOR_MATRIX_TYPE( MT2 );
   BLAZE_CONSTRAINT_MUST_FORM_VALID_MATMATMULTEXPR( MT1, MT2 );
   /*! \endcond */
   //**********************************************************************************************
};
//*************************************************************************************************




//=================================================================================================
//
//  GLOBAL BINARY ARITHMETIC OPERATORS
//
//=================================================================================================

//*************************************************************************************************
/*!\brief Multiplication operator for the multiplication of two row-major sparse matrices
//        (\f$ A=B*C \f$).
// \ingroup sparse_matrix
//
// \param lhs The left-hand side sparse matrix for the matrix multiplication.
// \param rhs The right-hand side sparse matrix for the matrix multiplication.
// \return The product of the two matrices.
// \exception std::invalid_argument Matrix sizes do not match
//
// This operator represents the multiplication of two row-major sparse matrices:

   \code
   using blaze::rowMajor;

   blaze::CompressedMatrix<double,rowMajor> A, B, C;
   // ... Resizing and initialization
   C = A * B;
   \endcode

// The operator returns an expression representing a sparse matrix of the higher-order element
// type of the two involved matrix element types \a T1::ElementType and \a T2::ElementType.
// Both matrix types \a T1 and \a T2 as well as the two element types \a T1::ElementType and
// \a T2::ElementType have to be supported by the MultTrait class template.\n
// In case the current number of rows and columns of the two given matrices don't match, a
// \a std::invalid_argument is thrown.
*/
template< typename T1    // Type of the left-hand side sparse matrix
        , typename T2 >  // Type of the right-hand side sparse matrix
inline const SMatSMatMultExpr<T1,T2>
   operator*( const SparseMatrix<T1,false>& lhs, const SparseMatrix<T2,false>& rhs )
{
   BLAZE_FUNCTION_TRACE;

   if( (~lhs).columns() != (~rhs).rows() )
      throw std::invalid_argument( "Matrix sizes do not match" );

   return SMatSMatMultExpr<T1,T2>( ~lhs, ~rhs );
}
//*************************************************************************************************




//=================================================================================================
//
//  ROWS SPECIALIZATIONS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename MT1, typename MT2 >
struct Rows< SMatSMatMultExpr<MT1,MT2> >
   : public Rows<MT1>
{};
/*! \endcond */
//*************************************************************************************************




//=================================================================================================
//
//  COLUMNS SPECIALIZATIONS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename MT1, typename MT2 >
struct Columns< SMatSMatMultExpr<MT1,MT2> >
   : public Columns<MT2>
{};
/*! \endcond */
//*************************************************************************************************




//=================================================================================================
//
//  ISLOWER SPECIALIZATIONS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename MT1, typename MT2 >
struct IsLower< SMatSMatMultExpr<MT1,MT2> >
   : public IsTrue< IsLower<MT1>::value && IsLower<MT2>::value >
{};
/*! \endcond */
//*************************************************************************************************




//=================================================================================================
//
//  ISUPPER SPECIALIZATIONS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename MT1, typename MT2 >
struct IsUpper< SMatSMatMultExpr<MT1,MT2> >
   : public IsTrue< IsUpper<MT1>::value && IsUpper<MT2>::value >
{};
/*! \endcond */
//*************************************************************************************************




//=================================================================================================
//
//  EXPRESSION TRAIT SPECIALIZATIONS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename MT1, typename MT2, typename VT >
struct SMatDVecMultExprTrait< SMatSMatMultExpr<MT1,MT2>, VT >
{
 public:
   //**********************************************************************************************
   typedef typename SelectType< IsSparseMatrix<MT1>::value && IsRowMajorMatrix<MT1>::value &&
                                IsSparseMatrix<MT2>::value && IsRowMajorMatrix<MT2>::value &&
                                IsDenseVector<VT>::value   && IsColumnVector<VT>::value
                              , typename SMatDVecMultExprTrait< MT1, typename SMatDVecMultExprTrait<MT2,VT>::Type >::Type
                              , INVALID_TYPE >::Type  Type;
   //**********************************************************************************************
};
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename MT1, typename MT2, typename VT >
struct SMatSVecMultExprTrait< SMatSMatMultExpr<MT1,MT2>, VT >
{
 public:
   //**********************************************************************************************
   typedef typename SelectType< IsSparseMatrix<MT1>::value && IsRowMajorMatrix<MT1>::value &&
                                IsSparseMatrix<MT2>::value && IsRowMajorMatrix<MT2>::value &&
                                IsSparseVector<VT>::value  && IsColumnVector<VT>::value
                              , typename SMatSVecMultExprTrait< MT1, typename SMatSVecMultExprTrait<MT2,VT>::Type >::Type
                              , INVALID_TYPE >::Type  Type;
   //**********************************************************************************************
};
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename VT, typename MT1, typename MT2 >
struct TDVecSMatMultExprTrait< VT, SMatSMatMultExpr<MT1,MT2> >
{
 public:
   //**********************************************************************************************
   typedef typename SelectType< IsDenseVector<VT>::value   && IsRowVector<VT>::value       &&
                                IsSparseMatrix<MT1>::value && IsRowMajorMatrix<MT1>::value &&
                                IsSparseMatrix<MT2>::value && IsRowMajorMatrix<MT2>::value
                              , typename TDVecSMatMultExprTrait< typename TDVecSMatMultExprTrait<VT,MT1>::Type, MT2 >::Type
                              , INVALID_TYPE >::Type  Type;
   //**********************************************************************************************
};
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename VT, typename MT1, typename MT2 >
struct TSVecSMatMultExprTrait< VT, SMatSMatMultExpr<MT1,MT2> >
{
 public:
   //**********************************************************************************************
   typedef typename SelectType< IsSparseVector<VT>::value  && IsRowVector<VT>::value       &&
                                IsSparseMatrix<MT1>::value && IsRowMajorMatrix<MT1>::value &&
                                IsSparseMatrix<MT2>::value && IsRowMajorMatrix<MT2>::value
                              , typename TSVecSMatMultExprTrait< typename TSVecSMatMultExprTrait<VT,MT1>::Type, MT2 >::Type
                              , INVALID_TYPE >::Type  Type;
   //**********************************************************************************************
};
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename MT1, typename MT2, bool AF >
struct SubmatrixExprTrait< SMatSMatMultExpr<MT1,MT2>, AF >
{
 public:
   //**********************************************************************************************
   typedef typename MultExprTrait< typename SubmatrixExprTrait<const MT1,AF>::Type
                                 , typename SubmatrixExprTrait<const MT2,AF>::Type >::Type  Type;
   //**********************************************************************************************
};
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename MT1, typename MT2 >
struct RowExprTrait< SMatSMatMultExpr<MT1,MT2> >
{
 public:
   //**********************************************************************************************
   typedef typename MultExprTrait< typename RowExprTrait<const MT1>::Type, MT2 >::Type  Type;
   //**********************************************************************************************
};
/*! \endcond */
//*************************************************************************************************


//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename MT1, typename MT2 >
struct ColumnExprTrait< SMatSMatMultExpr<MT1,MT2> >
{
 public:
   //**********************************************************************************************
   typedef typename MultExprTrait< MT1, typename ColumnExprTrait<const MT2>::Type >::Type  Type;
   //**********************************************************************************************
};
/*! \endcond */
//*************************************************************************************************

} // namespace blaze

#endif
