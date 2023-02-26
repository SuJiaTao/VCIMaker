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

#define SAVED_DIR_FILEPATH "lastdir.txt"

typedef struct TButton {
	vPUPanel background;
	vPUPanel text;
} TButton, *PTButton;

PTButton CreatePTButton(vPUPanelStyle guiStyle, vGRect rect,
	PCHAR text, float tSize) {
	PTButton ptbutton = vAlloc(sizeof(TButton));

	ptbutton->text =
		vUCreatePanelText(guiStyle, rect, vUPanelTextFormat_CenteredComplete,
			tSize, text);
	ptbutton->background =
		vUCreatePanelButton(guiStyle, rect, NULL, NULL);

	return ptbutton;
}

int main(int argc, char** argv) {
	FreeConsole();
	HRESULT initRslt = CoInitialize(NULL);

	// init libraries
	vCoreInitialize();
	vGInitializeData gInitDat;
	gInitDat.targetFrameRate = 120;
	gInitDat.windowName = "VCI Maker";
	gInitDat.windowWidth  = 1000;
	gInitDat.windowHeight = 700;
	vGInitialize(&gInitDat);
	vGSetWindowResizable(FALSE);
	vGSetWindowMaximizeable(FALSE);
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
			vUCreateRectAlignedBorder(
				vURectAlignment_Top,
				vGCreateRectCentered(0.15f, 0.15f),
				0.05f),
			vUPanelTextFormat_Centered, 0.15f, "VCI MAKER");

	// file output directory
	char _outDirPath[BUFF_MASSIVE];
	vZeroMemory(_outDirPath, sizeof(_outDirPath));

	// load last directory (if exists)
	if (vFileExists(SAVED_DIR_FILEPATH)) {
		HANDLE dirFile = vFileOpen(SAVED_DIR_FILEPATH);
		vFileRead(dirFile, 0, vFileSize(dirFile), _outDirPath);
		vFileClose(dirFile);
	}
	else
	{
		GetCurrentDirectoryA(MAX_PATH, _outDirPath);
	}

	// generate dirpanel string
	char _dirPanelStr[BUFF_MASSIVE];
	sprintf_s(_dirPanelStr, sizeof(_dirPanelStr), "Output dir:\n%s", _outDirPath);

	// make directory pannel
	vPUPanel directory =
		vUCreatePanelText(GUIStyle,
			vUCreateRectAlignedOut(
				title->boundingBox,
				vURectAlignment_Top,
				vGCreateRectCentered(0.05f, 0.05f),
				0.02f),
			vUPanelTextFormat_Centered, 0.05f, _dirPanelStr);

	PTButton changeDirButton =
		CreatePTButton(
			GUIStyle,
			vUCreateRectCenteredOffset(
				vCreatePosition(-0.6f, 0.45f),
				1.00f,
				0.25f
			),
			"Change Dir",
			0.07f
		);

	PTButton compileFilesButton =
		CreatePTButton(
			GUIStyle,
			vUCreateRectCenteredOffset(
				vCreatePosition(0.6f, 0.45f),
				1.00f,
				0.25f
			),
			"Compile Files",
			0.07f
		);



	// main loop
	while (vGShouldExit() == FALSE) {

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
			CHAR tempDirBuff[MAX_PATH];
			vZeroMemory(tempDirBuff, MAX_PATH);
			BOOL rslt = SHGetPathFromIDListA(itemID, tempDirBuff);

			// free item ID
			CoTaskMemFree(itemID);

			// if no new path, quit ignore
			if (tempDirBuff[0] == NULL) continue;

			// update output dir and save
			sprintf_s(_outDirPath, BUFF_MASSIVE, "%s", tempDirBuff);
			HANDLE savedWriteFile = vFileCreate(SAVED_DIR_FILEPATH);
			vFileWrite(savedWriteFile, 0, strlen(tempDirBuff), tempDirBuff);
			vFileClose(savedWriteFile);

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
			fopenOptions.lpstrFile = vAllocZeroed(0x2000);
			fopenOptions.nMaxFile  = 0x2000;
			fopenOptions.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR |
				OFN_EXPLORER | OFN_ALLOWMULTISELECT;
			fopenOptions.lpstrFilter = TEXT("All files(*.*)\0*.*\0\0");
			fopenOptions.lpstrFilter = NULL;
			fopenOptions.nFilterIndex = 1;
			BOOL option = GetOpenFileNameA(&fopenOptions);

			// check for nothing selected
			if (option == FALSE) {
				vFree(fopenOptions.lpstrFile);
				continue;
			}


			// parse files
			// files[0] is "root folder"
			// all the rest are filenames
			PCHAR files[BUFF_MASSIVE];
			INT fileCount = 0;
			INT strWalk = 0;
			while (TRUE) {
				if (strWalk > 0x2000) break; // safeguard
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

				PCHAR progressDialougeTextBuff =
					vAllocZeroed(BUFF_MASSIVE);
				vGRect progressDialougeRect =
					vUCreateRectCenteredOffset(
						vCreatePosition(0, 0.2f), 1.5f, 0.3f
					);
				vPUPanel progressDialougePanel
					= vUCreatePanelText(GUIStyle, progressDialougeRect,
						vUPanelTextFormat_CenteredComplete,
						0.05f,
						progressDialougeTextBuff);

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

				// if parse FAILED, skip
				if (parsedImg == NULL) {
					vFree(progressDialougeTextBuff);
					vUDestroyPanel(progressDialougePanel);
					vLogErrorFormatted(__func__, "Failed to compile file %s.\n",
						files[i]);
					continue;
				}

				// if SINGLE file, change filename to have correct
				// output
				if (fileCount == 1) {
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
				vFree(fileNameWithoutExtension);

				// generate vci files
				vGMakeVCI(pathOut, width, height, parsedImg);

				// preview VCI files
				vPGSkin previewSkin = vGCreateSkinFromVCI(pathOut, FALSE, 0);
				float aspect = (float)previewSkin->width / (float)previewSkin->height;
				vPUPanel previewPanel =
					vUCreatePanelRect(GUIStyle,
						vUCreateRectCenteredOffset(
							vCreatePosition(0, -0.25f), 0.75f * aspect, 0.75f),
						previewSkin);

				// sync so that image can be seen
				vGSync();

				// free resources
				vUDestroyPanel(previewPanel);
				vGDestroySkin(previewSkin);

				// free img
				stbi_image_free(parsedImg);

				// free resources
				vUDestroyPanel(progressDialougePanel);
				vFree(progressDialougeTextBuff);
				vFree(loadPath);
			}

			vFree(fopenOptions.lpstrFile);
		}

		Sleep(1); // pause to regulate thread
	}

	vGExit();
}