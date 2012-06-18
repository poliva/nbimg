/* 
 * nbimg
 *
 * Copyright (C) 2007-2008 Pau Oliva Fora - <pof@eslack.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * opinion) any later version. See <http://www.gnu.org/licenses/gpl.html>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

unsigned long magicHeader[]={'H','T','C','I','M','A','G','E'};
const char magicHeader2[]={'R','0','0','0','F','F','\n'};
const char initialsignature[]={'X' ,'D' ,'A' ,'-' ,'D' ,'e' ,'v' ,'e' ,'l' ,'o' ,'p' ,'e' ,'r' ,'s' ,'!' ,' ' };

struct HTCIMAGEHEADER {
	char device[32];
	unsigned long sectiontypes[32];
	unsigned long sectionoffsets[32];
	unsigned long sectionlengths[32];
	char CID[32];
	char version[16];
	char language[16];
};
struct HTCIMAGEHEADER HTCIMAGEHEADER;

int bufferedReadWrite(FILE *input, FILE *output, unsigned long length)
{
	unsigned char data[2048];
	unsigned long nread;

	while (length > 0) {
		nread = length;
		if (nread > sizeof(data))
			nread = sizeof(data);
		nread = fread(data, 1, nread, input);
		if (!nread)
			return 0;
		if (fwrite(data, 1, nread, output) != nread)
			return 0;
		length -= nread;
	}
	return 1;
}


static unsigned char bmphead1[18] = {
	0x42, 0x4d, 		/* signature */
	0x36, 0x84, 0x03, 0x00, /* size of BMP file in bytes (unreliable) */
	0x00, 0x00, 0x00, 0x00, /* reserved, must be zero */
	0x36, 0x00, 0x00, 0x00, /* offset to start of image data in bytes */
	0x28, 0x00, 0x00, 0x00, /* size of BITMAP INFO HEADER structure, must be 0x28 */
};

unsigned char bmpheadW[4];
unsigned char bmpheadH[4];
//	0xf0, 0x00, 0x00, 0x00, /* image width in pixels */
//	0x40, 0x01, 0x00, 0x00, /* image height in pixels */

static unsigned char bmphead2[8] = {
	0x01, 0x00,		/* number of planes in the image, must be 1 */
	0x18, 0x00, 		/* number of bits per pixel (1, 4, 8 or 24) */
	0x00, 0x00, 0x00, 0x00, /* compression type (0=none, 1=RLE-8, 2=RLE-4) */
};

unsigned char bmpheadS[4];
//	0x00, 0x84, 0x03, 0x00, /* size of image data in bytes (including padding) 0x38400 for 320x240*/ 

static unsigned char bmphead3[16] = {
	0x00, 0x00, 0x00, 0x00, /* horizontal resolution in pixels per meter (unreliable) */
	0x00, 0x00, 0x00, 0x00, /* vertical resolution in pixels per meter (unreliable) */
	0x00, 0x00, 0x00, 0x00, /* number of colors in image, or zero */
	0x00, 0x00, 0x00, 0x00  /* number of important colors, or zero */
};

static unsigned char smartphonesig[30] = {
	0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x73, 0x6d,
	0x61, 0x72, 0x74, 0x70, 0x68, 0x6f, 0x6e, 0x65, 0x20, 0x73,
	0x69, 0x67, 0x6e, 0x61, 0x74, 0x75, 0x72, 0x65, 0x2e, 0x00
};

static unsigned char htcsig[32] = {
	0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x48, 0x54,
	0x43, 0x20, 0x73, 0x70, 0x6c, 0x61, 0x73, 0x68, 0x20, 0x73,
	0x69, 0x67, 0x6e, 0x61, 0x74, 0x75, 0x72, 0x65, 0x2e, 0x00,
	0x00, 0x00
};

int convertBMP2NB(FILE *input, char *filename, unsigned long dataLen, int addhtcsig, int smartphone, int padsize, int pattern)
{
	FILE *output;
	int y,x,i;
	char filename2[1024];
	unsigned int biWidth;
	unsigned int biHeight;
	unsigned short encoded;
	unsigned char *pixdata;
	unsigned char data[dataLen];

	sprintf(filename2, "%s.nb", filename);
	printf("[] Encoding: %s\n", filename2);

	if (fread(data, 1, dataLen, input) != dataLen) {
		fprintf(stderr, "[!!] Could not read full image\n");
		return 1;
	}

	output=fopen(filename2,"wb");
	if (output == NULL) {
		fprintf(stderr, "[!!] Could not open '%s'\n", filename);
		return 1;
	}

	memcpy(&biWidth, data+18, sizeof(biWidth));
	memcpy(&biHeight, data+22, sizeof(biHeight));

	printf ("[] Image dimensions: %dx%d\n", biWidth, biHeight);

	if (smartphone) {
		printf ("[] Adding smartphone signature\n");
		fwrite(smartphonesig, 1, sizeof(smartphonesig), output);
	}
	
	for (y=0; y < biHeight; y++) {
		pixdata = data + 54 + ((biHeight-(y+1))*biWidth*3);
		for (x=0; x < biWidth; x++) {

			encoded =   pixdata[0]  >> 3 ; //B
			encoded |= (int)(pixdata[1]>>2) << 5 ; //G
			encoded |= (int)(pixdata[2]>>3) << 11 ; //R
			
			pixdata+= 3;
			fwrite(&encoded, 1, 2, output);
		}
	}

	printf ("[] Adding %d bytes padding using pattern [0x%x]\n", padsize, pattern);
	for (i = 0; i < padsize; i++) {
		fwrite(&pattern, 1, 1, output);
	}

	if (smartphone == 0 && addhtcsig == 1) {
		printf ("[] Adding HTC splash screen signature\n");
		fwrite(htcsig, 1, sizeof(htcsig), output);
	}

	fclose(output);
	return 0;
}

/* convertBMP - converts a NB splash screen to a bitmap */
int convertNB2BMP(FILE *input, char *filename, int biWidth, int biHeight, unsigned long dataLen)
{
	FILE *output;
	int y,x;
	char filename2[1024];
	unsigned char colors[3];
	unsigned short encoded;
	unsigned long biSize;
	unsigned char data[dataLen];

	sprintf(filename2, "%s.bmp", filename);
	printf("[] Encoding: %s\n", filename2);

	if (fread(data, 1, dataLen, input) != dataLen) {
		fprintf(stderr, "[!!] Could not read full image\n");
		return 1;
	}

	output=fopen(filename2,"wb");
	if (output == NULL) {
		fprintf(stderr, "[!!] Could not open '%s'\n", filename);
		return 1;
	}

	if (fwrite(bmphead1, 1, sizeof(bmphead1), output) != sizeof(bmphead1)) {
		fprintf(stderr, "[!!] Could not write bitmap header 1\n");
		fclose(output);
		return 1;
	}

	fwrite(&biWidth, 1, sizeof(biWidth), output);
	fwrite(&biHeight, 1, sizeof(biHeight), output);

	if (fwrite(bmphead2, 1, sizeof(bmphead2), output) != sizeof(bmphead2)) {
		fprintf(stderr, "[!!] Could not write bitmap header 2\n");
		fclose(output);
		return 1;
	}

	biSize = biWidth*biHeight*3;
	fwrite(&biSize, 1, sizeof(biSize), output);

	if (fwrite(bmphead3, 1, sizeof(bmphead3), output) != sizeof(bmphead3)) {
		fprintf(stderr, "[!!] Could not write bitmap header 3\n");
		fclose(output);
		return 1;
	}

	for (y=0; y < biHeight; y++) {
		for (x=0; x < biWidth; x++) {
			encoded = ((unsigned short *)data)[((biHeight-(y+1))*biWidth)+x];
			colors[0] = (encoded << 3) & 0xF8; // 11111000b  take only 5 bytes
			colors[1] = (encoded >> 3) & 0xFC; // 11111100b  take only 6 bytes
			colors[2] = (encoded >> 8) & 0xF8; // 11111000b  take only 5 bytes
			fwrite(colors, 1, 3, output);
		}
	}

	fclose(output);
	return 0;
}

int convertNB2NBH(char *filename, int SignMaxChunkSize, char *modelid, char *type)
{

	FILE *nbh;
	FILE *dbh;
	FILE *nb;

	char filename2[1024];
	char nbfile[1024];

	unsigned long offset;
	unsigned long totalsize;
	unsigned long blockLen;
	unsigned long sectionlen;
	unsigned long signLen;
	unsigned long totalwrite;
	unsigned long lps;

	unsigned char signature[5000];
	unsigned char flag;

	struct HTCIMAGEHEADER header;

	printf ("[] Generating NBH file\n");

	strncpy(header.device, modelid, 32);
	sprintf(nbfile, "%s.nb", filename);

	strcpy(header.CID, "11111111");
	strcpy(header.version, "nbimg");
	strcpy(header.language, "pof");

	switch (SignMaxChunkSize) {
		case 64:
			blockLen = 0x10000;
			signLen = 128;
			break;
		case 1024:
			blockLen = 0x100000;
			signLen = 128;
			break;
		default:
			fprintf(stderr, "[!!] SignMaxChunkSize must be 64 or 1024\n");
			return 1;
			break;
	}

	memset(header.sectiontypes, 0, sizeof(header.sectiontypes));
	memset(header.sectionoffsets, 0, sizeof(header.sectionoffsets));
	memset(header.sectionlengths, 0, sizeof(header.sectionlengths));
	memset(signature, 0, sizeof(signature));
	sscanf(type, "0x%lx", &header.sectiontypes[0]);

	/* calculate offset for first section type */
	offset = sizeof(magicHeader) + sizeof(HTCIMAGEHEADER);

	nb = fopen(nbfile, "rb");
	if (nb == NULL) {
		fprintf(stderr, "[!!] Could not open '%s'\n", nbfile);
		fclose(dbh);
		exit(1);
	}

	fseek(nb, 0, SEEK_END);
	sectionlen = ftell(nb);
	fclose(nb);

	header.sectionoffsets[0]=offset;
	header.sectionlengths[0]=sectionlen;

	/* we have the header, now write the DBH file */
	dbh = fopen("tempfile.dbh","wb");
	if (dbh == NULL) {
		fprintf(stderr, "[!!] Could not open 'tempfile.dbh'\n");
		exit(1);
	}

	/* write the DBH header */
	fwrite(magicHeader, 1, sizeof(magicHeader), dbh);
	fwrite(&header, 1, sizeof(HTCIMAGEHEADER), dbh);

	nb = fopen(nbfile, "rb");
	if (nb == NULL) {
		fprintf(stderr, "[!!] Could not open '%s'\n", nbfile);
		fclose(dbh);
		exit(1);
	}

	fseek(nb, 0, SEEK_END);
	sectionlen = ftell(nb);
	fseek(nb, 0, SEEK_SET);

	bufferedReadWrite(nb, dbh, sectionlen);	

	fclose(nb);
	fclose(dbh);

	/* we have the DBH, now write the NBH */

	sprintf(filename2, "%s.nbh", filename);
	strcpy(filename, filename2);

	nbh = fopen(filename, "wb");
	if (nbh == NULL) {
		fprintf(stderr, "[!!] Could not open '%s'\n", filename);
		exit(1);
	}

	fwrite(magicHeader2, 1, sizeof(magicHeader2), nbh);
	fwrite(initialsignature, 1, sizeof(initialsignature), nbh);

	dbh = fopen("tempfile.dbh", "rb");
	if (dbh == NULL) {
		fprintf(stderr, "[!!] Could not open 'tempfile.dbh'\n");
		fclose(nbh);
		exit(1);
	}

	fseek(dbh, 0, SEEK_END);
	totalsize = ftell(dbh);
	fseek(dbh, 0, SEEK_SET);
	totalwrite = 0;
	flag = 1;

	do {
		if ((totalsize - totalwrite) < blockLen)
			lps = totalsize - totalwrite;
		else 
			lps = blockLen;
		fwrite(&lps, 4, 1, nbh);
		fwrite(&signLen, 4, 1, nbh);
		fwrite(&flag, 1, 1, nbh);
		bufferedReadWrite(dbh, nbh, lps);
		fwrite(signature, 1, signLen, nbh);
		totalwrite += lps;
	} while (totalwrite < totalsize);
	
	fclose(dbh);
	unlink("tempfile.dbh");

	/* write the last block */
	blockLen = 0;
	flag = 2;
	fwrite(&blockLen, 4, 1, nbh);
	fwrite(&signLen, 4, 1, nbh);
	fwrite(&flag, 1, 1, nbh);
	fwrite(signature, 1, signLen, nbh);

	fclose(nbh);
	return 0;
}

void help_show_message()
{
        fprintf(stderr, "Usage: nbimg -F file.[nb|bmp]\n\n");
        fprintf(stderr, "Mandatory arguments:\n");
        fprintf(stderr, "   -F <filename>    Filename to convert.\n");
        fprintf(stderr, "                    If the extension is BMP it will be converted to NB.\n");
        fprintf(stderr, "                    If the extension is NB it will be converted to BMP.\n\n");
        fprintf(stderr, "Optional arguments:\n");
        fprintf(stderr, "   -w <width>       Image width in pixels. If not specified will be autodetected.\n");
        fprintf(stderr, "   -h <height>      Image height in pixels. If not specified will be autodetected.\n");
        fprintf(stderr, "   -t <pattern>     Manually specify the padding pattern (usually 0 or 255).\n");
        fprintf(stderr, "   -p <size>        Manually specify the padding size.\n");
        fprintf(stderr, "   -n               Do not add HTC splash signature to NB file.\n");
        fprintf(stderr, "   -s               Output smartphone format.\n\n");
        fprintf(stderr, "NBH arguments:      (only when converting from BMP to NBH)\n");
	fprintf(stderr, "   -D <model_id>    Generate NBH with specified Model ID (mandatory)\n");
	fprintf(stderr, "   -S <chunksize>   NBH SignMaxChunkSize (64 or 1024)\n");
	fprintf(stderr, "   -T <type>        NBH header type, this is typically 0x600 or 0x601\n\n");
}

int main(int argc, char** argv)
{
	char filename[1024];

	FILE *input;
	char* extension;
	unsigned long dataLen;
	int biHeight=0, biWidth=0;
	int addhtcsig=1;
	int smartphone=0;
	int padsize=0;
	int pattern=0xff;
	int pattern_set=0;
	int padsize_set=0;
	int c;

	char type[32];
	int gen_nbh=0;
	int SignMaxChunkSize=64;
	char modelid[32];

	printf ("=== nbimg v1.1\n");
	printf ("=== Convert NB <--> BMP splash screens\n");
	printf ("=== (c)2008 Pau Oliva - pof @ xda-developers\n\n");

	if (argc < 2) {
		help_show_message();
		return 1;
	}

	strcpy(type,"0x600");
	memset(filename,0,sizeof(filename));
	memset(modelid,0,sizeof(modelid));

	while ((c = getopt(argc, argv, "F:w:h:nsp:t:D:S:T:")) != -1) {
		switch(c) {
		case 'F':
			sprintf(filename, optarg);
			break;
		case 'w':
			biWidth = atoi(optarg);
			break;
		case 'h':
			biHeight = atoi(optarg);
			break;
		case 'n':
			addhtcsig=0;
			break;
		case 's':
			smartphone=1;
			break;
		case 'p':
			padsize = atoi(optarg);
			padsize_set=1;
			break;
		case 't':
			pattern = atoi(optarg);
			pattern_set=1;
			break;
		case 'D':
			sprintf(modelid, optarg);
			gen_nbh=1;
			break;
		case 'S':
			SignMaxChunkSize = atoi(optarg);
			gen_nbh=1;
			break;
		case 'T':
			sprintf(type, optarg);
			gen_nbh=1;
			break;
		default:
			help_show_message();
			return 1;
			break;
		}
	}

	if (strlen(filename) == 0) {
		printf ("[!!] ERROR: Missing input file\n");
		help_show_message();
		return 1;
	}

	if (strlen(modelid) == 0 && gen_nbh == 1) {
		printf ("[!!] ERROR: Missing Model ID for NBH file (option -D)\n");
		help_show_message();
		return 1;
	}

	printf("[] File: %s\n", filename);

	input = fopen(filename, "rb");
	if (input == NULL) {
		fprintf(stderr, "[!!] Could not open '%s'\n", filename);
		return 1;
	}

	extension = strrchr(filename, '.');
	if (extension == NULL) {
		fprintf(stderr, "[!!] File extension must be BMP or NB\n");
		return 1;
	}

	fseek(input, 0, SEEK_END);
	dataLen = ftell(input);
	fseek(input, 0, SEEK_SET);

	if (!strcasecmp(extension, ".bmp")) {

		switch (dataLen) {
			case 230454:
				if (pattern_set == 0) pattern = 0x0;
				if (smartphone == 1) {
					if (padsize_set == 0) padsize = 108544;
				} else {
					if (padsize_set == 0) padsize = 108544 - sizeof(htcsig);
				}
				break;
			case 921654:
				if (pattern_set == 0) pattern = 0xff;
				if (smartphone == 1) {
					if (padsize_set == 0) padsize = 40960;
				} else {
					if (padsize_set == 0) padsize = 40960 - sizeof(htcsig);
				}
				break;
			default:
				fprintf (stderr,"[] No padding added. Check file size.\n");
				break;
		}

		convertBMP2NB(input, filename, dataLen, addhtcsig, smartphone, padsize, pattern);

		if (gen_nbh == 1) {
			convertNB2NBH(filename, SignMaxChunkSize, modelid, type);
		}

	}

	else if (!strcasecmp(extension, ".nb")) {

		switch (dataLen) {

			case 262144: 
			case 153600: 
				if (biWidth == 0) biWidth = 240;
				if (biHeight == 0) biHeight = 320;
				break;
			case 655360:
			case 614912:
				if (biWidth == 0) biWidth = 480;
				if (biHeight == 0) biHeight = 640;
				break;
			default:
				if (biWidth == 0 || biHeight == 0) {
					fprintf(stderr, "[!!] Could not determine Width and Height.\n");
					fprintf(stderr, "[!!] Please specify manually using both -w and -h\n");
					fclose(input);
					return 1;
				}
				break;
		}
		printf("[] Image dimensions: %dx%d\n", biWidth, biHeight);

		dataLen = biWidth * biHeight * 2;

		convertNB2BMP(input, filename, biWidth, biHeight, dataLen);
	}

	else {
		fprintf(stderr, "[!!] File extension (%s) must be BMP or NB\n", extension);
		return 1;
	}

	fclose(input);
	printf ("[] Done!\n");
	return 0;
}
