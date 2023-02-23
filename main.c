// Bailey Jia-Tao Brown
// 2023

#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <Windows.h>
#include <compressapi.h>

#include <vcore.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

typedef struct VCIFileHead {
	UINT32 width;
	UINT32 height;
} VCIFileHead, *pVCIFileHead;

static void print_charline(char c, long len) {
	PCHAR str = vAllocZeroed(len + 1);

	__stosb(str, c, len + 1);
	puts(str);

	vFree(str);
}

static void makeVCIFile(void) {
	printf("File to load: ");

	// get user input for file
	PCHAR inputBuff = vAllocZeroed(BUFF_LARGE);
	gets_s(inputBuff, BUFF_LARGE);

	// attempt to load file
	// on false, re-prompt
	if (vFileExists(inputBuff) == FALSE) {
		printf("Error loading file %s.\n", inputBuff);
		return;
	}

	// open file
	printf("Loading file %s...\n", inputBuff);
	HANDLE imgFileHndl = vFileOpen(inputBuff);

	// copy file to memory
	vUI64 fileSize = vFileSize(imgFileHndl);
	PBYTE imgData = vAllocZeroed(fileSize);
	vFileRead(imgFileHndl, 0, fileSize, imgData);

	// close file and free input buffer
	vFileClose(imgFileHndl);

	// get stb to parse
	printf("Parsing image...\n");
	stbi_set_flip_vertically_on_load(TRUE);
	INT width, height, channels;
	PBYTE parsedImgDat =
		stbi_load_from_memory(imgData, fileSize,
			&width, &height, &channels, STBI_rgb_alpha);
	printf("Image parsed successfully.\n");
	printf("Image params:\n\tWidth: %d\n\tHeight: %d\n",
		width, height);

	// prompt output file name
	printf("Filename to Output: ");
	gets_s(inputBuff, BUFF_LARGE);

	// add file extension to the end
	strcat_s(inputBuff, BUFF_LARGE, ".vci");

	// create compressor
	COMPRESSOR_HANDLE compObj;
	CreateCompressor(COMPRESS_ALGORITHM_XPRESS_HUFF, NULL,
		&compObj);

	// compress parsed image
	PBYTE compressedImgDat =
		vAllocZeroed(width * height * 4);
	SIZE_T compressedImgSize;
	printf("Compressing Data...\n");
	Compress(compObj, parsedImgDat, width * height * 4,
		compressedImgDat, width * height * 4, &compressedImgSize);
	CloseCompressor(compObj);

	printf("Writing file...\n");

	// write to file and close
	HANDLE fileOut = vFileCreate(inputBuff);

	VCIFileHead fileHeader;
	fileHeader.width = width;
	fileHeader.height = height;

	// write file header
	vFileWrite(fileOut, ZERO, sizeof(VCIFileHead), &fileHeader);

	// write compressed data
	vFileWrite(fileOut, sizeof(VCIFileHead), compressedImgSize,
		compressedImgDat);
	vFileClose(fileOut);
	printf("File written sucessfully as %s\n",
		inputBuff);

	// free all data
	vFree(compressedImgDat);
	vFree(imgData);
	vFree(inputBuff);
}

static void loadVCIFileToBinary(void) {
	printf("File to load: ");

	// get user input for file
	PCHAR inputBuff = vAllocZeroed(BUFF_LARGE);
	gets_s(inputBuff, BUFF_LARGE);

	// attempt to load file
	// on false, re-prompt
	if (vFileExists(inputBuff) == FALSE) {
		printf("Error loading file %s.\n", inputBuff);
		return;
	}

	// open file
	printf("Loading file %s...\n", inputBuff);
	HANDLE compImgFileHndl = vFileOpen(inputBuff);

	// copy file to memory
	vUI64 fileSize = vFileSize(compImgFileHndl);
	PBYTE compData = vAllocZeroed(fileSize);
	vFileRead(compImgFileHndl, 0, fileSize, compData);

	// close file and free input buffer
	vFileClose(compImgFileHndl);

	printf("File loaded.\n");

	// get decompress size and make decompress buffer
	VCIFileHead headInfo;
	vMemCopy(&headInfo, compData, sizeof(VCIFileHead));
	PBYTE decompBuffer =
		vAllocZeroed(headInfo.width * headInfo.height * 4);

	// decompress
	printf("Decompressing...\n");
	SIZE_T decompSize;
	DECOMPRESSOR_HANDLE decompObj;
	CreateDecompressor(COMPRESS_ALGORITHM_XPRESS_HUFF, NULL,
		&decompObj);
	Decompress(decompObj, compData + sizeof(VCIFileHead), fileSize - sizeof(VCIFileHead), 
		decompBuffer, headInfo.width * headInfo.height * 4, &decompSize);
	printf("Decompression completed.\n");

	// output again
	// prompt output file name
	printf("Filename to Output: ");
	gets_s(inputBuff, BUFF_LARGE);

	// add file extension to the end
	strcat_s(inputBuff, BUFF_LARGE, ".bin");

	// write compressed data
	HANDLE binFileHndl =
		vFileCreate(inputBuff);
	vFileWrite(binFileHndl, 0, decompSize,
		decompBuffer);
	vFileClose(binFileHndl);
	printf("File written sucessfully as %s\n",
		inputBuff);

	vFree(compData);
	vFree(decompBuffer);
	vFree(inputBuff);
}

int main(int argc, char** argv) {
	SetConsoleTitleA("VCIMaker");

	vCoreInitialize();

	// intro print
	print_charline('=', 30);
	puts("VCIMaker\nBailey Jia Tao Brown\n2023");
	print_charline('=', 30);

	// main loop
	while (TRUE) {
		print_charline('=', 30);
		printf("Enter 'Q' to:\nConvert image file to VCI\n");
		printf("Enter 'E' to:\nConvert VCI to BIN\n");

		printf("Your input: ");
		char buff[BUFF_MEDIUM];
		gets_s(buff, BUFF_MEDIUM);

		// check bad input
		if (strlen(buff) > 1)
			buff[0] = ' ';

		// ignore case
		buff[0] = CharUpperA(buff[0]);

		switch (buff[0])
		{
		case 'Q':
			makeVCIFile();
			break;
		case 'E':
			loadVCIFileToBinary();
			break;
		default:
			printf("Undefined input. Please try again.\n");
		}
	}
	
}