/*
********************************************************************************

NOTE - One of the two copyright statements below may be chosen
       that applies for the software.

********************************************************************************

This software module was originally developed by

Heiko Schwarz    (Fraunhofer HHI),
Tobias Hinz      (Fraunhofer HHI),
Karsten Suehring (Fraunhofer HHI)

in the course of development of the ISO/IEC 14496-10:2005 Amd.1 (Scalable Video
Coding) for reference purposes and its performance may not have been optimized.
This software module is an implementation of one or more tools as specified by
the ISO/IEC 14496-10:2005 Amd.1 (Scalable Video Coding).

Those intending to use this software module in products are advised that its
use may infringe existing patents. ISO/IEC have no liability for use of this
software module or modifications thereof.

Assurance that the originally developed software module can be used
(1) in the ISO/IEC 14496-10:2005 Amd.1 (Scalable Video Coding) once the
ISO/IEC 14496-10:2005 Amd.1 (Scalable Video Coding) has been adopted; and
(2) to develop the ISO/IEC 14496-10:2005 Amd.1 (Scalable Video Coding): 

To the extent that Fraunhofer HHI owns patent rights that would be required to
make, use, or sell the originally developed software module or portions thereof
included in the ISO/IEC 14496-10:2005 Amd.1 (Scalable Video Coding) in a
conforming product, Fraunhofer HHI will assure the ISO/IEC that it is willing
to negotiate licenses under reasonable and non-discriminatory terms and
conditions with applicants throughout the world.

Fraunhofer HHI retains full right to modify and use the code for its own
purpose, assign or donate the code to a third party and to inhibit third
parties from using the code for products that do not conform to MPEG-related
ITU Recommendations and/or ISO/IEC International Standards. 

This copyright notice must be included in all copies or derivative works.
Copyright (c) ISO/IEC 2005. 

********************************************************************************

COPYRIGHT AND WARRANTY INFORMATION

Copyright 2005, International Telecommunications Union, Geneva

The Fraunhofer HHI hereby donate this source code to the ITU, with the following
understanding:
    1. Fraunhofer HHI retain the right to do whatever they wish with the
       contributed source code, without limit.
    2. Fraunhofer HHI retain full patent rights (if any exist) in the technical
       content of techniques and algorithms herein.
    3. The ITU shall make this code available to anyone, free of license or
       royalty fees.

DISCLAIMER OF WARRANTY

These software programs are available to the user without any license fee or
royalty on an "as is" basis. The ITU disclaims any and all warranties, whether
express, implied, or statutory, including any implied warranties of
merchantability or of fitness for a particular purpose. In no event shall the
contributor or the ITU be liable for any incidental, punitive, or consequential
damages of any kind whatsoever arising from the use of these programs.

This disclaimer of warranty extends to the user of these programs and user's
customers, employees, agents, transferees, successors, and assigns.

The ITU does not represent or warrant that the programs furnished hereunder are
free of infringement of any third-party patents. Commercial implementations of
ITU-T Recommendations, including shareware, may be subject to royalty fees to
patent holders. Information regarding the ITU-T patent policy is available from 
the ITU Web site at http://www.itu.int.

THIS IS NOT A GRANT OF PATENT RIGHTS - SEE THE ITU-T PATENT POLICY.

********************************************************************************
*/



#include "H264AVCEncoderLib.h"
#include "FGSSubbandDecoder.h"
#include "CabacReader.h"


#include "H264AVCCommonLib/MbDataCtrl.h"
#include "H264AVCCommonLib/IntFrame.h"
#include "H264AVCCommonLib/IntYuvMbBuffer.h"
#include "H264AVCCommonLib/Transform.h"
#include "H264AVCCommonLib/TraceFile.h"




H264AVC_NAMESPACE_BEGIN




RQFGSDecoder::RQFGSDecoder()
: m_bInit                     ( false )
, m_papcYuvFullPelBufferCtrl  ( 0 )
, m_pcTransform               ( 0 )
, m_pcCabacReader             ( 0 )
, m_bPicInit                  ( false )
, m_bPicChanged               ( false )
, m_bPicFinished              ( false )
, m_uiWidthInMB               ( 0 )
, m_uiHeightInMB              ( 0 )
, m_pcCurrSliceHeader         ( 0 )
, m_pcCurrMbDataCtrl          ( 0 )
{
  ::memset( m_apaucLumaCoefMap,       0x00,   16*sizeof(UChar*) );
  ::memset( m_aapaucChromaDCCoefMap,  0x00, 2* 4*sizeof(UChar*) );
  ::memset( m_aapaucChromaACCoefMap,  0x00, 2*16*sizeof(UChar*) );
  ::memset( m_apaucChromaDCBlockMap,  0x00,    2*sizeof(UChar*) );
  ::memset( m_apaucChromaACBlockMap,  0x00,    2*sizeof(UChar*) );
  m_paucBlockMap        = 0;
  m_paucSubMbMap        = 0;
  m_pauiMacroblockMap   = 0;
}


RQFGSDecoder::~RQFGSDecoder()
{
  AOT( m_bInit );
}


ErrVal
RQFGSDecoder::create( RQFGSDecoder*& rpcRQFGSDecoder )
{
  rpcRQFGSDecoder = new RQFGSDecoder;
  ROT( NULL == rpcRQFGSDecoder );
  return Err::m_nOK;
}
  

ErrVal
RQFGSDecoder::destroy()
{
  delete this;
  return Err::m_nOK;
}


ErrVal
RQFGSDecoder::init( YuvBufferCtrl** apcYuvFullPelBufferCtrl,
                    Transform*      pcTransform,
                    CabacReader*    pcCabacReader )
{
  ROT( m_bInit );
  ROF( apcYuvFullPelBufferCtrl );
  ROF( pcTransform );
  ROF( pcCabacReader );

  m_papcYuvFullPelBufferCtrl  = apcYuvFullPelBufferCtrl;
  m_pcTransform               = pcTransform;
  m_pcCabacReader             = pcCabacReader;
  m_bInit                     = true;

  m_bPicInit                  = false;
  m_bPicChanged               = false;
  m_bPicFinished              = false;
  m_uiWidthInMB               = 0;
  m_uiHeightInMB              = 0;
  m_pcCurrSliceHeader         = 0;
  m_pcCurrMbDataCtrl          = 0;

  ::memset( m_apaucLumaCoefMap,       0x00,   16*sizeof(UChar*) );
  ::memset( m_aapaucChromaDCCoefMap,  0x00, 2* 4*sizeof(UChar*) );
  ::memset( m_aapaucChromaACCoefMap,  0x00, 2*16*sizeof(UChar*) );
  ::memset( m_apaucChromaDCBlockMap,  0x00,    2*sizeof(UChar*) );
  ::memset( m_apaucChromaACBlockMap,  0x00,    2*sizeof(UChar*) );
  m_paucBlockMap        = 0;
  m_paucSubMbMap        = 0;
  m_pauiMacroblockMap   = 0;

  return Err::m_nOK;
}
  

ErrVal
RQFGSDecoder::uninit()
{
  ROT( m_bPicInit );
  ROF( m_bInit );
  
  m_cMbDataCtrlEL.uninit();

  m_bInit                     = false;
  m_papcYuvFullPelBufferCtrl  = 0;
  m_pcTransform               = 0;
  m_pcCabacReader             = 0;

  m_bPicInit                  = false;
  m_bPicChanged               = false;
  m_bPicFinished              = false;
  m_uiWidthInMB               = 0;
  m_uiHeightInMB              = 0;
  m_pcCurrSliceHeader         = 0;
  m_pcCurrMbDataCtrl          = 0;

  Int i;
  for( i = 0; i < 16; i++ ) 
  {
    delete  [] m_apaucLumaCoefMap        [i];
    delete  [] m_aapaucChromaACCoefMap[0][i];
    delete  [] m_aapaucChromaACCoefMap[1][i];
  }
  for( i = 0; i < 4; i++ ) 
  {
    delete  [] m_aapaucChromaDCCoefMap[0][i];
    delete  [] m_aapaucChromaDCCoefMap[1][i];
  }
  delete    [] m_apaucChromaDCBlockMap[0];
  delete    [] m_apaucChromaDCBlockMap[1];
  delete    [] m_apaucChromaACBlockMap[0];
  delete    [] m_apaucChromaACBlockMap[1];
  delete    [] m_paucBlockMap;
  delete    [] m_paucSubMbMap;
  delete    [] m_pauiMacroblockMap;

  return Err::m_nOK;
}


ErrVal
RQFGSDecoder::xInitSPS( const SequenceParameterSet& rcSPS )
{
  UInt uiSize = rcSPS.getFrameWidthInMbs() * rcSPS.getFrameHeightInMbs();

  if( uiSize > m_cMbDataCtrlEL.getSize() )
  {
    RNOK( m_cMbDataCtrlEL.uninit() );
    RNOK( m_cMbDataCtrlEL.init  ( rcSPS ) );

    Int  i;
    for( i = 0; i < 16; i++ ) 
    {
      delete  [] m_apaucLumaCoefMap        [i];
      delete  [] m_aapaucChromaACCoefMap[0][i];
      delete  [] m_aapaucChromaACCoefMap[1][i];
    }
    for( i = 0; i < 4; i++ ) 
    {
      delete  [] m_aapaucChromaDCCoefMap[0][i];
      delete  [] m_aapaucChromaDCCoefMap[1][i];
    }
    delete    [] m_apaucChromaDCBlockMap[0];
    delete    [] m_apaucChromaDCBlockMap[1];
    delete    [] m_apaucChromaACBlockMap[0];
    delete    [] m_apaucChromaACBlockMap[1];
    delete    [] m_paucBlockMap;
    delete    [] m_paucSubMbMap;
    delete    [] m_pauiMacroblockMap;


    for( i = 0; i < 16; i++ )
    {
      ROFRS ( ( m_apaucLumaCoefMap        [i] = new UChar[uiSize*16] ), Err::m_nERR );
      ROFRS ( ( m_aapaucChromaACCoefMap[0][i] = new UChar[uiSize*4 ] ), Err::m_nERR );
      ROFRS ( ( m_aapaucChromaACCoefMap[1][i] = new UChar[uiSize*4 ] ), Err::m_nERR );
    }
    for( i = 0; i < 4; i++ )
    {
      ROFRS ( ( m_aapaucChromaDCCoefMap[0][i] = new UChar[uiSize   ] ), Err::m_nERR );
      ROFRS ( ( m_aapaucChromaDCCoefMap[1][i] = new UChar[uiSize   ] ), Err::m_nERR );
    }
    ROFRS   ( ( m_apaucChromaDCBlockMap[0]    = new UChar[uiSize   ] ), Err::m_nERR );
    ROFRS   ( ( m_apaucChromaDCBlockMap[1]    = new UChar[uiSize   ] ), Err::m_nERR );
    ROFRS   ( ( m_apaucChromaACBlockMap[0]    = new UChar[uiSize*4 ] ), Err::m_nERR );
    ROFRS   ( ( m_apaucChromaACBlockMap[1]    = new UChar[uiSize*4 ] ), Err::m_nERR );
    ROFRS   ( ( m_paucBlockMap                = new UChar[uiSize*16] ), Err::m_nERR );
    ROFRS   ( ( m_paucSubMbMap                = new UChar[uiSize*4 ] ), Err::m_nERR );
    ROFRS   ( ( m_pauiMacroblockMap           = new UInt [uiSize   ] ), Err::m_nERR );
  }

  return Err::m_nOK;
}


ErrVal
RQFGSDecoder::initPicture( SliceHeader* pcSliceHeader,
                           MbDataCtrl*  pcCurrMbDataCtrl )
{
  ROT( m_bPicInit );
  ROF( m_bInit );
  ROF( pcSliceHeader );
  ROF( pcCurrMbDataCtrl );

  RNOK( xInitSPS( pcSliceHeader->getSPS() ) );

  m_uiWidthInMB       = pcSliceHeader->getSPS().getFrameWidthInMbs  ();
  m_uiHeightInMB      = pcSliceHeader->getSPS().getFrameHeightInMbs ();
  m_pcCurrSliceHeader = pcSliceHeader;
  m_pcCurrMbDataCtrl  = pcCurrMbDataCtrl;
  m_bPicInit          = true;
  m_bPicChanged       = false;
  m_bPicFinished      = false;

  RNOK( xScaleBaseLayerCoeffs() );

  return Err::m_nOK;
}


ErrVal
RQFGSDecoder::finishPicture()
{
  ROF( m_bPicInit );

  m_bPicInit          = false;
  m_bPicChanged       = false;
  m_bPicFinished      = false;
  m_uiWidthInMB       = 0;
  m_uiHeightInMB      = 0;
  m_pcCurrSliceHeader = 0;
  m_pcCurrMbDataCtrl  = 0;

  return Err::m_nOK;
}



ErrVal
RQFGSDecoder::decodeNextLayer( SliceHeader* pcSliceHeader )
{
  ROF( m_bInit );
  ROF( m_bPicInit );
  
  //===== update slice header =====
  m_pcCurrSliceHeader->setSliceHeaderQp ( pcSliceHeader->getPicQp()          );
  m_pcCurrSliceHeader->setFirstMbInSlice( pcSliceHeader->getFirstMbInSlice() );
  m_pcCurrSliceHeader->setNumMbsInSlice ( pcSliceHeader->getNumMbsInSlice () );
  m_bPicChanged = true;

  RNOK( xDecodingFGS() );
  return Err::m_nOK;
}




ErrVal
RQFGSDecoder::reconstruct( IntFrame*  pcRecResidual )
{
  ROF( m_bInit );
  ROF( m_bPicInit );
  ROF( pcRecResidual );

  UInt            uiLayer         = m_pcCurrSliceHeader->getLayerId();
  YuvBufferCtrl*  pcYuvBufferCtrl = m_papcYuvFullPelBufferCtrl[uiLayer];
  IntYuvMbBuffer  cMbBuffer;

  RNOK( m_pcCurrMbDataCtrl->initSlice ( *m_pcCurrSliceHeader, PRE_PROCESS, true, NULL ) );

  for( UInt uiMbY = 0; uiMbY < m_uiHeightInMB; uiMbY++ )
  for( UInt uiMbX = 0; uiMbX < m_uiWidthInMB;  uiMbX++ )
  {
    MbDataAccess* pcMbDataAccess = 0;
    RNOK( m_pcCurrMbDataCtrl->initMb( pcMbDataAccess, uiMbY, uiMbX ) );
    RNOK( pcYuvBufferCtrl   ->initMb( uiMbY, uiMbX ) );
    RNOK( xReconstructMacroblock    ( *pcMbDataAccess, cMbBuffer    ) );
    RNOK( pcRecResidual->getFullPelYuvBuffer()->loadBuffer( &cMbBuffer ) );
  }

  return Err::m_nOK;
}




ErrVal
RQFGSDecoder::xScaleBaseLayerCoeffs()
{
  RNOK( m_pcCurrMbDataCtrl->initSlice( *m_pcCurrSliceHeader, PRE_PROCESS, true, NULL ) );

  for( UInt uiMbY = 0; uiMbY < m_uiHeightInMB; uiMbY++ )
  for( UInt uiMbX = 0; uiMbX < m_uiWidthInMB;  uiMbX++ )
  {
    MbDataAccess* pcMbDataAccess = 0;

    RNOK( m_pcCurrMbDataCtrl->initMb(  pcMbDataAccess, uiMbY, uiMbX ) );
    RNOK( xScaleTCoeffs             ( *pcMbDataAccess, true ) );
  }
  
  return Err::m_nOK;
}



ErrVal
RQFGSDecoder::xScale4x4Block( TCoeff*            piCoeff,
                              const UChar*       pucScale,
                              UInt               uiStart,
                              const QpParameter& rcQP )
{
  if( pucScale )
  {
    Int iAdd = ( rcQP.per() <= 3 ? ( 1 << ( 3 - rcQP.per() ) ) : 0 );
    
    for( UInt ui = uiStart; ui < 16; ui++ )
    {
      piCoeff[ui] = ( ( piCoeff[ui] * g_aaiDequantCoef[rcQP.rem()][ui] * pucScale[ui] + iAdd ) << rcQP.per() ) >> 4;
    }
  }
  else
  {
    for( UInt ui = uiStart; ui < 16; ui++ )
    {
      piCoeff[ui] *= ( g_aaiDequantCoef[rcQP.rem()][ui] << rcQP.per() );
    }
  }

  return Err::m_nOK;
}


ErrVal
RQFGSDecoder::xScale8x8Block( TCoeff*            piCoeff,
                              const UChar*       pucScale,
                              const QpParameter& rcQP )
{
  Int iAdd = ( rcQP.per() <= 5 ? ( 1 << ( 5 - rcQP.per() ) ) : 0 );

  if( pucScale )
  {
    for( UInt ui = 0; ui < 64; ui++ )
    {
      piCoeff[ui] = ( ( piCoeff[ui] * g_aaiDequantCoef64[rcQP.rem()][ui] * pucScale[ui] + iAdd ) << rcQP.per() ) >> 6;
    }
  }
  else
  {
    for( UInt ui = 0; ui < 64; ui++ )
    {
      piCoeff[ui] = ( ( piCoeff[ui] * g_aaiDequantCoef64[rcQP.rem()][ui] * 16 + iAdd ) << rcQP.per() ) >> 6;
    }
  }

  return Err::m_nOK;
}


ErrVal
RQFGSDecoder::xScaleTCoeffs( MbDataAccess& rcMbDataAccess,
                             Bool          bBaseLayer )
{
  const Int aaiDequantDcCoef[6] = {  10, 11, 13, 14, 16, 18 };
  Quantizer cQuantizer;
  cQuantizer.setQp( rcMbDataAccess, false );
  
  const QpParameter&  cLQp      = cQuantizer.getLumaQp  ();
  const QpParameter&  cCQp      = cQuantizer.getChromaQp();
  Bool                bIntra    = rcMbDataAccess.getMbData().isIntra();
  Bool                b8x8      = rcMbDataAccess.getMbData().isTransformSize8x8();
  Bool                b16x16    = rcMbDataAccess.getMbData().isIntra16x16();
  UInt                uiYScalId = ( bIntra ? ( b8x8 && !b16x16 ? 6 : 0 ) : ( b8x8 ? 7 : 3 ) );
  UInt                uiUScalId = ( bIntra ? 1 : 4 );
  UInt                uiVScalId = ( bIntra ? 2 : 5 );
  const UChar*        pucScaleY = rcMbDataAccess.getSH().getScalingMatrix( uiYScalId );
  const UChar*        pucScaleU = rcMbDataAccess.getSH().getScalingMatrix( uiUScalId );
  const UChar*        pucScaleV = rcMbDataAccess.getSH().getScalingMatrix( uiVScalId );
  Int                 iScale    = 1;

  //===== luma =====
  if( b16x16 )
  {
    //===== INTRA_16x16 =====
    for( B4x4Idx cIdx; cIdx.isLegal(); cIdx++ )
    {
      RNOK( xScale4x4Block( rcMbDataAccess.getMbTCoeffs().get   ( cIdx ), pucScaleY, 1, cLQp ) );
    }

    iScale  = aaiDequantDcCoef[cLQp.rem()] << cLQp.per();
    if( pucScaleY )
    {
      iScale  *= pucScaleY[0];
      iScale >>= 4;
    }
    // perform scaling only
    TCoeff* piCoeff = rcMbDataAccess.getMbTCoeffs().get( B4x4Idx(0) );

    for( Int uiDCIdx = 0; uiDCIdx < 16; uiDCIdx++ )
      piCoeff[16*uiDCIdx] *= iScale;

    //===== correct CBP =====
    rcMbDataAccess.getMbData().setMbCbp( rcMbDataAccess.getAutoCbp() );
  }
  else if( b8x8 )
  {
    //===== 8x8 BLOCKS =====
    for( B8x8Idx cIdx; cIdx.isLegal(); cIdx++ )
    {
      RNOK( xScale8x8Block( rcMbDataAccess.getMbTCoeffs().get8x8( cIdx ), pucScaleY, cLQp ) );
    }
  }
  else
  {
    //===== 4x4 BLOCKS =====
    for( B4x4Idx cIdx; cIdx.isLegal(); cIdx++ )
    {
      RNOK( xScale4x4Block( rcMbDataAccess.getMbTCoeffs().get   ( cIdx ), pucScaleY, 0, cLQp ) );
    }
  }

  //===== chroma =====
  for( CIdx cIdx; cIdx.isLegal(); cIdx++ )
  {
    RNOK( xScale4x4Block( rcMbDataAccess.getMbTCoeffs().get( cIdx ), ( cIdx.plane() ? pucScaleV : pucScaleU ), 1, cCQp ) );
  }

  // only performs scaling, not inverse transform
  UInt  uiMbIndex = rcMbDataAccess.getMbY()*m_uiWidthInMB+rcMbDataAccess.getMbX();
  for( UInt uiPlane = 0; uiPlane < 2; uiPlane++ )
  {
    const UChar* pucScaleUV = (uiPlane == 0) ? pucScaleU : pucScaleV;
    TCoeff*      piCoeff = rcMbDataAccess.getMbTCoeffs().get( CIdx(4*uiPlane) );

    iScale = g_aaiDequantCoef[cCQp.rem()][0] << cCQp.per();
    /* HS: old scaling modified:
       (I did not work for scaling matrices, when QpPer became less than 5 in an FGS enhancement) */

    for( UInt uiDCIdx = 0; uiDCIdx < 4; uiDCIdx++ )
    {
      if(! bBaseLayer && ! ( m_aapaucChromaDCCoefMap[uiPlane][uiDCIdx][uiMbIndex] & CODED ) )
        // condition "! bBaseLayer" is needed. When "xScaleTCoeffs is called 
        // before first FGS layer, m_aapaucChromaDCCoefMap is not initialized
        piCoeff[16*uiDCIdx]  = 0;
      else
        piCoeff[16*uiDCIdx] *= iScale;
    }
  }

  return Err::m_nOK;
}


ErrVal
RQFGSDecoder::xReconstructMacroblock( MbDataAccess&   rcMbDataAccess,
                                      IntYuvMbBuffer& rcMbBuffer )
{
  m_pcTransform->setClipMode( false );

  Int                 iLStride  = rcMbBuffer.getLStride();
  Int                 iCStride  = rcMbBuffer.getCStride();
  Bool                b8x8      = rcMbDataAccess.getMbData().isTransformSize8x8();
  MbTransformCoeffs&  rcCoeffs  = rcMbDataAccess.getMbTCoeffs();

  rcMbBuffer.setAllSamplesToZero();
  
  TCoeff lumaDcCoeffs[16];
  if ( rcMbDataAccess.getMbData().isIntra16x16() )
  {
    // backup luma DC
    TCoeff *piCoeffs;
    UInt   uiDCIdx;

    piCoeffs = rcCoeffs.get( B4x4Idx(0) );
    for( uiDCIdx = 0; uiDCIdx < 16; uiDCIdx++ )
      lumaDcCoeffs[uiDCIdx] = piCoeffs[16*uiDCIdx] ;

    // inverse transform on luma DC
    RNOK( m_pcTransform->invTransformDcCoeff( piCoeffs, 1 ) );

    // inverse transform on entire MB
    for( B4x4Idx cIdx; cIdx.isLegal(); cIdx++ )
    {
      RNOK( m_pcTransform->invTransform4x4Blk( rcMbBuffer.getYBlk( cIdx ), iLStride, rcCoeffs.get( cIdx ) ) );
    }

    // restore luma DC
    for( uiDCIdx = 0; uiDCIdx < 16; uiDCIdx++ )
      piCoeffs[16*uiDCIdx] = lumaDcCoeffs[uiDCIdx];
  }
  else if( b8x8 )
  {
    for( B8x8Idx cIdx; cIdx.isLegal(); cIdx++ )
    {
      RNOK( m_pcTransform->invTransform8x8Blk( rcMbBuffer.getYBlk( cIdx ), iLStride, rcCoeffs.get8x8( cIdx ) ) );
    }
  }
  else
  {
    for( B4x4Idx cIdx; cIdx.isLegal(); cIdx++ )
    {
      RNOK( m_pcTransform->invTransform4x4Blk( rcMbBuffer.getYBlk( cIdx ), iLStride, rcCoeffs.get( cIdx ) ) );
    }
  }

  Int       iShift, iScale;
  UInt      uiPlane;
  Quantizer cQuantizer;
  cQuantizer.setQp( rcMbDataAccess, false );
  TCoeff            chromaDcCoeffs[2][4];

  const QpParameter&  cCQp      = cQuantizer.getChromaQp();
  Bool                bIntra    = rcMbDataAccess.getMbData().isIntra();
  UInt                uiUScalId = ( bIntra ? 1 : 4 );
  UInt                uiVScalId = ( bIntra ? 2 : 5 );
  const UChar*        pucScaleU = rcMbDataAccess.getSH().getScalingMatrix( uiUScalId );
  const UChar*        pucScaleV = rcMbDataAccess.getSH().getScalingMatrix( uiVScalId );

  // backup chroma DC coefficients
  for( uiPlane = 0; uiPlane < 2; uiPlane++ )
  {
    TCoeff*   piCoeff = rcCoeffs.get( CIdx(4*uiPlane) );
    for( UInt uiDCIdx = 0; uiDCIdx < 4; uiDCIdx++ )
      chromaDcCoeffs[uiPlane][uiDCIdx] = piCoeff[16*uiDCIdx];
  }

  // scaling has already been performed on DC coefficients
  /* HS: old scaling modified:
     (I did not work for scaling matrices, when QpPer became less than 5 in an FGS enhancement) */
  iScale = ( pucScaleU ? pucScaleU[0] : 1 );
  iShift = ( pucScaleU ? 5 : 1 );
  m_pcTransform->invTransformChromaDc( rcCoeffs.get( CIdx(0) ), iScale, iShift );     
  iScale = ( pucScaleV ? pucScaleV[0] : 1 );
  iShift = ( pucScaleV ? 5 : 1 );
  m_pcTransform->invTransformChromaDc( rcCoeffs.get( CIdx(4) ), iScale, iShift );

  RNOK( m_pcTransform->invTransformChromaBlocks( rcMbBuffer.getMbCbAddr(), iCStride, rcCoeffs.get( CIdx(0) ) ) );
  RNOK( m_pcTransform->invTransformChromaBlocks( rcMbBuffer.getMbCrAddr(), iCStride, rcCoeffs.get( CIdx(4) ) ) );

  // restore chroma DC coefficients
  for( uiPlane = 0; uiPlane < 2; uiPlane++ )
  {
    TCoeff*   piCoeff = rcCoeffs.get( CIdx(4*uiPlane) );
    for( UInt uiDCIdx = 0; uiDCIdx < 4; uiDCIdx++ )
      piCoeff[16*uiDCIdx] = chromaDcCoeffs[uiPlane][uiDCIdx];
  }
  m_pcTransform->setClipMode( true );

  return Err::m_nOK;
}


Int
RQFGSDecoder::xScaleLevel4x4( Int                 iLevel,
                              Int                 iIndex,
                              const QpParameter&  cQP,
                              const QpParameter&  cBaseQP )
{
  Int iSign       = ( iLevel < 0 ? -1 : 1 );
  Int iBaseScale  = g_aaiDequantCoef[cBaseQP.rem()][iIndex] << cBaseQP.per();
  Int iScale      = g_aaiDequantCoef[cQP    .rem()][iIndex] << cQP    .per();

  return iSign * ( ( abs(iLevel) * iBaseScale ) + ( iScale >> 1 ) ) / iScale;
}

Int
RQFGSDecoder::xScaleLevel8x8( Int                 iLevel,
                              Int                 iIndex,
                              const QpParameter&  cQP,
                              const QpParameter&  cBaseQP )
{
  Int iSign       = ( iLevel < 0 ? -1 : 1 );
  Int iBaseScale  = g_aaiDequantCoef64[cBaseQP.rem()][iIndex] << cBaseQP.per();
  Int iScale      = g_aaiDequantCoef64[cQP    .rem()][iIndex] << cQP    .per();

  return iSign * ( ( abs(iLevel) * iBaseScale ) + ( iScale >> 1 ) ) / iScale;
}


ErrVal
RQFGSDecoder::xScaleSymbols4x4( TCoeff*             piCoeff,
                                const QpParameter&  cQP,
                                const QpParameter&  cBaseQP )
{
  for( Int iIndex = 0; iIndex < 16; iIndex++ )
  {
    if( piCoeff[iIndex] )
    {
      piCoeff[iIndex] = xScaleLevel4x4( piCoeff[iIndex], iIndex, cQP, cBaseQP );
    }
  }
  return Err::m_nOK;
}


ErrVal
RQFGSDecoder::xScaleSymbols8x8( TCoeff*             piCoeff,
                                const QpParameter&  cQP,
                                const QpParameter&  cBaseQP )
{
  for( Int iIndex = 0; iIndex < 64; iIndex++ )
  {
    if( piCoeff[iIndex] )
    {
      piCoeff[iIndex] = xScaleLevel8x8( piCoeff[iIndex], iIndex, cQP, cBaseQP );
    }
  }
  return Err::m_nOK;
}




ErrVal
RQFGSDecoder::xUpdateSymbols( TCoeff* piCoeff,
                              TCoeff* piCoeffEL,
                              Bool&   bSigDC,
                              Bool&   bSigAC,
                              Int     iNumCoeff )
{
  piCoeff     [0] += piCoeffEL[0];
  if( piCoeff [0] )
  {
    bSigDC = true;
  }

  for( Int iIndex = 1; iIndex < iNumCoeff; iIndex++ )
  {
    piCoeff    [iIndex] += piCoeffEL[iIndex];
    if( piCoeff[iIndex] )
    {
      bSigAC = true;
    }
  }

  return Err::m_nOK;
}



ErrVal
RQFGSDecoder::xDecodingFGS()
{
  RNOK( m_cMbDataCtrlEL    .reset     () );
  RNOK( m_cMbDataCtrlEL    .clear     () );
  RNOK( m_cMbDataCtrlEL    .initSlice ( *m_pcCurrSliceHeader, PARSE_PROCESS, true, NULL ) );
  RNOK( m_pcCurrMbDataCtrl->initSlice ( *m_pcCurrSliceHeader, PRE_PROCESS, true, NULL ) );
  
  RNOK( xInitializeCodingPath() );

  RNOK( m_cMbDataCtrlEL    .initSlice ( *m_pcCurrSliceHeader, PRE_PROCESS, true, NULL ) );

  Int iLastQP = m_pcCurrSliceHeader->getPicQp();

  UInt uiFirstMbInSlice = m_pcCurrSliceHeader->getFirstMbInSlice ();
  UInt uiNumMbsInSlice  = m_pcCurrSliceHeader->getNumMbsInSlice  ();
  m_bFgsComponentSep    = m_pcCurrSliceHeader->getFgsComponentSep();

  try
  {
    RNOK( m_pcCabacReader   ->startSlice( *m_pcCurrSliceHeader ) );
    
    //===== SIGNIFICANCE PATH =====
    {
      UInt iStartCycle, iCycle = 0;
      UInt iLumaScanIdx     = 0;
      UInt iChromaDCScanIdx = 0;
      UInt iChromaACScanIdx = 1;

      UInt iCompleteLuma     = 0;
      UInt iCompleteChromaDC = 0;
      UInt iCompleteChromaAC = 0;

      UInt uiFirstMbY = (UInt) ( uiFirstMbInSlice / m_uiWidthInMB );
      UInt uiFirstMbX = uiFirstMbInSlice % m_uiWidthInMB;
      UInt uiLastMbY  = (UInt) ( ( uiFirstMbInSlice + uiNumMbsInSlice ) / m_uiWidthInMB );
      UInt uiLastMbX  = ( uiFirstMbInSlice + uiNumMbsInSlice ) % m_uiWidthInMB;

      while (iLumaScanIdx < 16 || iChromaDCScanIdx < 4 || iChromaACScanIdx < 16) {
        UInt bAllowChromaDC = (iCycle == 0) || ((iCycle >= iStartCycle) && ((iCycle-iStartCycle) % 2 == 0));
        UInt bAllowChromaAC = (iCycle > 0) && ((iCycle == iStartCycle) || ((iCycle >= iStartCycle) && ((iCycle-iStartCycle) % 3 == 1)));
        UInt itCompleteLuma     = 1;
        UInt itCompleteChromaDC = bAllowChromaDC;
        UInt itCompleteChromaAC = bAllowChromaAC;
        for( UInt uiMbYIdx = uiFirstMbY; uiMbYIdx < uiLastMbY; uiMbYIdx++ )
        for( UInt uiMbXIdx = ( uiMbYIdx == uiFirstMbY ? uiFirstMbX : 0 ); uiMbXIdx < ( uiMbYIdx == uiLastMbY ? uiLastMbX : m_uiWidthInMB );  uiMbXIdx++ )
        {
          //===== Luma =====
          for( UInt uiB8YIdx = 2 * uiMbYIdx; uiB8YIdx < 2 * uiMbYIdx + 2; uiB8YIdx++ )
          for( UInt uiB8XIdx = 2 * uiMbXIdx; uiB8XIdx < 2 * uiMbXIdx + 2; uiB8XIdx++ )
          {
            for( UInt uiBlockYIdx = 2 * uiB8YIdx; uiBlockYIdx < 2 * uiB8YIdx + 2; uiBlockYIdx++ )
            for( UInt uiBlockXIdx = 2 * uiB8XIdx; uiBlockXIdx < 2 * uiB8XIdx + 2; uiBlockXIdx++ )
            {
              if (!iCompleteLuma) {
                RNOK( xDecodeNewCoefficientLuma( uiBlockYIdx, uiBlockXIdx, itCompleteLuma, iLastQP ) );
              } else if (iLumaScanIdx < 16) {
                // REFINEMENT PASS
                RNOK( xDecodeCoefficientLumaRef( uiBlockYIdx, uiBlockXIdx, iLumaScanIdx ) );
              }
            } // 4x4 block iteration
          } // 8x8 block iteration

          //===== CHROMA DC =====
          if( bAllowChromaDC && ( m_bFgsComponentSep == 0 || iLumaScanIdx == 16 ) ) {
            for( UInt uiPlane = 0; uiPlane < 2; uiPlane ++ ) {
              if( !iCompleteChromaDC ) {
                RNOK( xDecodeNewCoefficientChromaDC( uiPlane, uiMbYIdx, uiMbXIdx, itCompleteChromaDC, iLastQP ) );
              } else if (iChromaDCScanIdx < 4) {
                // REFINEMENT PASS
                RNOK( xDecodeCoefficientChromaDCRef( uiPlane, uiMbYIdx, uiMbXIdx, iChromaDCScanIdx ) );
              }
            } // for
          } // if

          //===== CHROMA AC =====
          if( bAllowChromaAC && ( m_bFgsComponentSep == 0 || iLumaScanIdx == 16 ) ) {
            for( UInt uiPlane = 0; uiPlane < 2; uiPlane ++ )
            for( UInt uiB8YIdx = 2 * uiMbYIdx; uiB8YIdx < 2 * uiMbYIdx + 2; uiB8YIdx++ )
            for( UInt uiB8XIdx = 2 * uiMbXIdx; uiB8XIdx < 2 * uiMbXIdx + 2; uiB8XIdx++ ) {
              if( !iCompleteChromaAC ) {
                RNOK( xDecodeNewCoefficientChromaAC( uiPlane, uiB8YIdx, uiB8XIdx, itCompleteChromaAC, iLastQP ) );
              } else if (iChromaACScanIdx < 16) {
                // REFINEMENT PASS
                RNOK( xDecodeCoefficientChromaACRef( uiPlane, uiB8YIdx, uiB8XIdx, iChromaACScanIdx ) );
              }
            } // for
          } // if
        } // macroblock iteration

        if (iCompleteLuma)
          iLumaScanIdx++;
        if (iCompleteChromaDC && bAllowChromaDC)
          iChromaDCScanIdx++;
        if (iCompleteChromaAC && bAllowChromaAC)
          iChromaACScanIdx++;

        if ( iCycle == 0 && m_bFgsComponentSep == 0 )
        {
          m_pcCabacReader->RQdecodeCycleSymbol(iStartCycle);
        }

        iCompleteLuma     |= itCompleteLuma;
        iCompleteChromaDC |= itCompleteChromaDC;
        iCompleteChromaAC |= itCompleteChromaAC;
        iCycle++;
      } // while
    }
    // ==
    
    UInt  uiTermBit = 0;
    RNOK( m_pcCabacReader->RQdecodeTermBit( uiTermBit ) );
    // heiko.schwarz@hhi.fhg.de: decoder could assert when nearly complete FGS slices are decoded
    //ROF ( uiTermBit );
    if( !uiTermBit )
    {
      throw CabaDecoder::ReadStop();
    }
  }
  catch( CabaDecoder::ReadStop )
  {
    m_bPicFinished = true;
  }
  RNOK( m_pcCabacReader->finishSlice( *m_pcCurrSliceHeader ) );

  if( ! m_bPicFinished )
  {
    m_pcCurrSliceHeader->setQualityLevel( m_pcCurrSliceHeader->getQualityLevel() + 1 );
  }


  RNOK( xUpdateCodingPath() );
  
  return Err::m_nOK;
}



ErrVal
RQFGSDecoder::xUpdateCodingPath()
{
  for( UInt uiMbY = 0; uiMbY < m_uiHeightInMB; uiMbY++ )
  for( UInt uiMbX = 0; uiMbX < m_uiWidthInMB;  uiMbX++ )
  {
    MbDataAccess* pcMbDataAccessBL = 0;
    MbDataAccess* pcMbDataAccessEL = 0;
    RNOK( m_pcCurrMbDataCtrl->initMb( pcMbDataAccessBL, uiMbY, uiMbX ) );
    RNOK( m_cMbDataCtrlEL    .initMb( pcMbDataAccessEL, uiMbY, uiMbX ) );

    //===== scale enhancement layer coefficients =====
    pcMbDataAccessEL->getMbData().setTransformSize8x8( pcMbDataAccessBL->getMbData().isTransformSize8x8() );    
    RNOK( xScaleTCoeffs( *pcMbDataAccessEL, false ) );
  
    //===== update coefficients, CBP, QP =====
    RNOK( xUpdateMacroblock( *pcMbDataAccessBL, *pcMbDataAccessEL, uiMbY, uiMbX ) );
  }

  return Err::m_nOK;
}


ErrVal
RQFGSDecoder::xInitializeCodingPath()
{
  for( UInt uiMbY = 0; uiMbY < m_uiHeightInMB; uiMbY++ )
  for( UInt uiMbX = 0; uiMbX < m_uiWidthInMB;  uiMbX++ )
  {
    MbDataAccess* pcMbDataAccess    = 0;
    MbDataAccess* pcMbDataAccessEL  = 0;
    RNOK( m_pcCurrMbDataCtrl->initMb( pcMbDataAccess,   uiMbY, uiMbX ) );
    RNOK( m_cMbDataCtrlEL    .initMb( pcMbDataAccessEL, uiMbY, uiMbX ) );

    //===== set QP for enhancement layer =====
    Int iQpEL = max( 0, pcMbDataAccess->getMbData().getQp() - RQ_QP_DELTA );
    pcMbDataAccessEL->getMbData().setQp( iQpEL );

    
    MbData& rcMbData        = pcMbDataAccess->getMbData();
    UInt    uiMbIndex       = uiMbY * m_uiWidthInMB + uiMbX;
    Bool    bIntra4x4       =     rcMbData.isIntra4x4   ();
    Bool    bIntra16x16     =     rcMbData.isIntra16x16 ();
    Bool    bIsSignificant  = (   rcMbData.getMbCbp()          > 0 );
    Bool    bIsSigLuma      = ( ( rcMbData.getMbCbp() & 0xFF ) > 0 );
    Bool    b8x8Present     = (   pcMbDataAccess->getSH().getPPS().getTransform8x8ModeFlag() &&
                                  rcMbData.is8x8TrafoFlagPresent() );
    Bool    b8x8Transform   = ( b8x8Present && ( bIsSigLuma || bIntra4x4 ) && rcMbData.isTransformSize8x8() );
    UInt    uiMbCbp         = pcMbDataAccess->getAutoCbp();

    //===== set macroblock mode =====
    m_pauiMacroblockMap[uiMbIndex] =  ( bIntra16x16 || bIsSignificant                           ? SIGNIFICANT         : CLEAR )
                                   +  ( bIntra16x16 || bIntra4x4 || bIsSigLuma || !b8x8Present  ? TRANSFORM_SPECIFIED : CLEAR );
    
    //--- LUMA ---
    for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
    {
      UInt uiSubMbIndex = ( 2*uiMbY + c8x8Idx.y()/2 ) * 2 * m_uiWidthInMB + ( 2*uiMbX + c8x8Idx.x() / 2 );

      //===== set sub-macroblock mode =====
      m_paucSubMbMap[uiSubMbIndex] = ( ( uiMbCbp & ( 1 << c8x8Idx.b8x8Index() ) ) > 0 ? SIGNIFICANT : CLEAR );

      if( b8x8Transform )
      {
        UInt    auiBlockIdx[4]  = { ( 4*uiMbY + c8x8Idx.y()     ) * 4 * m_uiWidthInMB + ( 4*uiMbX + c8x8Idx.x()     ),
                                    ( 4*uiMbY + c8x8Idx.y()     ) * 4 * m_uiWidthInMB + ( 4*uiMbX + c8x8Idx.x() + 1 ),
                                    ( 4*uiMbY + c8x8Idx.y() + 1 ) * 4 * m_uiWidthInMB + ( 4*uiMbX + c8x8Idx.x()     ),
                                    ( 4*uiMbY + c8x8Idx.y() + 1 ) * 4 * m_uiWidthInMB + ( 4*uiMbX + c8x8Idx.x() + 1 ) };
        TCoeff* piCoeff         = rcMbData.getMbTCoeffs().get8x8( c8x8Idx );

        //===== set block mode =====
        m_paucBlockMap[auiBlockIdx[0]] = m_paucSubMbMap[uiSubMbIndex];
        m_paucBlockMap[auiBlockIdx[1]] = m_paucSubMbMap[uiSubMbIndex];
        m_paucBlockMap[auiBlockIdx[2]] = m_paucSubMbMap[uiSubMbIndex];
        m_paucBlockMap[auiBlockIdx[3]] = m_paucSubMbMap[uiSubMbIndex];

        //===== set transform coefficients =====
        for( UInt ui8x8ScanIndex = 0; ui8x8ScanIndex < 64; ui8x8ScanIndex++ )
        {
          UInt  uiS = ui8x8ScanIndex/4;
          UInt  uiB = auiBlockIdx[ui8x8ScanIndex%4];
          m_apaucLumaCoefMap[uiS][uiB] = ( piCoeff[g_aucFrameScan64[ui8x8ScanIndex]] ? SIGNIFICANT : CLEAR );
        }
      }
      else
      {
        for( S4x4Idx cIdx( c8x8Idx ); cIdx.isLegal( c8x8Idx ); cIdx++ )
        {
          UInt    uiBlockIdx  = ( 4*uiMbY + cIdx.y() ) * 4 * m_uiWidthInMB + ( 4*uiMbX + cIdx.x() );
          TCoeff* piCoeff     = rcMbData.getMbTCoeffs().get( cIdx );
          UChar   ucBlockSig  = CLEAR;

          //===== set transform coefficients =====
          for( UInt uiScanIndex = 0; uiScanIndex < 16; uiScanIndex++ )
          {
            if( piCoeff[g_aucFrameScan[uiScanIndex]] )
            {
              m_apaucLumaCoefMap[uiScanIndex][uiBlockIdx] = SIGNIFICANT;
              ucBlockSig                                  = SIGNIFICANT;
            }
            else
            {
              m_apaucLumaCoefMap[uiScanIndex][uiBlockIdx] = CLEAR;
            }
          }

          //===== set block mode =====
          m_paucBlockMap[uiBlockIdx] = ucBlockSig;
        }
      }
    }


    //--- CHROMA DC ---
    for( UInt uiPlane = 0; uiPlane < 2; uiPlane++ )
    {
      TCoeff* piCoeff     = rcMbData.getMbTCoeffs().get( CIdx(4*uiPlane) );
      UChar   ucBlockSig  = CLEAR;

      for( UInt ui = 0; ui < 4; ui++ )
      {
        if( piCoeff[g_aucIndexChromaDCScan[ui]] )
        {
          m_aapaucChromaDCCoefMap[uiPlane][ui][uiMbIndex] = SIGNIFICANT;
          ucBlockSig                                      = SIGNIFICANT;
        }
        else
        {
          m_aapaucChromaDCCoefMap[uiPlane][ui][uiMbIndex] = CLEAR;
        }
      }
      m_apaucChromaDCBlockMap[uiPlane][uiMbIndex] = ucBlockSig;
    }

    //--- CHROMA AC ---
    for( CIdx cCIdx; cCIdx.isLegal(); cCIdx++ )
    {
      UInt    ui8x8Idx    = ( 2*uiMbY + cCIdx.y() ) * 2 * m_uiWidthInMB + ( 2 * uiMbX + cCIdx.x() );
      TCoeff* piCoeff     = rcMbData.getMbTCoeffs().get( cCIdx );
      UChar   ucBlockSig  = CLEAR;

      for( UInt ui = 1; ui < 16; ui++ )
      {
        if( piCoeff[g_aucFrameScan[ui]] )
        {
          m_aapaucChromaACCoefMap[cCIdx.plane()][ui][ui8x8Idx]  = SIGNIFICANT;
          ucBlockSig                                            = SIGNIFICANT;
        }
        else
        {
          m_aapaucChromaACCoefMap[cCIdx.plane()][ui][ui8x8Idx]  = CLEAR;
        }
      }
      m_apaucChromaACBlockMap[cCIdx.plane()][ui8x8Idx] = ucBlockSig;
    }
  }

  return Err::m_nOK;
}




ErrVal
RQFGSDecoder::xDecodeNewCoefficientLuma( UInt   uiBlockYIndex,
                                         UInt   uiBlockXIndex,
                                         UInt&  uiComplete,
                                         Int&   riLastQp )
{
  UInt    uiBlockIndex  =  uiBlockYIndex    * 4 * m_uiWidthInMB +  uiBlockXIndex;
  UInt    uiSubMbIndex  = (uiBlockYIndex/2) * 2 * m_uiWidthInMB + (uiBlockXIndex/2);
  UInt    uiMbIndex     = (uiBlockYIndex/4) * 1 * m_uiWidthInMB + (uiBlockXIndex/4);
  
  UInt    uiB8x8        = ((uiBlockYIndex%4)/2) * 2 + ((uiBlockXIndex%4)/2);
  UInt    uiB4x4        =  (uiBlockYIndex%4)    * 4 +  (uiBlockXIndex%4);
  Par8x8  ePar8x8       = Par8x8(uiB8x8);
  
  B8x8Idx c8x8Idx(ePar8x8);
  B4x4Idx c4x4Idx(uiB4x4);


  MbDataAccess* pcMbDataAccessEL  = 0;
  MbDataAccess* pcMbDataAccessBL  = 0;
  RNOK( m_cMbDataCtrlEL     .initMb( pcMbDataAccessEL, uiBlockYIndex/4, uiBlockXIndex/4 ) );
  RNOK( m_pcCurrMbDataCtrl ->initMb( pcMbDataAccessBL, uiBlockYIndex/4, uiBlockXIndex/4 ) );
  

  if( ! ( m_paucSubMbMap[uiSubMbIndex] & CODED ) )
  {
    //===== CBP =====
    Bool bSigCBP = m_pcCabacReader->RQdecodeCBP_8x8( *pcMbDataAccessEL, *pcMbDataAccessBL, c8x8Idx );
    m_paucSubMbMap[uiSubMbIndex] |= CODED;
    if(  bSigCBP )
    {
      m_paucSubMbMap[uiSubMbIndex] |= SIGNIFICANT;
    }
    
    if( !bSigCBP )
    {
      //===== set coefficient and block map =====
      for( S4x4Idx cIdx(c8x8Idx); cIdx.isLegal(c8x8Idx); cIdx++ )
      {
        UInt uiBlk = ( (uiBlockYIndex/4)*4 + cIdx.y() ) * 4 * m_uiWidthInMB + ( (uiBlockXIndex/4)*4 + cIdx.x() );

        m_paucBlockMap[uiBlk] |= CODED;

        for( UInt ui = 0; ui < 16; ui++ )
        {
          if( ! ( m_apaucLumaCoefMap[ui][uiBlk] & SIGNIFICANT ) )
          {
            m_apaucLumaCoefMap[ui][uiBlk] |= CODED;
            m_pauiMacroblockMap[uiMbIndex] += ( 1 << NUM_COEFF_SHIFT );
            ROT( (m_pauiMacroblockMap[uiMbIndex]>>NUM_COEFF_SHIFT) > 384 );
          }
        }
      }
      return Err::m_nOK;
    }
    
    //===== DELTA QP & TRANSFORM SIZE =====
    if( ! ( m_pauiMacroblockMap[uiMbIndex] & CODED ) )
    {
      //===== delta QP =====
      if( ! ( m_pauiMacroblockMap[uiMbIndex] & SIGNIFICANT ) )
      {
        pcMbDataAccessEL->setLastQp( riLastQp );
        RNOK( m_pcCabacReader->RQdecodeDeltaQp( *pcMbDataAccessEL, *pcMbDataAccessBL ) );
        riLastQp = pcMbDataAccessEL->getMbData().getQp();

        m_pauiMacroblockMap[uiMbIndex] |= SIGNIFICANT;
      }

      //===== transform size =====
      if( ! ( m_pauiMacroblockMap[uiMbIndex] & TRANSFORM_SPECIFIED ) )
      {
        RNOK( m_pcCabacReader->RQdecode8x8Flag( *pcMbDataAccessEL, *pcMbDataAccessBL ) );

        m_pauiMacroblockMap[uiMbIndex] |= TRANSFORM_SPECIFIED;
      }

      m_pauiMacroblockMap[uiMbIndex] |= CODED;
    }
  }


  if( pcMbDataAccessBL->getMbData().isTransformSize8x8() )
  {
    UInt auiBlockIdx[4] = { ( (uiBlockYIndex/4)*4 + c8x8Idx.y()     ) * 4 * m_uiWidthInMB + ( (uiBlockXIndex/4)*4 + c8x8Idx.x()     ),
                            ( (uiBlockYIndex/4)*4 + c8x8Idx.y()     ) * 4 * m_uiWidthInMB + ( (uiBlockXIndex/4)*4 + c8x8Idx.x() + 1 ),
                            ( (uiBlockYIndex/4)*4 + c8x8Idx.y() + 1 ) * 4 * m_uiWidthInMB + ( (uiBlockXIndex/4)*4 + c8x8Idx.x()     ),
                            ( (uiBlockYIndex/4)*4 + c8x8Idx.y() + 1 ) * 4 * m_uiWidthInMB + ( (uiBlockXIndex/4)*4 + c8x8Idx.x() + 1 ) };



    UInt    uiStop          = 64;
    UInt    ui8x8ScanIndex  = uiStop;
    UInt    ui8x8StartIndex = uiStop;
    UInt    ui8x8Index;
    for( ui8x8Index = 0; ui8x8Index < uiStop; ui8x8Index++ )
    {
      UInt  uiS = ui8x8Index / 4;
      UInt  uiB = auiBlockIdx[ui8x8Index%4];
      if( !( m_apaucLumaCoefMap[uiS][uiB] & SIGNIFICANT ) || ( m_apaucLumaCoefMap[uiS][uiB] & CODED ) )
      {
        ui8x8StartIndex = ui8x8Index;
        break;
      }
    }
    for( ui8x8Index = 0; ui8x8Index < uiStop; ui8x8Index++ )
    {
      UInt  uiS = ui8x8Index / 4;
      UInt  uiB = auiBlockIdx[ui8x8Index%4];
      if( !( m_apaucLumaCoefMap[uiS][uiB] & (SIGNIFICANT|CODED) ) )
      {
        ui8x8ScanIndex  = ui8x8Index;
        break;
      }
    }
    uiComplete &= (ui8x8ScanIndex == uiStop);
    ROTRS( ui8x8ScanIndex == uiStop, Err::m_nOK );

    UInt uiNeedEob = ( ui8x8ScanIndex > ui8x8StartIndex );


    while( ui8x8ScanIndex < 64 )
    {
      RNOK( m_pcCabacReader->RQdecodeNewTCoeff_8x8( *pcMbDataAccessEL, *pcMbDataAccessBL,
                                                    c8x8Idx, ui8x8ScanIndex, uiNeedEob ) );
      if( uiNeedEob )
      {
        //===== end of block =====
        for( UInt ui8x8 = ui8x8ScanIndex; ui8x8 < 64; ui8x8++ )
        {
          UInt  uiS = ui8x8/4;
          UInt  uiB = auiBlockIdx[ui8x8%4];
          if( !( m_apaucLumaCoefMap[uiS][uiB] & SIGNIFICANT ) )
          {
            m_apaucLumaCoefMap[uiS][uiB]    |= CODED;
            m_pauiMacroblockMap[uiMbIndex]  += ( 1 << NUM_COEFF_SHIFT );
            ROT( (m_pauiMacroblockMap[uiMbIndex]>>NUM_COEFF_SHIFT) > 384 );
          }
        }
        break;
      }
      else
      {
        //===== coefficient =====
        uiNeedEob = 0;
        UInt  uiS = ui8x8ScanIndex/4;
        UInt  uiB = auiBlockIdx[ui8x8ScanIndex%4];

        m_apaucLumaCoefMap[uiS][uiB] |= CODED;
        if( pcMbDataAccessEL->getMbTCoeffs().get8x8( c8x8Idx )[g_aucFrameScan64[ui8x8ScanIndex]] )
        {
          m_apaucLumaCoefMap[uiS][uiB] |= SIGNIFICANT;
        }
        m_pauiMacroblockMap[uiMbIndex]  += ( 1 << NUM_COEFF_SHIFT );
        ROT( (m_pauiMacroblockMap[uiMbIndex]>>NUM_COEFF_SHIFT) > 384 );

        if( m_apaucLumaCoefMap[uiS][uiB] & SIGNIFICANT )
        {
          break;
        }

        ui8x8ScanIndex++;
        while( ( ui8x8ScanIndex < 64 ) && (m_apaucLumaCoefMap[ui8x8ScanIndex/4][auiBlockIdx[ui8x8ScanIndex%4]] & SIGNIFICANT) )
        {
          ui8x8ScanIndex++;
        }
      }
    }
  }
  else
  {
    if( ! ( m_paucBlockMap[uiBlockIndex] & CODED ) )
    {
      Bool bSigBCBP = m_pcCabacReader->RQdecodeBCBP_4x4( *pcMbDataAccessEL, *pcMbDataAccessBL, c4x4Idx );
      m_paucBlockMap[uiBlockIndex] |= CODED;
      if(  bSigBCBP )
      {
        m_paucBlockMap[uiBlockIndex] |= SIGNIFICANT;
      }
      if( ! bSigBCBP )
      {
        for( UInt ui = 0; ui < 16; ui++ )
        {
          if( ! ( m_apaucLumaCoefMap[ui][uiBlockIndex] & SIGNIFICANT ) )
          {
            m_apaucLumaCoefMap[ui][uiBlockIndex] |= CODED;
            m_pauiMacroblockMap[uiMbIndex] += ( 1 << NUM_COEFF_SHIFT );
            ROT( (m_pauiMacroblockMap[uiMbIndex]>>NUM_COEFF_SHIFT) > 384 );
          }
        }
        return Err::m_nOK;
      }
    }

    UInt    uiStop       = 16;
    UInt    uiScanIndex  = uiStop;
    UInt    uiStartIndex = uiStop;
    UInt    uiIndex;
    for( uiIndex = 0; uiIndex < uiStop; uiIndex++ )
    {
      if( ! ( m_apaucLumaCoefMap[uiIndex][uiBlockIndex] & SIGNIFICANT ) || ( m_apaucLumaCoefMap[uiIndex][uiBlockIndex] & CODED ) )
      {
        uiStartIndex = uiIndex;
        break;
      }
    }
    for( uiIndex = 0; uiIndex < uiStop; uiIndex++ )
    {
      if( !( m_apaucLumaCoefMap[uiIndex][uiBlockIndex] & (SIGNIFICANT|CODED) ) )
      {
        uiScanIndex = uiIndex;
        break;
      }
    }
    uiComplete &= (uiScanIndex == uiStop);
    ROTRS(uiScanIndex == uiStop, Err::m_nOK);

    UInt uiNeedEob = ( uiScanIndex > uiStartIndex );
    
    while (uiScanIndex < 16) {
      RNOK( m_pcCabacReader->RQdecodeNewTCoeff_Luma( *pcMbDataAccessEL, *pcMbDataAccessBL,
                                                    LUMA_SCAN, c4x4Idx, uiScanIndex, uiNeedEob ) );
      if ( uiNeedEob) {
        for( UInt ui = uiScanIndex; ui < 16; ui++ )
        {
          if( ! ( m_apaucLumaCoefMap[ui][uiBlockIndex] & SIGNIFICANT ) )
          {
            m_apaucLumaCoefMap[ui][uiBlockIndex] |= CODED;
            m_pauiMacroblockMap[uiMbIndex] += ( 1 << NUM_COEFF_SHIFT );
            ROT( (m_pauiMacroblockMap[uiMbIndex]>>NUM_COEFF_SHIFT) > 384 );
          }
        }
        break;
      } else {

        uiNeedEob = 0;
        m_apaucLumaCoefMap[uiScanIndex][uiBlockIndex] |= CODED;
        if( pcMbDataAccessEL->getMbTCoeffs().get( c4x4Idx )[g_aucFrameScan[uiScanIndex]] )
        {
          m_apaucLumaCoefMap[uiScanIndex][uiBlockIndex] |= SIGNIFICANT;
        }
        m_pauiMacroblockMap[uiMbIndex] += ( 1 << NUM_COEFF_SHIFT );
        ROT( (m_pauiMacroblockMap[uiMbIndex]>>NUM_COEFF_SHIFT) > 384 );
      
      }

      if (m_apaucLumaCoefMap[uiScanIndex][uiBlockIndex] & SIGNIFICANT)
        break;
      uiScanIndex++;
      while ((uiScanIndex < 16) && (m_apaucLumaCoefMap[uiScanIndex][uiBlockIndex] & SIGNIFICANT))
        uiScanIndex++;
    } // while
    // ==
  }
  
  return Err::m_nOK;
}



ErrVal
RQFGSDecoder::xDecodeNewCoefficientChromaDC ( UInt    uiPlane,
                                              UInt    uiMbYIdx,
                                              UInt    uiMbXIdx,
                                              UInt&   uiComplete,
                                              Int&    riLastQP )
{
  UInt uiMbIndex = uiMbYIdx * m_uiWidthInMB + uiMbXIdx;


  UInt    uiDCIdx    = 4;
  UInt    uiStartIdx = 4;
  UInt    uiIndex;
  for( uiIndex = 0; uiIndex < 4; uiIndex++ )
  {
    if( ! ( m_aapaucChromaDCCoefMap[uiPlane][uiIndex][uiMbIndex] & SIGNIFICANT) || ( m_aapaucChromaDCCoefMap[uiPlane][uiIndex][uiMbIndex] & CODED ) )
    {
      uiStartIdx = uiIndex;
      break;
    }
  }
  for( uiIndex = 0; uiIndex < 4; uiIndex++ )
  {
    if( ! ( m_aapaucChromaDCCoefMap[uiPlane][uiIndex][uiMbIndex] & (SIGNIFICANT|CODED) ) )
    {
      uiDCIdx = uiIndex;
      break;
    }
  }
  uiComplete &= (uiDCIdx == 4);
  ROTRS(uiDCIdx == 4, Err::m_nOK);

  UInt uiNeedEob = ( uiDCIdx > uiStartIdx );

  MbDataAccess* pcMbDataAccessEL  = 0;
  MbDataAccess* pcMbDataAccessBL  = 0;
  RNOK( m_cMbDataCtrlEL     .initMb( pcMbDataAccessEL, uiMbYIdx, uiMbXIdx ) );
  RNOK( m_pcCurrMbDataCtrl ->initMb( pcMbDataAccessBL, uiMbYIdx, uiMbXIdx ) );
  

  if( ! ( m_pauiMacroblockMap[uiMbIndex] & CHROMA_CBP_CODED ) )
  {
    //===== CHROMA CBP =====
    Bool  bSigCBP = m_pcCabacReader->RQdecodeCBP_Chroma( *pcMbDataAccessEL, *pcMbDataAccessBL );
    m_pauiMacroblockMap[uiMbIndex] |= CHROMA_CBP_CODED;
    
    if( !bSigCBP )
    {
      //----- chroma DC -----
      for( UInt uiCP = 0; uiCP < 2; uiCP++ )
      {
        m_apaucChromaDCBlockMap[uiCP][uiMbIndex] |= CODED;

        for( UInt ui = 0; ui < 4; ui++ )
        {
          if( ! ( m_aapaucChromaDCCoefMap[uiCP][ui][uiMbIndex] & SIGNIFICANT ) )
          {
            m_aapaucChromaDCCoefMap[uiCP][ui][uiMbIndex] |= CODED;
            m_pauiMacroblockMap[uiMbIndex] += ( 1 << NUM_COEFF_SHIFT );
            ROT( (m_pauiMacroblockMap[uiMbIndex]>>NUM_COEFF_SHIFT) > 384 );
          }
        }
      }

      //----- chroma AC -----
      for( CIdx cCIdx; cCIdx.isLegal(); cCIdx++ )
      {
        UInt  uiCP  = cCIdx.plane();
        UInt  uiBlk = ( uiMbYIdx*2 + cCIdx.y() ) * 2 * m_uiWidthInMB + ( uiMbXIdx*2 + cCIdx.x() );

        m_apaucChromaACBlockMap[uiCP][uiBlk] |= CODED;

        for( UInt ui = 1; ui < 16; ui++ )
        {
          if( ! ( m_aapaucChromaACCoefMap[uiCP][ui][uiBlk] & SIGNIFICANT ) )
          {
            m_aapaucChromaACCoefMap[uiCP][ui][uiBlk] |= CODED;
            m_pauiMacroblockMap[uiMbIndex] += ( 1 << NUM_COEFF_SHIFT );
            ROT( (m_pauiMacroblockMap[uiMbIndex]>>NUM_COEFF_SHIFT) > 384 );
          }
        }
      }

      return Err::m_nOK;
    }
    
    //===== DELTA QP =====
    if( ! ( m_pauiMacroblockMap[uiMbIndex] & CODED ) )
    {
      //===== delta QP =====
      if( ! ( m_pauiMacroblockMap[uiMbIndex] & SIGNIFICANT ) )
      {
        pcMbDataAccessEL->setLastQp( riLastQP );
        RNOK( m_pcCabacReader->RQdecodeDeltaQp( *pcMbDataAccessEL, *pcMbDataAccessBL ) );
        riLastQP = pcMbDataAccessEL->getMbData().getQp();

        m_pauiMacroblockMap[uiMbIndex] |= SIGNIFICANT;
      }
    }
  }



  if( ! ( m_apaucChromaDCBlockMap[uiPlane][uiMbIndex] & CODED ) )
  {
    Bool bSigBCBP = m_pcCabacReader->RQdecodeBCBP_ChromaDC( *pcMbDataAccessEL, *pcMbDataAccessBL, CIdx(4*uiPlane) );
    m_apaucChromaDCBlockMap[uiPlane][uiMbIndex] |= CODED;
    if(  bSigBCBP )
    {
      m_apaucChromaDCBlockMap[uiPlane][uiMbIndex] |= SIGNIFICANT;
    }
    if( ! bSigBCBP )
    {
      for( UInt ui = 0; ui < 4; ui++ )
      {
        if( ! ( m_aapaucChromaDCCoefMap[uiPlane][ui][uiMbIndex] & SIGNIFICANT ) )
        {
          m_aapaucChromaDCCoefMap[uiPlane][ui][uiMbIndex] |= CODED;
          m_pauiMacroblockMap[uiMbIndex] += ( 1 << NUM_COEFF_SHIFT );
          ROT( (m_pauiMacroblockMap[uiMbIndex]>>NUM_COEFF_SHIFT) > 384 );
        }
      }
      return Err::m_nOK;
    }
  }


  // Encode EOB marker?
  while (uiDCIdx < 4) {
    RNOK( m_pcCabacReader->RQdecodeNewTCoeff_Chroma( *pcMbDataAccessEL, *pcMbDataAccessBL,
                                                    CHROMA_DC, CIdx(4*uiPlane), uiDCIdx, uiNeedEob ) );
    if (uiNeedEob) {
      for( UInt ui = uiDCIdx; ui < 4; ui++ )
      {
        if( ! ( m_aapaucChromaDCCoefMap[uiPlane][ui][uiMbIndex] & SIGNIFICANT ) )
        {
          m_aapaucChromaDCCoefMap[uiPlane][ui][uiMbIndex] |= CODED;
          m_pauiMacroblockMap[uiMbIndex] += ( 1 << NUM_COEFF_SHIFT );
          ROT( (m_pauiMacroblockMap[uiMbIndex]>>NUM_COEFF_SHIFT) > 384 );
        }
      }
      break;
    } else {
      uiNeedEob = 0;
      m_aapaucChromaDCCoefMap[uiPlane][uiDCIdx][uiMbIndex] |= CODED;
      if( pcMbDataAccessEL->getMbTCoeffs().get( CIdx(4*uiPlane) )[g_aucIndexChromaDCScan[uiDCIdx]] )
      {
        m_aapaucChromaDCCoefMap[uiPlane][uiDCIdx][uiMbIndex] |= SIGNIFICANT;
      }
      m_pauiMacroblockMap[uiMbIndex] += ( 1 << NUM_COEFF_SHIFT );
      ROT( (m_pauiMacroblockMap[uiMbIndex]>>NUM_COEFF_SHIFT) > 384 );
    }

    if ( m_aapaucChromaDCCoefMap[uiPlane][uiDCIdx][uiMbIndex] & SIGNIFICANT)
      break;
    uiDCIdx++;
    while ((uiDCIdx < 4) && (m_aapaucChromaDCCoefMap[uiPlane][uiDCIdx][uiMbIndex] & SIGNIFICANT))
      uiDCIdx++;
  } // while
  // ==
  
  return Err::m_nOK;
}






ErrVal
RQFGSDecoder::xDecodeNewCoefficientChromaAC ( UInt    uiPlane,
                                              UInt    uiB8YIdx,
                                              UInt    uiB8XIdx,
                                              UInt&   uiComplete,
                                              Int&    riLastQP )
{
  UInt uiB8Index  = (uiB8YIdx  ) * 2 * m_uiWidthInMB + (uiB8XIdx  );
  UInt uiMbIndex  = (uiB8YIdx/2) * 1 * m_uiWidthInMB + (uiB8XIdx/2);
  UInt uiCIdx     = 4*uiPlane + 2*(uiB8YIdx%2) + (uiB8XIdx%2);

  CIdx cChromaIdx(uiCIdx);


  UInt    uiScanIndex  = 16;
  UInt    uiStartIndex = 1;
  UInt    uiIndex;
  for( uiIndex = 1; uiIndex < 16; uiIndex++ )
  {
    if( !( m_aapaucChromaACCoefMap[uiPlane][uiIndex][uiB8Index] & SIGNIFICANT ) || ( m_aapaucChromaACCoefMap[uiPlane][uiIndex][uiB8Index] & CODED ) )
    {
      uiStartIndex = uiIndex;
      break;
    }
  }
  for( uiIndex = 1; uiIndex < 16; uiIndex++ )
  {
    if( !( m_aapaucChromaACCoefMap[uiPlane][uiIndex][uiB8Index] & (SIGNIFICANT|CODED) ) )
    {
      uiScanIndex = uiIndex;
      break;
    }
  }
  uiComplete &= (uiScanIndex == 16);
  ROTRS(uiScanIndex == 16, Err::m_nOK);

  UInt uiNeedEob = (uiScanIndex > uiStartIndex );


  MbDataAccess* pcMbDataAccessEL  = 0;
  MbDataAccess* pcMbDataAccessBL  = 0;
  RNOK( m_cMbDataCtrlEL     .initMb( pcMbDataAccessEL, uiB8YIdx/2, uiB8XIdx/2 ) );
  RNOK( m_pcCurrMbDataCtrl ->initMb( pcMbDataAccessBL, uiB8YIdx/2, uiB8XIdx/2 ) );
  

  if( ! ( m_pauiMacroblockMap[uiMbIndex] & CHROMA_CBP_CODED ) )
  {
    //===== CHROMA CBP =====
    Bool  bSigCBP = m_pcCabacReader->RQdecodeCBP_Chroma( *pcMbDataAccessEL, *pcMbDataAccessBL );
    m_pauiMacroblockMap[uiMbIndex] |= CHROMA_CBP_CODED;
    
    if( !bSigCBP )
    {
      //----- chroma DC -----
      for( UInt uiCP = 0; uiCP < 2; uiCP++ )
      {
        m_apaucChromaDCBlockMap[uiCP][uiMbIndex] |= CODED;

        for( UInt ui = 0; ui < 4; ui++ )
        {
          if( ! ( m_aapaucChromaDCCoefMap[uiCP][ui][uiMbIndex] & SIGNIFICANT ) )
          {
            m_aapaucChromaDCCoefMap[uiCP][ui][uiMbIndex] |= CODED;
            m_pauiMacroblockMap[uiMbIndex] += ( 1 << NUM_COEFF_SHIFT );
            ROT( (m_pauiMacroblockMap[uiMbIndex]>>NUM_COEFF_SHIFT) > 384 );
          }
        }
      }

      //----- chroma AC -----
      for( CIdx cCIdx; cCIdx.isLegal(); cCIdx++ )
      {
        UInt  uiCP  = cCIdx.plane();
        UInt  uiBlk = ( (uiB8YIdx/2)*2 + cCIdx.y() ) * 2 * m_uiWidthInMB + ( (uiB8XIdx/2)*2 + cCIdx.x() );

        m_apaucChromaACBlockMap[uiCP][uiBlk] |= CODED;

        for( UInt ui = 1; ui < 16; ui++ )
        {
          if( ! ( m_aapaucChromaACCoefMap[uiCP][ui][uiBlk] & SIGNIFICANT ) )
          {
            m_aapaucChromaACCoefMap[uiCP][ui][uiBlk] |= CODED;
            m_pauiMacroblockMap[uiMbIndex] += ( 1 << NUM_COEFF_SHIFT );
            ROT( (m_pauiMacroblockMap[uiMbIndex]>>NUM_COEFF_SHIFT) > 384 );
          }
        }
      }

      return Err::m_nOK;
    }
    
    //===== DELTA QP =====
    if( ! ( m_pauiMacroblockMap[uiMbIndex] & CODED ) )
    {
      //===== delta QP =====
      if( ! ( m_pauiMacroblockMap[uiMbIndex] & SIGNIFICANT ) )
      {
        pcMbDataAccessEL->setLastQp( riLastQP );
        RNOK( m_pcCabacReader->RQdecodeDeltaQp( *pcMbDataAccessEL, *pcMbDataAccessBL ) );
        riLastQP = pcMbDataAccessEL->getMbData().getQp();

        m_pauiMacroblockMap[uiMbIndex] |= SIGNIFICANT;
      }
    }
  }



  if( ! ( m_pauiMacroblockMap[uiMbIndex] & CHROMA_CBP_AC_CODED ) )
  {
    //===== CHROMA CBP =====
    Bool  bSigCBP = m_pcCabacReader->RQdecodeCBP_ChromaAC( *pcMbDataAccessEL, *pcMbDataAccessBL );
    m_pauiMacroblockMap[uiMbIndex] |= CHROMA_CBP_AC_CODED;
    
    if( !bSigCBP )
    {
      for( CIdx cCIdx; cCIdx.isLegal(); cCIdx++ )
      {
        UInt  uiCP  = cCIdx.plane();
        UInt  uiBlk = ( (uiB8YIdx/2)*2 + cCIdx.y() ) * 2 * m_uiWidthInMB + ( (uiB8XIdx/2)*2 + cCIdx.x() );

        m_apaucChromaACBlockMap[uiCP][uiBlk] |= CODED;

        for( UInt ui = 1; ui < 16; ui++ )
        {
          if( ! ( m_aapaucChromaACCoefMap[uiCP][ui][uiBlk] & SIGNIFICANT ) )
          {
            m_aapaucChromaACCoefMap[uiCP][ui][uiBlk] |= CODED;
            m_pauiMacroblockMap[uiMbIndex] += ( 1 << NUM_COEFF_SHIFT );
            ROT( (m_pauiMacroblockMap[uiMbIndex]>>NUM_COEFF_SHIFT) > 384 );
          }
        }
      }

      return Err::m_nOK;
    }
  }



  if( ! ( m_apaucChromaACBlockMap[uiPlane][uiB8Index] & CODED ) )
  {
    Bool bSigBCBP = m_pcCabacReader->RQdecodeBCBP_ChromaAC( *pcMbDataAccessEL, *pcMbDataAccessBL, cChromaIdx );
    m_apaucChromaACBlockMap[uiPlane][uiB8Index] |= CODED;
    if(  bSigBCBP )
    {
      m_apaucChromaACBlockMap[uiPlane][uiB8Index] |= SIGNIFICANT;
    }
    if( ! bSigBCBP )
    {
      for( UInt ui = 1; ui < 16; ui++ )
      {
        if( ! ( m_aapaucChromaACCoefMap[uiPlane][ui][uiB8Index] & SIGNIFICANT ) )
        {
          m_aapaucChromaACCoefMap[uiPlane][ui][uiB8Index] |= CODED;
          m_pauiMacroblockMap[uiMbIndex] += ( 1 << NUM_COEFF_SHIFT );
          ROT( (m_pauiMacroblockMap[uiMbIndex]>>NUM_COEFF_SHIFT) > 384 );
        }
      }
      return Err::m_nOK;
    }
  }


  while (uiScanIndex < 16) {

    RNOK( m_pcCabacReader->RQdecodeNewTCoeff_Chroma( *pcMbDataAccessEL, *pcMbDataAccessBL,
                                                    CHROMA_AC, cChromaIdx, uiScanIndex, uiNeedEob ) );
    if (uiNeedEob) {
      for( UInt ui = uiScanIndex; ui < 16; ui++ )
      {
        if( ! ( m_aapaucChromaACCoefMap[uiPlane][ui][uiB8Index] & SIGNIFICANT ) )
        {
          m_aapaucChromaACCoefMap[uiPlane][ui][uiB8Index] |= CODED;
          m_pauiMacroblockMap[uiMbIndex] += ( 1 << NUM_COEFF_SHIFT );
          ROT( (m_pauiMacroblockMap[uiMbIndex]>>NUM_COEFF_SHIFT) > 384 );
        }
      }
      break;  

    } else {

      uiNeedEob = 0;

      m_aapaucChromaACCoefMap[uiPlane][uiScanIndex][uiB8Index] |= CODED;
      if( pcMbDataAccessEL->getMbTCoeffs().get( cChromaIdx )[g_aucFrameScan[uiScanIndex]] )
      {
        m_aapaucChromaACCoefMap[uiPlane][uiScanIndex][uiB8Index] |= SIGNIFICANT;
      }
      m_pauiMacroblockMap[uiMbIndex] += ( 1 << NUM_COEFF_SHIFT );
      ROT( (m_pauiMacroblockMap[uiMbIndex]>>NUM_COEFF_SHIFT) > 384 );
    }
    
    if (m_aapaucChromaACCoefMap[uiPlane][uiScanIndex][uiB8Index] & SIGNIFICANT)
      break;
    uiScanIndex++;
    while ((uiScanIndex < 16) && (m_aapaucChromaACCoefMap[uiPlane][uiScanIndex][uiB8Index] & SIGNIFICANT))
      uiScanIndex++;
  } // while
  // ==
  
  return Err::m_nOK;
}







ErrVal
RQFGSDecoder::xDecodeCoefficientLumaRef( UInt   uiBlockYIndex,
                                         UInt   uiBlockXIndex,
                                         UInt   uiScanIndex )
{
  UInt    uiMbIndex     = (uiBlockYIndex/4) * 1 * m_uiWidthInMB + (uiBlockXIndex/4);
  UInt    uiBlockIndex  =  uiBlockYIndex    * 4 * m_uiWidthInMB +  uiBlockXIndex;

  UInt    uiB8x8        = ((uiBlockYIndex%4)/2) * 2 + ((uiBlockXIndex%4)/2);
  UInt    uiB4x4        =  (uiBlockYIndex%4)    * 4 +  (uiBlockXIndex%4);
  Par8x8  ePar8x8       = Par8x8(uiB8x8);
  
  B8x8Idx c8x8Idx(ePar8x8);
  B4x4Idx c4x4Idx(uiB4x4);

  //===== check if coefficient is not significant or was already coded =====
  ROFRS( m_apaucLumaCoefMap[uiScanIndex][uiBlockIndex] & SIGNIFICANT, Err::m_nOK );
  ROTRS( m_apaucLumaCoefMap[uiScanIndex][uiBlockIndex] & CODED,       Err::m_nOK );

  MbDataAccess* pcMbDataAccessEL  = 0;
  MbDataAccess* pcMbDataAccessBL  = 0;
  RNOK( m_cMbDataCtrlEL     .initMb( pcMbDataAccessEL, uiBlockYIndex/4, uiBlockXIndex/4 ) );
  RNOK( m_pcCurrMbDataCtrl ->initMb( pcMbDataAccessBL, uiBlockYIndex/4, uiBlockXIndex/4 ) );
  

  if( pcMbDataAccessBL->getMbData().isTransformSize8x8() )
  {
    UInt  ui8x8ScanIndex  = 4*uiScanIndex + 2*( uiBlockYIndex % 2 ) + ( uiBlockXIndex % 2 );
    RNOK( m_pcCabacReader->RQdecodeTCoeffRef_8x8( *pcMbDataAccessEL, *pcMbDataAccessBL,
                                                  c8x8Idx, ui8x8ScanIndex ) );    
  }
  else
  {
    RNOK( m_pcCabacReader->RQdecodeTCoeffRef_Luma( *pcMbDataAccessEL, *pcMbDataAccessBL,
                                                   LUMA_SCAN, c4x4Idx, uiScanIndex ) );
  }

  m_apaucLumaCoefMap[uiScanIndex][uiBlockIndex] |= CODED;
  m_pauiMacroblockMap[uiMbIndex] += ( 1 << NUM_COEFF_SHIFT );
  ROT( (m_pauiMacroblockMap[uiMbIndex]>>NUM_COEFF_SHIFT) > 384 );

  return Err::m_nOK;
}


ErrVal
RQFGSDecoder::xDecodeCoefficientChromaDCRef ( UInt  uiPlane,
                                              UInt  uiMbYIdx,
                                              UInt  uiMbXIdx,
                                              UInt  uiDCIdx )
{
  UInt  uiMbIndex = uiMbYIdx * m_uiWidthInMB + uiMbXIdx;

  //===== check if coefficient is not significant or was already coded =====
  ROFRS( m_aapaucChromaDCCoefMap[uiPlane][uiDCIdx][uiMbIndex] & SIGNIFICANT, Err::m_nOK );
  ROTRS( m_aapaucChromaDCCoefMap[uiPlane][uiDCIdx][uiMbIndex] & CODED,       Err::m_nOK );

  MbDataAccess* pcMbDataAccessEL  = 0;
  MbDataAccess* pcMbDataAccessBL  = 0;
  RNOK( m_cMbDataCtrlEL     .initMb( pcMbDataAccessEL, uiMbYIdx, uiMbXIdx ) );
  RNOK( m_pcCurrMbDataCtrl ->initMb( pcMbDataAccessBL, uiMbYIdx, uiMbXIdx ) );
  

  RNOK( m_pcCabacReader->RQdecodeTCoeffRef_Chroma( *pcMbDataAccessEL, *pcMbDataAccessBL,
                                                   CHROMA_DC, CIdx(4*uiPlane), uiDCIdx ) );

  m_aapaucChromaDCCoefMap[uiPlane][uiDCIdx][uiMbIndex] |= CODED;
  m_pauiMacroblockMap[uiMbIndex] += ( 1 << NUM_COEFF_SHIFT );
  ROT( (m_pauiMacroblockMap[uiMbIndex]>>NUM_COEFF_SHIFT) > 384 );

  return Err::m_nOK;
}
 

ErrVal
RQFGSDecoder::xDecodeCoefficientChromaACRef ( UInt  uiPlane,
                                              UInt  uiB8YIdx,
                                              UInt  uiB8XIdx,
                                              UInt  uiScanIdx )
{
  UInt  uiB8Index   = (uiB8YIdx  ) * 2 * m_uiWidthInMB + (uiB8XIdx  );
  UInt  uiMbIndex   = (uiB8YIdx/2) * 1 * m_uiWidthInMB + (uiB8XIdx/2);
  UInt  uiChromaIdx = 4*uiPlane + 2*(uiB8YIdx%2) + (uiB8XIdx%2);
  
  CIdx  cChromaIdx(uiChromaIdx);

  //===== check if coefficient is not significant or was already coded =====
  ROFRS( m_aapaucChromaACCoefMap[uiPlane][uiScanIdx][uiB8Index] & SIGNIFICANT, Err::m_nOK );
  ROTRS( m_aapaucChromaACCoefMap[uiPlane][uiScanIdx][uiB8Index] & CODED,       Err::m_nOK );

  MbDataAccess* pcMbDataAccessEL  = 0;
  MbDataAccess* pcMbDataAccessBL  = 0;
  RNOK( m_cMbDataCtrlEL     .initMb( pcMbDataAccessEL, uiB8YIdx/2, uiB8XIdx/2 ) );
  RNOK( m_pcCurrMbDataCtrl ->initMb( pcMbDataAccessBL, uiB8YIdx/2, uiB8XIdx/2 ) );
  

  RNOK( m_pcCabacReader->RQdecodeTCoeffRef_Chroma( *pcMbDataAccessEL, *pcMbDataAccessBL,
                                                   CHROMA_AC, cChromaIdx, uiScanIdx ) );

  m_aapaucChromaACCoefMap[uiPlane][uiScanIdx][uiB8Index] |= CODED;
  m_pauiMacroblockMap[uiMbIndex] += ( 1 << NUM_COEFF_SHIFT );
  ROT( (m_pauiMacroblockMap[uiMbIndex]>>NUM_COEFF_SHIFT) > 384 );

  return Err::m_nOK;
}




ErrVal
RQFGSDecoder::xUpdateMacroblock( MbDataAccess&  rcMbDataAccessBL,
                                 MbDataAccess&  rcMbDataAccessEL,
                                 UInt           uiMbY,
                                 UInt           uiMbX )
{
  UInt  uiExtCbp  = 0;
  Bool  b8x8      = rcMbDataAccessBL.getMbData().isTransformSize8x8();
  UInt  uiMbIndex = uiMbY*m_uiWidthInMB + uiMbX;

  //===== luma =====
  if( b8x8 )
  {
    for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
    {
      Bool    bSig      = false;
      TCoeff* piCoeffBL = rcMbDataAccessBL.getMbTCoeffs().get8x8( c8x8Idx );
      TCoeff* piCoeffEL = rcMbDataAccessEL.getMbTCoeffs().get8x8( c8x8Idx );

      for( UInt ui8x8ScanIdx = 0; ui8x8ScanIdx < 64; ui8x8ScanIdx++ )
      {
        UInt  uiPos         = g_aucFrameScan64[ui8x8ScanIdx];
        UInt  ui4x4ScanIdx  = ui8x8ScanIdx / 4;
        UInt  uiBlk4x4Idx   = ui8x8ScanIdx % 4;
        UInt  uiBlockIndex  = (4*uiMbY+c8x8Idx.y()+(uiBlk4x4Idx/2))*4*m_uiWidthInMB + (4*uiMbX+c8x8Idx.x()+(uiBlk4x4Idx%2));

        if( m_apaucLumaCoefMap[ui4x4ScanIdx][uiBlockIndex] & CODED )
        {
          piCoeffBL[uiPos] += piCoeffEL[uiPos];
        }
        if( piCoeffBL[uiPos] )
        {
          bSig = true;
        }
      }
      if( bSig )
      {
        uiExtCbp |= ( 0x33 << c8x8Idx.b4x4() );
      }
    }
  }
  else
  {
    for( B4x4Idx c4x4Idx; c4x4Idx.isLegal(); c4x4Idx++ )
    {
      Bool    bSig      = false;
      TCoeff* piCoeffBL = rcMbDataAccessBL.getMbTCoeffs().get( c4x4Idx );
      TCoeff* piCoeffEL = rcMbDataAccessEL.getMbTCoeffs().get( c4x4Idx );

      for( UInt uiScanIdx = 0; uiScanIdx < 16; uiScanIdx++ )
      {
        UInt  uiPos         = g_aucFrameScan[uiScanIdx];
        UInt  uiBlockIndex  = (4*uiMbY+c4x4Idx.y())*4*m_uiWidthInMB + (4*uiMbX+c4x4Idx.x());

        if( m_apaucLumaCoefMap[uiScanIdx][uiBlockIndex] & CODED )
        {
          piCoeffBL[uiPos] += piCoeffEL[uiPos];
        }
        if( piCoeffBL[uiPos] )
        {
          bSig = true;
        }
      }
      if( bSig )
      {
        uiExtCbp |= ( 1 << c4x4Idx );
      }
    }
  }


  //===== chroma DC =====
  Bool  bSigDC = false;
  for( UInt uiPlane = 0; uiPlane < 2; uiPlane++ )
  {
    TCoeff* piCoeffBL = rcMbDataAccessBL.getMbTCoeffs().get( CIdx(4*uiPlane) );
    TCoeff* piCoeffEL = rcMbDataAccessEL.getMbTCoeffs().get( CIdx(4*uiPlane) );

    for( UInt uiDCIdx = 0; uiDCIdx < 4; uiDCIdx++ )
    {
      UInt uiPos = g_aucIndexChromaDCScan[uiDCIdx];

      piCoeffBL[uiPos] += piCoeffEL[uiPos];

      if( piCoeffBL[uiPos] )
      {
        bSigDC = true;
      }
    }
  }

  //===== chroma AC =====
  Bool  bSigAC = false;
  for( CIdx cCIdx; cCIdx.isLegal(); cCIdx++ )
  {
    TCoeff* piCoeffBL = rcMbDataAccessBL.getMbTCoeffs().get( cCIdx );
    TCoeff* piCoeffEL = rcMbDataAccessEL.getMbTCoeffs().get( cCIdx );

    for( UInt uiScanIdx = 1; uiScanIdx < 16; uiScanIdx++ )
    {
      UInt  uiPos     = g_aucFrameScan[uiScanIdx];
      UInt  ui8x8Idx  = (2*uiMbY+cCIdx.y())*2*m_uiWidthInMB + (2*uiMbX+cCIdx.x());

      if( m_aapaucChromaACCoefMap[cCIdx.plane()][uiScanIdx][ui8x8Idx] & CODED )
      {
        piCoeffBL[uiPos] += piCoeffEL[uiPos];
      }
      if( piCoeffBL[uiPos] )
      {
        bSigAC = true;
      }
    }
  }

  
  //===== set CBP =====
  UInt  uiChromaCBP = ( bSigAC ? 2 : bSigDC ? 1 : 0 );
  uiExtCbp         |= ( uiChromaCBP << 16 );
  rcMbDataAccessBL.getMbData().setAndConvertMbExtCbp( uiExtCbp );


  //===== set QP =====
  Int iELQP     = rcMbDataAccessEL.getMbData().getQp();
  Int iNumCoded = ( m_pauiMacroblockMap[uiMbIndex] >> NUM_COEFF_SHIFT );
  Int iQPDelta  = ( 384 - iNumCoded ) / 64;
  Int iQP       = min( 51, iELQP + iQPDelta );
  if( ! ( m_pauiMacroblockMap[uiMbIndex] & SIGNIFICANT ) )
  {
    iQP = rcMbDataAccessEL.getSH().getPicQp();
  }
  rcMbDataAccessBL.getMbData().setQp( iQP );

  return Err::m_nOK;
}



H264AVC_NAMESPACE_END

