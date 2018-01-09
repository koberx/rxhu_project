#ifndef _JPEGDECODE_H_
#define _JPEGDECODE_H_
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <utility>
#include <map>
#include <math.h>
#include "logger.h"
#include "bmp.h"

class JPEG_DECODER {
	public:
		std::string 	jpegfile;
		FILE *f;
		JPEG_DECODER(char *file) : jpegfile(file) { f = fopen(jpegfile.data(), "r"); }
		~JPEG_DECODER() { fclose(f); }
		void 			JPEG_init_cos_cache();
		void 			JPEG_save_bmp();		
};

/**/
const int SOI_MARKER = 0xD8;
const int APP0_MARKER = 0xE0;
const int DQT_MARKER = 0xDB;
const int SOF_MARKER = 0xC0;
const int DHT_MARKER = 0xC4;
const int SOS_MARKER = 0xDA;
const int EOI_MARKER = 0xD9;
const int COM_MARKER = 0xFE;

struct {
    int height;
    int width;
} image;

struct {
    unsigned char id;
    unsigned char width;
    unsigned char height;
    unsigned char quant;
} subVector[4];

struct acCode {
    unsigned char len;
    unsigned char zeros;
    int value;
};

struct RGB {
    unsigned char R, G, B;
};

typedef double BLOCK[8][8];

const int DC = 0;
const int AC = 1;

class MCU {
public:
    BLOCK mcu[4][2][2];
    // ³ýåeÓÃ
    void show();
    void quantify();
    void zigzag();
    void idct();
    void decode();
    RGB **toRGB();
private:
    double cc(int i, int j);
    double c(int i);
    unsigned char chomp(double x);
    double trans(int id, int h, int w);
};

extern 	void readData(FILE *f);//
extern 	MCU readMCU(FILE *f);
extern	acCode readAC(FILE *f, unsigned char number);
extern 	int readDC(FILE *f, unsigned char number);
extern unsigned char matchHuff(FILE *f, unsigned char number, unsigned char ACorDC);
extern bool getBit(FILE *f);
extern void readSOS(FILE *f);//
extern void readDHT(FILE *f);//
extern void readSOF(FILE *f);//
extern void readDQT(FILE *f);//
extern void readCOM(FILE *f);//
extern void readAPP(FILE *f);//
extern unsigned int EnterNewSection(FILE *f, const char *s) ;
extern unsigned int readSectionLength(FILE *f);
extern void showSectionName(const char *s);


#endif