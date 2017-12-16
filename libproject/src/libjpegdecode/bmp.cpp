
#include <string.h>
#include "bmp.h"
#include "logger.h"
#define BMP_PALETTE_SIZE	( 256 * 4 )
/* Holds the last error code */
static BMP_STATUS bmp_last_error_code = BMP_OK;
int logleve = ERROR;

typedef struct 
{ 
	unsigned char Blue; 
	unsigned char Green;
	unsigned char Red;
	unsigned char Reserved;
} __attribute__((packed)) RgbQuad;


#if 0
/* Error description strings */
static const char *[20] bmp_error_string = {
	"",	
	"General error",	
	"Could not allocate enough memory to complete the operation",	
	"File input/output error",	
	"File not found",	
	"File is not a supported BMP variant (must be uncompressed 8, 24 or 32 BPP)",	
	"File is not a valid BMP image",	
	"An argument is invalid or out of range",	
	"The requested action is not compatible with the BMP's type"
};

#endif



BMP::BMP(unsigned int width, unsigned int height, unsigned short depth ,char* file) : filename(file) 
{
	int				bytes_per_pixel = depth >> 3;
	unsigned int	bytes_per_row;

	if ( height <= 0 || width <= 0 ) {		
		bmp_last_error_code = BMP_INVALID_ARGUMENT;		
		//return NULL;	
	}

	if ( depth != 8 && depth != 24 && depth != 32 )	{		
		bmp_last_error_code = BMP_FILE_NOT_SUPPORTED;		
		//return NULL;	
	}
	
	/* Set header' default values */
	bmp.Header.Magic = 				0x4D42;
	bmp.Header.Reserved1 = 			0;
	bmp.Header.Reserved2 = 			0;
	bmp.Header.HeaderSize = 		40;
	bmp.Header.Planes = 			1;
	bmp.Header.CompressionType = 	0;
	bmp.Header.HPixelsPerMeter = 	0;
	bmp.Header.VPixelsPerMeter =	0;
	bmp.Header.ColorsUsed = 		0;
	bmp.Header.ColorsRequired = 	0;
	
	/* Calculate the number of bytes used to store a single image row. This is always	rounded up to the next multiple of 4. */
	bytes_per_row = width * bytes_per_pixel;	
	bytes_per_row += ( bytes_per_row % 4 ? 4 - bytes_per_row % 4 : 0 );
	
	/* Set header's image specific values */	
	bmp.Header.Width  				= width;	
	bmp.Header.Height 				= height;	
	bmp.Header.BitsPerPixel 		= depth;	
	bmp.Header.ImageDataSize		= bytes_per_row * height;	
	bmp.Header.FileSize				= bmp.Header.ImageDataSize + 54 + ( depth == 8 ? BMP_PALETTE_SIZE : 0 );	
	bmp.Header.DataOffset			= 54 + ( depth == 8 ? BMP_PALETTE_SIZE : 0 );

	/* Allocate palette */	
	if ( bmp.Header.BitsPerPixel == 8 ) {
		bmp.Palette = (unsigned char*) calloc( BMP_PALETTE_SIZE, sizeof( unsigned char ) );		
		if ( bmp.Palette == NULL )	{			
			bmp_last_error_code = BMP_OUT_OF_MEMORY;			
			free( bmp.Palette );			
			//return NULL;		
		}
	} else {
		bmp.Palette = NULL;
	}

	/* Allocate pixels */	
	bmp.Data = (unsigned char*) calloc( bmp.Header.ImageDataSize, sizeof(unsigned char) );	
	if ( bmp.Data == NULL )	{		
		bmp_last_error_code = BMP_OUT_OF_MEMORY;		
		free( bmp.Palette );		
		free( bmp.Data);		
		//return NULL;	
	}

	bmp_last_error_code = BMP_OK;

	//return this;
	
}

BMP::~BMP() 
{
	if ( bmp.Palette != NULL)
		free( bmp.Palette );
	
	if ( bmp.Data != NULL ) 		
		free( bmp.Data );	
	
	bmp_last_error_code = BMP_OK;
}

int	BMP::ReadHeader	(FILE* f ) 
{
	if ( f == NULL )	
		return BMP_INVALID_ARGUMENT;
	
	/* The header's fields are read one by one, and converted from the format's	little endian to the system's native representation. */
	if (1 != fread( &bmp.Header, sizeof(BMP_Header),1, f )) {		
		LOG(ERROR,"Read the bmp file head error!");				
		return BMP_IO_ERROR;	
	}

	return BMP_OK;
}

int	BMP::WriteHeader (FILE* f )
{
	if ( f == NULL )	
		return BMP_INVALID_ARGUMENT;
	if(1 != fwrite( &bmp.Header, sizeof(BMP_Header),1, f )) {
		LOG(ERROR,"Write the bmp file head error!");
		return BMP_IO_ERROR;
	}

	return BMP_OK;
}



int BMP::BMP_ReadFile() 
{
	FILE*	f;
	/* Open file */	
	f = fopen( filename.data(), "rb" );	
	if ( f == NULL ) {		
		bmp_last_error_code = BMP_FILE_NOT_FOUND;				
		return BMP_FILE_NOT_FOUND;	
	}

	/* Read header */	
	if ( ReadHeader( f ) != BMP_OK || bmp.Header.Magic != 0x4D42 ) {		
		bmp_last_error_code = BMP_FILE_INVALID;	
		LOG(ERROR,"Read the bmp file header failed!");
		fclose( f );				
		return BMP_FILE_INVALID;	
	}
	
	/* Verify that the bitmap variant is supported */	
	if ( ( bmp.Header.BitsPerPixel != 32 && bmp.Header.BitsPerPixel != 24 && bmp.Header.BitsPerPixel != 8 )		
		|| bmp.Header.CompressionType != 0 || bmp.Header.HeaderSize != 40 ) {		
		bmp_last_error_code = BMP_FILE_NOT_SUPPORTED;		
		fclose( f );				
		return BMP_FILE_NOT_SUPPORTED;	
	}
	
	/* read palette */	
	if ( bmp.Header.BitsPerPixel == 8 ) {
		if ( fread( bmp.Palette, sizeof( unsigned char ), BMP_PALETTE_SIZE, f ) != BMP_PALETTE_SIZE ) {			
			bmp_last_error_code = BMP_FILE_INVALID;			
			fclose( f );									
			return BMP_FILE_INVALID;		
		}
	} 
	
	/* Read image data */	
	if(bmp.Header.ImageDataSize == 0) {
		bmp.Header.ImageDataSize = bmp.Header.Width * bmp.Header.Height * (bmp.Header.BitsPerPixel / 8);
	}
	if ( fread( bmp.Data, sizeof( unsigned char ), bmp.Header.ImageDataSize, f ) != bmp.Header.ImageDataSize )	{		
		bmp_last_error_code = BMP_FILE_INVALID;		
		fclose( f );
		LOG(ERROR,"Read the bmp file image data failed!");
		return BMP_FILE_INVALID;	
	}
	
	fclose( f );	
	bmp_last_error_code = BMP_OK;

	return BMP_OK;

	
}

int	BMP::BMP_WriteFile ()
{
	FILE*	f;
	
#if 0
	if ( filename == NULL )	{		
		bmp_last_error_code = BMP_INVALID_ARGUMENT;		
		return BMP_INVALID_ARGUMENT;	
	}
#endif
	/* Open file */	
	f = fopen(filename.data(), "wb" );	
	if ( f == NULL ) {		
		bmp_last_error_code = BMP_FILE_NOT_FOUND;				
		return BMP_FILE_NOT_FOUND;	
	}
	
	/* Write header */	
	if ( WriteHeader( f ) != BMP_OK ) {		
		bmp_last_error_code = BMP_FILE_INVALID;		
		fclose( f );				
		return BMP_FILE_INVALID;	
	}
	
	/* Write image data */	
	if ( fwrite( bmp.Data, sizeof( unsigned char ), bmp.Header.ImageDataSize, f ) != bmp.Header.ImageDataSize )	{		
		bmp_last_error_code = BMP_FILE_INVALID;		
		fclose( f );			
		return BMP_FILE_INVALID;	
	}

	fclose( f );	
	bmp_last_error_code = BMP_OK;
	
	return BMP_OK;
}

int BMP::BMP_GetPixelRGB(unsigned int x, unsigned int y, unsigned char* r, unsigned char* g, unsigned char* b )
{
	unsigned int	bytes_per_row;
	unsigned char*	pixel;
	unsigned char 	bytes_per_pixel;

	if (x < 0 || x >= bmp.Header.Width || y < 0 || y >= bmp.Header.Height ) {		
		bmp_last_error_code = BMP_INVALID_ARGUMENT;	
		return BMP_INVALID_ARGUMENT;
	} else {
		bmp_last_error_code = BMP_OK;

		bytes_per_pixel = bmp.Header.BitsPerPixel >> 3;
		
		/* Row's size is rounded up to the next multiple of 4 bytes */		
		bytes_per_row = bmp.Header.ImageDataSize / bmp.Header.Height;
		
		/* Calculate the location of the relevant pixel (rows are flipped) */		
		pixel = bmp.Data + ( ( bmp.Header.Height - y - 1 ) * bytes_per_row + x * bytes_per_pixel);

		 /* In indexed color mode the pixel's value is an index within the palette */		
		 if ( bmp.Header.BitsPerPixel == 8 )	
		 	pixel = bmp.Palette + *pixel * 4;	

		 /* Note: colors are stored in BGR order */		
		 if ( r )	*r = *( pixel + 2 );		
		 if ( g )	*g = *( pixel + 1 );		
		 if ( b )	*b = *( pixel + 0 );
		
	}

	return BMP_OK;
}

int BMP::BMP_SetPixelRGB( unsigned int x, unsigned int y, unsigned char r, unsigned char g, unsigned char b )
{
	unsigned int	bytes_per_row;
	unsigned char*	pixel;
	unsigned char 	bytes_per_pixel;

	if (x < 0 || x >= bmp.Header.Width || y < 0 || y >= bmp.Header.Height ) {		
		bmp_last_error_code = BMP_INVALID_ARGUMENT;	
		return BMP_INVALID_ARGUMENT;
	} else if ( bmp.Header.BitsPerPixel != 24 && bmp.Header.BitsPerPixel != 32 ) {		
		bmp_last_error_code = BMP_TYPE_MISMATCH;	
	} else {		
		bmp_last_error_code = BMP_OK;	
		
		bytes_per_pixel = bmp.Header.BitsPerPixel >> 3;		

		/* Row's size is rounded up to the next multiple of 4 bytes */		
		bytes_per_row = bmp.Header.ImageDataSize / bmp.Header.Height;		

		/* Calculate the location of the relevant pixel (rows are flipped) */		
		pixel = bmp.Data + ( ( bmp.Header.Height - y - 1 ) * bytes_per_row + x * bytes_per_pixel );		

		/* Note: colors are stored in BGR order */		
		*( pixel + 2 ) = r;		
		*( pixel + 1 ) = g;		
		*( pixel + 0 ) = b;	
	}

	return BMP_OK;
}

int BMP::BMP_GetPixelIndex(unsigned int x, unsigned int y, unsigned char* val )
{
	return BMP_OK;
}

int BMP::BMP_SetPixelIndex(unsigned int x, unsigned int y, unsigned char val )
{
	return BMP_OK;
}

int BMP::BMP_GetPaletteColor(unsigned char index, unsigned char* r, unsigned char* g, unsigned char* b )
{
	return BMP_OK;
}

int BMP::BMP_SetPaletteColor( unsigned char index, unsigned char r, unsigned char g, unsigned char b )
{
	return BMP_OK;
}

BMP_STATUS BMP::BMP_GetError()
{	
	return bmp_last_error_code;
}

const char* BMP::BMP_GetErrorDescription()
{	
#if 0
	if ( bmp_last_error_code > 0 && bmp_last_error_code < BMP_ERROR_NUM )	
		return bmp_error_string[ bmp_last_error_code ];	
	else	
#endif
		return NULL;	
}

void BMP::BMP_SaveAsRgb32Buffer(unsigned int xres,unsigned int yres,unsigned char *dst_buffer,unsigned char scn_pixe_bytes)
{
	unsigned char *src = bmp.Data;
	unsigned int location = 0;
	int len = bmp.Header.BitsPerPixel / 8;
	unsigned int line_x = 0, line_y = 0;
	unsigned int tmp;
	RgbQuad rgb;
	while(1)
	{
		tmp = 0;
		if(bmp.Header.BitsPerPixel == 24) {
			rgb.Reserved = 0xFF;
			memcpy((char *)&rgb,src,len);
			src += len;
		} else {
			memcpy((char *)&rgb,src,len);
			src += len;
		}
		//计算该像素在映射内存起始地址的偏移量
		location = line_x * scn_pixe_bytes + (bmp.Header.Height - line_y - 1) * xres * scn_pixe_bytes;
		tmp |= rgb.Reserved << 24 | rgb.Red << 16 | rgb.Green << 8 | rgb.Blue;
		*((unsigned long *)(dst_buffer + location)) = tmp;    
		line_x++;    
		if (line_x == bmp.Header.Width )
		{
			line_x = 0;
			line_y++;
			if(line_y == bmp.Header.Height)    
				break;    
		}    
	}
}

void BMP::BMP_PrintHeader()
{
	LOG(ALERT,"bmp.Header.Magic = 0x%x",bmp.Header.Magic);
	LOG(ALERT,"bmp.Header.FileSize = %d",bmp.Header.FileSize);
	LOG(ALERT,"bmp.Header.Reserved1 = %d",bmp.Header.Reserved1);
	LOG(ALERT,"bmp.Header.Reserved2 = %d",bmp.Header.Reserved2);
	LOG(ALERT,"bmp.Header.DataOffset = %d",bmp.Header.DataOffset);
	LOG(ALERT,"bmp.Header.HeaderSize = %d",bmp.Header.HeaderSize);
	LOG(ALERT,"bmp.Header.Width = %d",bmp.Header.Width);
	LOG(ALERT,"bmp.Header.Height = %d",bmp.Header.Height);
	LOG(ALERT,"bmp.Header.Planes = %d",bmp.Header.Planes);
	LOG(ALERT,"bmp.Header.BitsPerPixel = %d",bmp.Header.BitsPerPixel);
	LOG(ALERT,"bmp.Header.CompressionType = %d",bmp.Header.CompressionType);
	LOG(ALERT,"bmp.Header.ImageDataSize = %d",bmp.Header.ImageDataSize);
	LOG(ALERT,"bmp.Header.HPixelsPerMeter = %d",bmp.Header.HPixelsPerMeter);
	LOG(ALERT,"bmp.Header.VPixelsPerMeter = %d",bmp.Header.VPixelsPerMeter);
	LOG(ALERT,"bmp.Header.ColorsUsed = %d",bmp.Header.ColorsUsed);
	LOG(ALERT,"bmp.Header.ColorsRequired = %d",bmp.Header.ColorsRequired);
}









