//--------------------------------------------------------------------------------------
//CImageSurface
// Class for storing, manipulating, and copying image data to and from D3D Surfaces
//
//--------------------------------------------------------------------------------------
// (C) 2005 ATI Research, Inc., All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef CIMAGESURFACE_H
#define CIMAGESURFACE_H

#include <math.h>
#include <stdio.h>
#include <assert.h>

//#include "ErrorMsg.h"
//#include "Types.h"
#include "VectorMacros.h"

#ifndef WCHAR 
#define WCHAR wchar_t
#endif //WCHAR 

#ifndef SAFE_DELETE 
#define SAFE_DELETE(p)       { if(p) { delete (p);   (p)=NULL; } }
#endif

#ifndef SAFE_DELETE_ARRAY 
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p); (p)=NULL; } }
#endif


//Data types processed by cube map processor
// note that UNORM data types use the full range 
// of the unsigned integer to represent the range [0, 1] inclusive
// the float16 datatype is stored as D3Ds S10E5 representation
#define CP_VAL_UNORM8       0
#define CP_VAL_UNORM8_BGRA  1
#define CP_VAL_UNORM16     10
#define CP_VAL_FLOAT16     20 
#define CP_VAL_FLOAT32     30


// Type of data used internally by CSurfaceImage
#define CP_ITYPE float

//2D images used to store cube faces for processing, note that this class is 
// meant to facilitate the copying of data to and from D3D surfaces hence the name ImageSurface
class CImageSurface
{
public:
   int m_Width;          //image width
   int m_Height;         //image height
   int m_NumChannels;    //number of channels
   CP_ITYPE *m_ImgData;    //cubemap image data
	 bool m_ImgDataShared;

   CImageSurface(void);
   void Clear(void);
   void Init(int a_Width, int a_Height, int a_NumChannels, bool shared = false );
   
   //copy data from external buffer into this CImageSurface
   void SetImageData(int a_SrcType, int a_SrcNumChannels, int a_SrcPitch, void *a_SrcDataPtr );

   // copy image data from an external buffer and scale and degamma the data
   void SetImageDataClampDegammaScale(int a_SrcType, int a_SrcNumChannels, int a_SrcPitch, void *a_SrcDataPtr, 
       float a_MaxClamp, float a_Degamma, float a_Scale );

   //copy data from this CImageSurface into an external buffer
   void GetImageData(int a_DstType, int a_DstNumChannels, int a_DstPitch, void *a_DstDataPtr );

   //copy image data from an external buffer and scale and gamma the data
   void GetImageDataScaleGamma(int a_DstType, int a_DstNumChannels, int a_DstPitch, void *a_DstDataPtr,
       float a_Scale, float a_Gamma );

   //clear one of the channels in the CSurfaceImage to a particular color
   void ClearChannelConst(int a_ChannelIdx, CP_ITYPE a_ClearColor);

   //various image operations that can be performed on the CImageSurface
   void InPlaceVerticalFlip(void);
   void InPlaceHorizonalFlip(void);
   void InPlaceDiagonalUVFlip(void);

   CP_ITYPE *GetSurfaceTexelPtr(int a_U, int a_V);
   ~CImageSurface();
};


#endif //CIMAGESURFACE_H
