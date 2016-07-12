#include "imageLoader.h"


int ImageLoader::createJPG(const char* strFileName)
{
	unsigned int textureID;
	
	
	
	/////// * /////////// * /////////// * NEW * /////// * /////////// * /////////// *	
	
	tImageJPG *pImage = LoadJPG(strFileName);			// Load the image and store the data
	
	/////// * /////////// * /////////// * NEW * /////// * /////////// * /////////// *
	
	
	//	if(pImage == NULL)									// If we can't load the file, quit!
	//		exit(0);
	
	// Generate a texture with the associative texture ID stored in the array
	glGenTextures(1, &textureID);
	
	// Bind the texture to the texture arrays index and init the texture
	glBindTexture(GL_TEXTURE_2D, textureID);

	unsigned char* image = new unsigned char[pImage->sizeX*pImage->sizeY*4];
	int count=0;
	for(int i=0;i<pImage->sizeY;i++)
	{
		for(int j=0;j<pImage->sizeX;j++)
		{
			count = (i*pImage->sizeX+j)*3;
			image[(i*pImage->sizeX+j)*4] = pImage->data[count];
			image[(i*pImage->sizeX+j)*4+1] = pImage->data[count+1];
			image[(i*pImage->sizeX+j)*4+2] = pImage->data[count+2];
			image[(i*pImage->sizeX+j)*4+3] = 255;			
		}
	}
	
	// Build Mipmaps (builds different versions of the picture for distances - looks better)
	gluBuild2DMipmaps(GL_TEXTURE_2D, 4, pImage->sizeX, pImage->sizeY, GL_RGBA, GL_UNSIGNED_BYTE, image);
	
	// Build Mipmaps (builds different versions of the picture for distances - looks better)
	//gluBuild2DMipmaps(GL_TEXTURE_2D, 3, pImage->sizeX, pImage->sizeY, GL_RGB, GL_UNSIGNED_BYTE, pImage->data);
	
	// Lastly, we need to tell OpenGL the quality of our texture map.  GL_LINEAR_MIPMAP_LINEAR
	// is the smoothest.  GL_LINEAR_MIPMAP_NEAREST is faster than GL_LINEAR_MIPMAP_LINEAR, 
	// but looks blochy and pixilated.  Good for slower computers though. 
	
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR_MIPMAP_LINEAR);	
	
	
	// Now we need to free the image data that we loaded since OpenGL stored it as a texture
	
	if (pImage)										// If we loaded the image
	{
		if (pImage->data)							// If there is texture data
		{
			free(pImage->data);						// Free the texture data, we don't need it anymore
		}
		
		free(pImage);								// Free the image structure
	}

	delete[] image;
	
	return textureID;
}

tImageJPG* ImageLoader::LoadJPG(const char *filename)
{
	struct jpeg_decompress_struct cinfo;
	tImageJPG *pImageData = NULL;
	FILE *pFile = NULL;
	
	// This is the only function you should care about.  You don't need to
	// really know what all of this does (since you can't cause it's a library!) :)
	// Just know that you need to pass in the jpeg file name, and get a pointer
	// to a tImageJPG structure which contains the width, height and pixel data.
	// Be sure to free the data after you are done with it, just like a bitmap.
	
	// Open a file pointer to the jpeg file and check if it was found and opened 
	//if((pFile = fopen(filename, "rb")) == NULL) 
	fopen_s(&pFile,filename, "rb"); 
	if(pFile == NULL) 
	{
		// Display an error message saying the file was not found, then return NULL
		//	MessageBox(g_hWnd, "Unable to load JPG File!", "Error", MB_OK);
			return NULL;
	}
	
	// Create an error handler
	jpeg_error_mgr jerr;	
	
	// Have our compression info object point to the error handler address
	cinfo.err = jpeg_std_error(&jerr);
	
	// Initialize the decompression object
	jpeg_create_decompress(&cinfo);
	
	// Specify the data source (Our file pointer)	
	jpeg_stdio_src(&cinfo, pFile);
	
	// Allocate the structure that will hold our eventual jpeg data (must free it!)
	pImageData = (tImageJPG*)malloc(sizeof(tImageJPG));
	
	// Decode the jpeg file and fill in the image data structure to pass back
	DecodeJPG(&cinfo, pImageData);
	
	// This releases all the stored memory for reading and decoding the jpeg
	jpeg_destroy_decompress(&cinfo);
	
	// Close the file pointer that opened the file
	fclose(pFile);
	
	// Return the jpeg data (remember, you must free this data after you are done)
	return pImageData;
	//return NULL;
}

void ImageLoader::DecodeJPG(jpeg_decompress_struct* cinfo, tImageJPG *pImageData)
{
	
	// Read in the header of the jpeg file
	jpeg_read_header(cinfo, TRUE);
	
	// Start to decompress the jpeg file with our compression info
	jpeg_start_decompress(cinfo);
	
	// Get the image dimensions and row span to read in the pixel data
	pImageData->rowSpan = cinfo->image_width * cinfo->num_components;
	pImageData->sizeX   = cinfo->image_width;
	pImageData->sizeY   = cinfo->image_height;
	
	// Allocate memory for the pixel buffer
	pImageData->data = new unsigned char[pImageData->rowSpan * pImageData->sizeY];
	
	// Here we use the library's state variable cinfo.output_scanline as the
	// loop counter, so that we don't have to keep track ourselves.
	
	// Create an array of row pointers
	unsigned char** rowPtr = new unsigned char*[pImageData->sizeY];
	for (int i = 0; i < pImageData->sizeY; i++)
		rowPtr[i] = &(pImageData->data[i*pImageData->rowSpan]);
	
	// Now comes the juice of our work, here we extract all the pixel data
	int rowsRead = 0;
	while (cinfo->output_scanline < cinfo->output_height) 
	{
		// Read in the current row of pixels and increase the rowsRead count
		rowsRead += jpeg_read_scanlines(cinfo, &rowPtr[rowsRead], cinfo->output_height - rowsRead);
	}
	
	// Delete the temporary row pointers
	delete [] rowPtr;
	
	// Finish decompressing the data
	jpeg_finish_decompress(cinfo);
}