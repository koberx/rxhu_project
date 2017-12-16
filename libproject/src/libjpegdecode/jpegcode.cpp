
#include "jpegdecode.h"
#include <string.h>

int main(int argc,char *argv[])
{	
	JPEG_DECODER* jpegobj = new JPEG_DECODER(argv[1]);
	jpegobj->JPEG_init_cos_cache();
	jpegobj->JPEG_save_bmp();
	delete(jpegobj);
	return 0;
}

