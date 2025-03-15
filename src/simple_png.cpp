/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h> 
#include <stdint.h> 
#include <stdarg.h>


#include <string.h> 
#include <inttypes.h>

#ifdef USE_FPNG

//https://github.com/richgel999/fpng
#include "fpng.h"
#endif


// libpng
#include <png.h>

#include "simple_png.h"


#ifdef USE_FPNG

// CFLAGS_FPNG = -march=native -msse4.1 -mpclmul

int simple_read_argb_from_fpng(const char *filename, uint8_t **out, uint32_t *w, uint32_t *h, char *title)
{
	bool ret;
	uint32_t width, height, channels_in_file;
	uint32_t outsize;
	std::vector<uint8_t> buf;

	ret = fpng::fpng_decode_file(filename, buf, width, height, channels_in_file, 4);

	printf("%s WxH = %d x %d, channels = %d\n", __func__, width, height, channels_in_file);

	outsize = width * height * 4;

	*w = width;
	*h = height;
	
	if((*out = (uint8_t *)malloc(outsize)) != NULL)
	{
		memcpy(*out, buf.data(), outsize);
	}
	
	return ret;

}

int simple_write_argb_to_fpng(const void* pImage, uint32_t width, uint32_t height, const char * format, ...)
{
  	va_list args;
	char *strp;
	bool ret;
	
	va_start (args, format);
	
	vasprintf (&strp, format, args);
	va_end (args);

	ret = fpng::fpng_encode_image_to_file(strp, pImage, width, height, 4, 0);

	return ret;
}
#endif
//------------------------------------------------------------------------------------------------------------------------------------------

static const char *iTXt_key = "Title";
static char *iTXt_title = NULL;
static int verbose = 1;

int simple_set_text_libpng(const char * format, ...)
{
  	va_list args;

	if(iTXt_title != NULL)  free(iTXt_title);

	
	va_start (args, format);
	
	vasprintf (&iTXt_title, format, args);
	va_end (args);

	printf("%s to %s\n", __func__, iTXt_title);

	return 0;
}

int simple_write_argb_to_libpng(const void* pImage, uint32_t width, uint32_t height, const char * format, ...)
{
  	va_list args;
	char *strp;
	FILE *fpOut;

	int code = 0;
	png_structp png_ptr;
	png_infop info_ptr;
	uint32_t y;
	const uint32_t *pRow;

	va_start (args, format);
	
	vasprintf (&strp, format, args);
	va_end (args);

	fpOut = fopen (strp, "wb");
	
	free(strp);

	if(fpOut == NULL)
	{
		fprintf(stderr, "Could not open file to write !!!!\n");
	}

	// Initialize write structure
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) {
		fprintf(stderr, "Could not allocate write struct\n");
		code = 1;
		goto finalise;
	}

	// Initialize info structure
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		fprintf(stderr, "Could not allocate info struct\n");
		code = 1;
		goto finalise;
	}

	// Setup Exception handling
	if (setjmp(png_jmpbuf(png_ptr))) {
		fprintf(stderr, "Error during png creation\n");
		code = 1;
		goto finalise;
	}


	png_init_io(png_ptr, fpOut);
	

	// Write header (8 bit colour depth)
	png_set_IHDR(png_ptr, info_ptr, width, height,
			8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	fprintf(stdout, "png_init_io.\n");
	// Set title
	if (iTXt_title != NULL) 
	{
		png_text title_text;
		title_text.compression = PNG_TEXT_COMPRESSION_zTXt;
		/* compression value:   -1: tEXt, none,		PNG_TEXT_COMPRESSION_NONE
					             0: zTXt, deflate,	PNG_TEXT_COMPRESSION_zTXt
    				             1: iTXt, none,		PNG_ITXT_COMPRESSION_NONE
					             2: iTXt, deflate  	PNG_ITXT_COMPRESSION_zTXt	*/
		title_text.key = strdup(iTXt_key);
		title_text.text = iTXt_title;
		png_set_text(png_ptr, info_ptr, &title_text, 1);

		free(title_text.key);

		fprintf(stdout, "write iTxt success.\n");
	}

	png_write_info(png_ptr, info_ptr);
	
	// Allocate memory for one row (3 bytes per pixel - RGB)
	//row = (png_bytep) malloc(4 * width * sizeof(png_byte));

	fprintf(stdout, "start to write line .....\n");

	pRow = (const uint32_t *)pImage;
	
	// Write image data
	for (y=0 ; y<height ; y++) 
	{
		png_write_row(png_ptr, (png_const_bytep)pRow);

		pRow += width;
	}

	// End write
	png_write_end(png_ptr, NULL);

finalise:
	if (info_ptr != NULL) png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
	if (png_ptr != NULL) png_destroy_write_struct(&png_ptr, (png_infopp)NULL);

	return code;

}


int simple_read_argb_from_libpng(const char *filename, uint8_t **out, uint32_t *w, uint32_t *h, char **title)
{
	png_FILE_p fpin;
	png_structp png_ptr;
	png_infop info_ptr;
	unsigned int sig_read = 0;
	png_uint_32 width, height;
	int bit_depth, color_type, interlace_type;

	png_alloc_size_t  row_bytes;
	uint8_t *pImage;
	int dec_color_type;
	
	unsigned int number_passes, y;//, iPal, iTrns;
	
	//png_byte SampleRangeLimitTable[ 5 * 256 + PNG_CENTER_SAMPLE ] = {0};
	
	//unsigned char *range_limit;

	//_png_SampleRangeLimitTable_Prepare(SampleRangeLimitTable, &range_limit);


	/* Create and initialize the png_struct with the desired error handler
	 * functions.  If you want to use the default stderr and longjump method,
	 * you can supply NULL for the last three parameters.  We also supply the
	 * the compiler header file version, so that we know if the application
	 * was compiled with a compatible version of the library.  REQUIRED
	 */
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, (png_voidp)NULL, NULL, NULL);
	   
	if (png_ptr == NULL)
	{
		return (-1);
	}
	
	/* Allocate/initialize the memory for image information.  REQUIRED. */
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		return (-1);
	}
	
	/* Set error handling if you are using the setjmp/longjmp method (this is
	 * the normal method of doing things with libpng).  REQUIRED unless you
	 * set up your own error handlers in the png_create_read_struct() earlier.
	 */
	
	if (setjmp(png_jmpbuf(png_ptr)))
	{
	  /* Free all of the memory associated with the png_ptr and info_ptr */
	  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	  /* If we get here, we had a problem reading the file */
	  return (-1);
	}
	  
	if ((fpin = fopen(filename, "rb")) == NULL)
	{
	  	fprintf(stderr, "Could not find input file %s\n", filename);
	  	return (-1);
	}

	png_init_io(png_ptr, fpin);


	/* If we have already read some of the signature */	
	png_set_sig_bytes(png_ptr, sig_read);
	
	
	/* OK, you're doing it the hard way, with the lower-level functions */
	
	/* The call to png_read_info() gives us all of the information from the
	 * PNG file before the first IDAT (image data chunk).  REQUIRED
	 */
	png_read_info(png_ptr, info_ptr);
	
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);

	printf("PNG resolution = %dx%d , bit_depth = %d , color_type= %d , interlace_type = %d \n" , width, height, bit_depth, color_type, interlace_type );
	
	/* Tell libpng to strip 16 bit/color files down to 8 bits/color.
	 * Use accurate scaling if it's available, otherwise just chop off the
	 * low byte.
	 */
	if (bit_depth == 16)
	{
		//png_set_scale_16(png_ptr);
		png_set_strip_16(png_ptr); 
	}

	/* Strip alpha bytes from the input data without combining with the
	 * background (not recommended).
	 */
	//if (color_type & PNG_COLOR_MASK_ALPHA)
	//   png_set_strip_alpha(png_ptr);

	/* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
	 * byte into separate bytes (useful for paletted and grayscale images).
	 */
	if (bit_depth < 8)
	   png_set_packing(png_ptr);

	/* Change the order of packed pixels to least significant bit first
	 * (not useful if you are using png_set_packing). */
	if (bit_depth < 8)
	   png_set_packswap(png_ptr);

	/* Expand paletted colors into true RGB triplets */
	if (color_type == PNG_COLOR_TYPE_PALETTE)
	   png_set_palette_to_rgb(png_ptr);
			
	/* Expand grayscale images to the full 8 bits from 1, 2, or 4 bits/pixel */
	if ((color_type == PNG_COLOR_TYPE_GRAY) && (bit_depth < 8))
	   png_set_expand_gray_1_2_4_to_8(png_ptr);
	   

	//png_set_gray_to_rgb(png_ptr);

	/* Expand paletted or RGB images with transparency to full alpha channels
	 * so the data will be available as RGBA quartets.
	 */
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
	{
	   png_set_tRNS_to_alpha(png_ptr);
	   printf("png_set_tRNS_to_alpha\n");
	}
#if 0
	png_color_16 my_background = {0 , 0, 0 ,0, 0};

	//if (png_get_bKGD(png_ptr, info_ptr, &image_background))		   
	//  png_set_background(png_ptr, image_background,  PNG_BACKGROUND_GAMMA_FILE, 1, 1.0);		   
	//else			  
		png_set_background(png_ptr, &my_background, PNG_BACKGROUND_GAMMA_SCREEN, 0, 1.0);

#endif

	png_textp text_ptr;
	int num_text;

	if (png_get_text(png_ptr, info_ptr, &text_ptr, &num_text) > 0)
	{
		int i;
		printf("Handling %d tEXt/zTXt/iTXt chunks\n", num_text);

		if (verbose != 0)
		{
			for (i=0; i<num_text; i++)
			{
			   	printf("   compression[%d] = %d\n", i, text_ptr[i].compression);
				printf("   key[%d]         = %s\n", i, text_ptr[i].key);
				printf("   text[%d]        = %s\n", i, text_ptr[i].text);
				printf("   text_length[%d] = %lld\n", i, text_ptr[i].text_length);
			}
		}

		for (i=0; i<num_text; i++)
		{
			if (strcmp(iTXt_key, text_ptr[i].key) == 0)
			{
				if(title)  *title = strdup(text_ptr[i].text);
			}
		}

	}


#if 0
	/* Flip the RGB pixels to BGR (or RGBA to BGRA) */  
	if (color_type & PNG_COLOR_MASK_COLOR)   png_set_bgr(png_ptr);
#endif

	/* Invert monochrome files to have 0 as white and 1 as black */		   
	if (bit_depth == 1 && color_type == PNG_COLOR_TYPE_GRAY)		   	   
		png_set_invert_mono(png_ptr);

#if 0
	/* Swap the RGBA or GA data to ARGB or AG (or BGRA to ABGR) */	
	png_set_swap_alpha(png_ptr);


	/* Swap bytes of 16 bit files to least significant byte first */
	if (bit_depth == 16)
		  png_set_swap(png_ptr);

	png_set_invert_alpha(png_ptr);
#endif


	/* Add filler (or alpha) byte (before/after each RGB triplet) */
	if (color_type == PNG_COLOR_TYPE_RGB)
			png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
		
#if 1	// RGB to ARGB
	//if(input_setting.AddAlpha)
	{
		if (color_type==PNG_COLOR_TYPE_RGB)			
			png_set_add_alpha(png_ptr, 0xff, PNG_FILLER_AFTER);
	}
#endif

	number_passes = png_set_interlace_handling(png_ptr);

	printf("PNG number_passes = %d \n" , number_passes);

	/* Optional call to gamma correct and add the background to the palette
	* and update info structure.  REQUIRED if you are expecting libpng to
	* update the palette for you (ie you selected such a transform above).
	*/
	png_read_update_info(png_ptr, info_ptr);

	/* Allocate the memory to hold the image using the fields of info_ptr. */

	// The easiest way to read the image:
	//png_bytep row_pointers[height];

	// Clear the pointer array 
	//for (row = 0; row < height; row++)
	//	row_pointers[row] = NULL;

	//for (row = 0; row < height; row++)
	//	row_pointers[row] = png_malloc(png_ptr, png_get_rowbytes(png_ptr, info_ptr));

	// Now it's time to read the image.	One of these methods is REQUIRED 

	png_byte **row_pointers =  (png_bytepp)png_malloc(png_ptr, height * sizeof(png_bytep));

	row_bytes = png_get_rowbytes(png_ptr, info_ptr);

	pImage = (uint8_t *)malloc(height * row_bytes);
	
	dec_color_type = png_get_color_type(png_ptr, info_ptr);

	printf("PNG_COLOR_TYPE GRAY = %d, PALETTE = %d, RGB = %d, RGB_ALPHA = %d\n", PNG_COLOR_TYPE_GRAY, PNG_COLOR_TYPE_PALETTE, PNG_COLOR_TYPE_RGB, PNG_COLOR_TYPE_RGB_ALPHA);
	
	printf("PNG Decoded Color Type : %d, row_bytes = %lld\n", dec_color_type, row_bytes);

	for (y = 0; y < height; ++y)
		row_pointers[y] = pImage + (y * row_bytes);
	
	png_read_image(png_ptr, row_pointers);

	png_free(png_ptr, row_pointers);

	*out = pImage;
	*w = width;
	*h = height;
	
	// Read rest of file, and get additional chunks in info_ptr - REQUIRED 
	png_read_end(png_ptr, info_ptr);
	
	/* At this point you have read the entire image */
	
	/* Clean up after the read, and free any memory allocated - REQUIRED */
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	
	return 0;
}
