#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <GL/glext.h>
#include <iostream>


#include <vector>
#include "imageLoader.h"

/* needed for DirectX's DDSURFACEDESC2 structure definition */
#include <ddraw.h>

/* ARB_texture_compression */
/*PFNGLCOMPRESSEDTEXIMAGE3DARBPROC    glCompressedTexImage3DARB;
PFNGLCOMPRESSEDTEXIMAGE2DARBPROC    glCompressedTexImage2DARB;
PFNGLCOMPRESSEDTEXIMAGE1DARBPROC    glCompressedTexImage1DARB;
PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC glCompressedTexSubImage3DARB;
PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC glCompressedTexSubImage2DARB;
PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC glCompressedTexSubImage1DARB;
PFNGLGETCOMPRESSEDTEXIMAGEARBPROC   glGetCompressedTexImageARB; */

void ImageLoader::GLErrorReport()
{
  GLuint errnum;
  const char *errstr;
  while (errnum = glGetError()) 
  {
    errstr = (char *) gluErrorString(errnum);
    printf(errstr);
  }
  return;
}

/*//using the old dd surface description, it has been superceeded by DDSURFACEDESC2
typedef unsigned int UI32;
typedef int I32;

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
    ((UI32)(char)(ch0) | ((UI32)(char)(ch1) << 8) |   \
    ((UI32)(char)(ch2) << 16) | ((UI32)(char)(ch3) << 24 ))
#endif //defined(MAKEFOURCC)


#define FOURCC_DXT1  (MAKEFOURCC('D','X','T','1'))
#define FOURCC_DXT2  (MAKEFOURCC('D','X','T','2'))
#define FOURCC_DXT3  (MAKEFOURCC('D','X','T','3'))
#define FOURCC_DXT4  (MAKEFOURCC('D','X','T','4'))
#define FOURCC_DXT5  (MAKEFOURCC('D','X','T','5'))*/

gliGenericImage* ImageLoader::readDDSFile(const char *filename, int & bufsize, int & numMipmaps) 
{
  gliGenericImage *genericImage;
  DDSURFACEDESC2 ddsd;
  //DDSURFACEDESC ddsd;
  char filecode[4];
  FILE *fp;
  int factor;

  // try to open the file 
  fp = fopen(filename, "rb");
  if (fp == NULL)
    return NULL;
  
  // verify the type of file
  fread(filecode, 1, 4, fp);
  if (strncmp(filecode, "DDS ", 4) != 0) {
    fclose(fp);
    return NULL;
  }
   
  // get the surface desc
  fread(&ddsd, sizeof(ddsd), 1, fp);

  genericImage = (gliGenericImage*) malloc(sizeof(gliGenericImage));
  memset(genericImage,0,sizeof(gliGenericImage));

  switch(ddsd.ddpfPixelFormat.dwFourCC)
  {
    case FOURCC_DXT1:
      genericImage->format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
      factor = 2;
      break;
    case FOURCC_DXT3:
      genericImage->format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
      factor = 4;
      break;
    case FOURCC_DXT5:
      genericImage->format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
      factor = 4;
      break;
    default:
      return NULL;
  }

  // how big is it going to be including all mipmaps?
  bufsize = ddsd.dwMipMapCount > 1 ? ddsd.dwLinearSize * factor : ddsd.dwLinearSize;
  genericImage->pixels = (unsigned char*)malloc(bufsize * sizeof(unsigned char));
  fread(genericImage->pixels, 1, bufsize, fp);
  // close the file pointer
  fclose(fp);
  
  genericImage->width       = ddsd.dwWidth;
  genericImage->height      = ddsd.dwHeight;
  genericImage->components  = (ddsd.ddpfPixelFormat.dwFourCC == FOURCC_DXT1) ? 3 : 4;
  numMipmaps = ddsd.dwMipMapCount;

  // return data */
  return genericImage; 
} 

void ImageLoader::loadExtensions()
{
//ARB_texture_compression
  glCompressedTexImage3DARB    =  (PFNGLCOMPRESSEDTEXIMAGE3DARBPROC) wglGetProcAddress("glCompressedTexImage3DARB");
  glCompressedTexImage2DARB    =  (PFNGLCOMPRESSEDTEXIMAGE2DARBPROC) wglGetProcAddress("glCompressedTexImage2DARB");
  glCompressedTexImage1DARB    =  (PFNGLCOMPRESSEDTEXIMAGE1DARBPROC) wglGetProcAddress("glCompressedTexImage1DARB");
  glCompressedTexSubImage3DARB =  (PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC) wglGetProcAddress("glCompressedTexSubImage3DARB");
  glCompressedTexSubImage2DARB =  (PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC) wglGetProcAddress("glCompressedTexSubImage2DARB");
  glCompressedTexSubImage1DARB =  (PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC) wglGetProcAddress("glCompressedTexSubImage1DARB");
  glGetCompressedTexImageARB   =  (PFNGLGETCOMPRESSEDTEXIMAGEARBPROC)  wglGetProcAddress("glGetCompressedTexImageARB"); 
}


GLuint ImageLoader::loadDDS(const char *fileName)
{
	int ddsbufsize;
	gliGenericImage * ddsimage = NULL;
	int i;
	int width;
	int height;
	int size;
	int offset;
	int numMipmaps;
	int blockSize;
	GLuint textID;

	static bool extensionsLoaded = false;
	if (!extensionsLoaded) 
	{
		loadExtensions();
		extensionsLoaded = true;
	}

	/* Assume tightly packed textures. */
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	/* load the .dds file */
	ddsimage = readDDSFile(fileName,ddsbufsize,numMipmaps);
	if (!ddsimage)
	{
		std::cout << "Failed to open : " << fileName << std::endl;
		return 0; 
	}
	height = ddsimage->height;
	width = ddsimage->width;
	offset = 0;
	blockSize = (ddsimage->format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16;

	//Generate the Texture ID
	glGenTextures(1, &textID);
	glBindTexture(GL_TEXTURE_2D, textID);


	/* load the mipmaps */
	for (i = 0; i < numMipmaps && (width || height); ++i)
	{
		if (width == 0)
			width = 1;
		if (height == 0)
			height = 1;

		size = ((width+3)/4)*((height+3)/4)*blockSize;

		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);	


		glCompressedTexImage2DARB(GL_TEXTURE_2D, i, ddsimage->format, width, height, 
			0, size, ddsimage->pixels + offset);

		GLErrorReport();
		offset += size;
		width >>= 1;
		height >>= 1;
	} 

	GLErrorReport();

	free(ddsimage->pixels);
	free(ddsimage);

	return textID;
}
