#ifndef _BMP_H_
#define _BMP_H_
#include <stdio.h>
#include <string>
/* Bitmap header */
typedef struct _BMP_Header {
	unsigned short		Magic;				/* Magic identifier: "BM" */
	unsigned int		FileSize;			/* Size of the BMP file in bytes */
	unsigned short		Reserved1;			/* Reserved */
	unsigned short		Reserved2;			/* Reserved */
	unsigned int		DataOffset;			/* Offset of image data relative to the file's start */
	unsigned int		HeaderSize;			/* Size of the header in bytes */
	unsigned int		Width;				/* Bitmap's width */
	unsigned int		Height;				/* Bitmap's height */
	unsigned short		Planes;				/* Number of color planes in the bitmap */
	unsigned short		BitsPerPixel;		/* Number of bits per pixel */
	unsigned int		CompressionType;	/* Compression type */
	unsigned int		ImageDataSize;		/* Size of uncompressed image's data */
	unsigned int		HPixelsPerMeter;	/* Horizontal resolution (pixels per meter) */
	unsigned int		VPixelsPerMeter;	/* Vertical resolution (pixels per meter) */
	unsigned int		ColorsUsed;			/* Number of color indexes in the color table that are actually used by the bitmap */
	unsigned int		ColorsRequired;		/* Number of color indexes that are required for displaying the bitmap */
} __attribute__((packed)) BMP_Header;

/* Error codes */
typedef enum {
	BMP_OK = 0,				/* No error */
	BMP_ERROR,				/* General error */
	BMP_OUT_OF_MEMORY,		/* Could not allocate enough memory to complete the operation */
	BMP_IO_ERROR,			/* General input/output error */
	BMP_FILE_NOT_FOUND,		/* File not found */
	BMP_FILE_NOT_SUPPORTED,	/* File is not a supported BMP variant */
	BMP_FILE_INVALID,		/* File is not a BMP image or is an invalid BMP */
	BMP_INVALID_ARGUMENT,	/* An argument is invalid or out of range */
	BMP_TYPE_MISMATCH,		/* The requested action is not compatible with the BMP's type */
	BMP_ERROR_NUM
} BMP_STATUS;

/* Private data structure */
struct _BMP {
	BMP_Header			Header;
	unsigned char*		Palette;
	unsigned char*		Data;
};




class BMP {
	public:
		std::string 	filename;
		
		/* Construction/destruction */
		BMP( unsigned int width, unsigned int height, unsigned short depth ,char* file);
		~BMP();
		
		/* I/O */
		int				BMP_ReadFile();
		int				BMP_WriteFile();
		/* Meta info */
		unsigned int 	BMP_GetWidth() { return bmp.Header.Width; }
		unsigned int	BMP_GetHeight() { return bmp.Header.Height; }
		unsigned short	BMP_GetDepth	() { return bmp.Header.BitsPerPixel; }
		unsigned char*	BMP_GetData() { return bmp.Data; }

		/* Pixel access */
		int				BMP_GetPixelRGB(unsigned int x, unsigned int y, unsigned char* r, unsigned char* g, unsigned char* b );
		int				BMP_SetPixelRGB( unsigned int x, unsigned int y, unsigned char r, unsigned char g, unsigned char b );
		int				BMP_GetPixelIndex(unsigned int x, unsigned int y, unsigned char* val );
		int				BMP_SetPixelIndex(unsigned int x, unsigned int y, unsigned char val );

		/* Palette handling */
		int				BMP_GetPaletteColor(unsigned char index, unsigned char* r, unsigned char* g, unsigned char* b );
		int				BMP_SetPaletteColor( unsigned char index, unsigned char r, unsigned char g, unsigned char b );
		
		/* Convert to buffer*/
		void			BMP_SaveAsRgb32Buffer(unsigned int xres,unsigned int yres,unsigned char *dst_buffer,unsigned char scn_pixe_bytes);
		
		/* Print the header info*/
		void			BMP_PrintHeader();
		/* Error handling */
		BMP_STATUS		BMP_GetError();
		const char*		BMP_GetErrorDescription();
	private:
		_BMP 			bmp;
		int				ReadHeader	(FILE* f );
		int				WriteHeader	(FILE* f );
};

/* Useful macro that may be used after each BMP operation to check for an error */
#define BMP_CHECK_ERROR( bmp_obj, output_file,return_value ) \
	if ( bmp_obj->BMP_GetError() != BMP_OK )													\
	{																				\
		fprintf( (output_file), "BMP error: %s\n", bmp_obj->BMP_GetErrorDescription() );	\
		return( return_value );														\
	}


#endif