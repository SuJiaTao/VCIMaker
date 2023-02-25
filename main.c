// Bailey Jia-Tao Brown
// 2023

#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <Windows.h>
#include <compressapi.h>
#include <ShlObj.h>

#include <vcore.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "vcore.h"
#include "vgfx.h"
#include "vuser.h"

typedef struct VCIFileHead {
	UINT32 width;
	UINT32 height;
} VCIFileHead, *pVCIFileHead;

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

typedef struct TButton {
	vPUPanel background;
	vPUPanel text;
} TButton, *PTButton;

PTButton CreatePTButton(vPUPanelStyle guiStyle, vGRect rect,
	PCHAR text, float tSize) {
	PTButton ptbutton = vAlloc(sizeof(TButton));

	ptbutton->text =
		vUCreatePanelText(guiStyle, rect, vUPanelTextFormat_CenteredComplete,
			text);
	ptbutton->background =
		vUCreatePanelButton(guiStyle, rect, NULL);
	ptbutton->text->textSize = tSize;

	return ptbutton;
}

int main(int argc, char** argv) {
	//FreeConsole(); // disable window
	HRESULT initRslt = CoInitialize(NULL);

	// init libraries
	vCoreInitialize();
	vGInitializeData gInitDat;
	gInitDat.targetFrameRate = 45;
	gInitDat.windowName = "VCI Maker";
	gInitDat.windowWidth  = 1000;
	gInitDat.windowHeight = 700;
	vGInitialize(&gInitDat);
	vUInitialize();

	// init GUI style
	vPUPanelStyle GUIStyle =
		vUCreatePanelStyle(
			vGCreateColorB(VGFX_COLOR_1b, 255),
			vGCreateColorB(0, 0, 0, 255),
			vGCreateColorB(VGFX_COLOR_3b, 255),
			 0.010f,
			 0.01f,
			-0.015f,
			NULL
		);

	// make title
	vPUPanel title =
		vUCreatePanelText(GUIStyle,
			vUCreateRectCenteredOffset(vCreatePosition(0, 0.7f), 2, 0.5f), 
			vUPanelTextFormat_Centered, "VCI MAKER");
	title->textSize = 0.15f;

	// file output directory
	char _outDirPath[MAX_PATH];
	vZeroMemory(_outDirPath, sizeof(_outDirPath));
	GetCurrentDirectoryA(MAX_PATH, _outDirPath);

	// generate dirpanel string
	char _dirPanelStr[MAX_PATH + BUFF_MEDIUM];
	sprintf_s(_dirPanelStr, sizeof(_dirPanelStr), "Output dir:\n%s", _outDirPath);

	// make directory pannel
	vPUPanel directory =
		vUCreatePanelText(GUIStyle,
			vUCreateRectCenteredOffset(vCreatePosition(0, 0.55f), 2, 0.5f),
			vUPanelTextFormat_Centered, _dirPanelStr);
	directory->textSize = 0.05f;

	PTButton changeDirButton =
		CreatePTButton(
			GUIStyle,
			vUCreateRectCenteredOffset(
				vCreatePosition(-0.75f, 0.45f),
				0.75f,
				0.25f
			),
			"Change Dir",
			0.07f
		);

	PTButton compileFilesButton =
		CreatePTButton(
			GUIStyle,
			vUCreateRectCenteredOffset(
				vCreatePosition(0.45f, 0.45f),
				1.00f,
				0.25f
			),
			"Compile Files",
			0.07f
		);

	// main loop
	while (vGExited() == FALSE) {

		// CHANGE DIR HANDLER
		if (vUIsMouseClickingPanel(changeDirButton->background)) {
			// setup file browser dialouge
			BROWSEINFOA browseInfo;
			vZeroMemory(&browseInfo, sizeof(BROWSEINFOA));
			CHAR unusedBuffer[MAX_PATH];
			browseInfo.pszDisplayName = unusedBuffer;
			browseInfo.ulFlags = BIF_NEWDIALOGSTYLE;
			browseInfo.hwndOwner =
				vGGetInternals()->window.window;
	
			// get item ID
			LPITEMIDLIST itemID = SHBrowseForFolderA(&browseInfo);

			// get new full path
			BOOL rslt = SHGetPathFromIDListA(itemID, _outDirPath);

			// free item ID
			CoTaskMemFree(itemID);

			// update dirpanel string
			vUPanelTextLock(directory);
			sprintf_s(_dirPanelStr, sizeof(_dirPanelStr), "Output dir:\n%s", _outDirPath);
			vUPanelTextUnlock(directory);
		}

		// FILE LOAD HANDLER
		if (vUIsMouseClickingPanel(compileFilesButton->background)) {
			// make file open params
			OPENFILENAMEA fopenOptions;
			ZeroMemory(&fopenOptions, sizeof(OPENFILENAMEA));
			fopenOptions.lStructSize = sizeof(fopenOptions);
			fopenOptions.hwndOwner = vGGetInternals()->window.window;
			fopenOptions.lpstrFile = vAllocZeroed(BUFF_MASSIVE);
			fopenOptions.nMaxFile  = BUFF_MASSIVE;
			fopenOptions.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR |
				OFN_EXPLORER | OFN_ALLOWMULTISELECT;
			fopenOptions.lpstrFilter = TEXT("All files(*.*)\0*.*\0\0");
			fopenOptions.lpstrFilter = NULL;
			fopenOptions.nFilterIndex = 1;
			BOOL option = GetOpenFileNameA(&fopenOptions);

			// parse files
			// files[0] is "root folder"
			// all the rest are filenames
			PCHAR files[BUFF_MEDIUM];
			INT fileCount = 0;
			INT strWalk = 0;
			while (TRUE) {
				files[fileCount] = fopenOptions.lpstrFile + strWalk;
				strWalk += strlen(files[fileCount]) + 1;
				fileCount++;
				if (fopenOptions.lpstrFile[strWalk] == 0) break;
			}

			// load each file
			for (int i = 0; i < fileCount; i++) {
				// if multi-file, skip first iteration
				if (fileCount > 1 && i == 0) continue;

				// generate load path
				PCHAR loadPath = vAllocZeroed(BUFF_MASSIVE);
				sprintf_s(
					loadPath,
					BUFF_MASSIVE,
					"%s\\%s",
					files[0],
					files[i]
				);

				// if only 1 file, change loadpath logic
				if (fileCount == 1) {
					sprintf_s(
						loadPath,
						BUFF_MASSIVE,
						"%s",
						files[0]
					);
				}

				printf("loadpath: %s\n", loadPath);

				PCHAR progressDialougeTextBuff =
					vAllocZeroed(BUFF_MASSIVE);
				vGRect progressDialougeRect =
					vUCreateRectCenteredOffset(
						vCreatePosition(0, 0.2f), 1.5f, 0.3f
					);
				vPUPanel progressDialougePanel
					= vUCreatePanelText(GUIStyle, progressDialougeRect,
						vUPanelTextFormat_CenteredComplete,
						progressDialougeTextBuff);
				progressDialougePanel->textSize = 0.05f;

				// load file (and create dialouge)
				vUPanelTextLock(progressDialougePanel);
				sprintf_s(progressDialougeTextBuff, BUFF_MASSIVE,
					"Compiling %s...", files[i]);
				vUPanelTextUnlock(progressDialougePanel);

				// parse
				stbi_set_flip_vertically_on_load(TRUE);
				INT width, height, components;
				PBYTE parsedImg
					= stbi_load(
						loadPath,
						&width,
						&height,
						&components,
						4
					);

				// if SINGLE file, change filename to have correct
				// output
				if (fileCount == 1) {
					puts("reached");
					// update filename to only be last part
					int pathlen = strlen(files[0]);
					for (int j = 0; j < pathlen; j++) {
						if (files[0][pathlen - j - 1] == '\\') {
							files[0] = (files[0] + (pathlen - j));
							break;
						}
					}
				}

				// generate name of file without extensions
				INT fileNameLen = strlen(files[i]);
				PCHAR fileNameWithoutExtension =
					vAllocZeroed(fileNameLen);
				for (int j = 0; j < fileNameLen; j++) {
					char temp = files[i][fileNameLen - j - 1];
					files[i][fileNameLen - j - 1] = NULL;
					if (temp == '.') break;
				}
				CHAR pathOut[MAX_PATH];
				sprintf_s(
					pathOut,
					MAX_PATH,
					"%s\\%s.vci",
					_outDirPath,
					files[i]
				);
				printf("outpath: %s\n", pathOut);
				vFree(fileNameWithoutExtension);

				// free img
				stbi_image_free(parsedImg);

				// free resources
				vUDestroyPanel(progressDialougePanel);
				vFree(progressDialougeTextBuff);
				vFree(loadPath);
			}
		}

		Sleep(1); // pause to regulate thread
	}
}