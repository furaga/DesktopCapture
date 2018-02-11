#include <windows.h>
#include <stdio.h>
#include <ctime>

void OutputDebugStringFVA(const char *format, va_list args)
{
	int len = _vscprintf(format, args) + 1;
	char * buffer = new char[len];
	vsprintf_s(buffer, len, format, args);
	OutputDebugStringA(buffer);
	delete[] buffer;
}

void Printf(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	OutputDebugStringFVA(format, args);
	va_end(args);
}

HDC hmdc = nullptr;
HBITMAP hbmp = nullptr;
BYTE* line = nullptr;
UINT sizeOfLine = 0;

int captureAndSaveImage(HWND hwnd, HDC hdc, char* fileName, bool saveFile = true) {
	const BYTE bfh[14] = { 0x42, 0x4d, 0,0,0,0, 0,0, 0,0, 0x36,0,0,0 };

	// capture image
	RECT rect;
	GetWindowRect(hwnd, &rect);
	UINT width = rect.right - rect.left;
	UINT height = rect.bottom - rect.top;

	if (hmdc == nullptr) {
		hmdc = CreateCompatibleDC(hdc);
		hbmp = CreateCompatibleBitmap(hdc, width, height);
	}

	HBITMAP hbmpOld = (HBITMAP)SelectObject(hmdc, hbmp);
	BitBlt(hmdc, 0, 0, width, height, hdc, 0, 0, SRCCOPY);
	SelectObject(hmdc, hbmpOld);

	if (line == nullptr) {
		sizeOfLine = width * 3;
		sizeOfLine += (sizeOfLine % 4 != 0 ? 4 - sizeOfLine % 4 : 0);
		if (!saveFile) {
			sizeOfLine *= height;
		}
		line = (BYTE*)malloc(sizeOfLine);
	}

	BITMAPINFO bi;
	ZeroMemory(&bi, sizeof bi);
	bi.bmiHeader.biSize = sizeof bi.bmiHeader;
	bi.bmiHeader.biWidth = width;
	bi.bmiHeader.biHeight = height;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 24;
	bi.bmiHeader.biCompression = BI_RGB;
	bi.bmiHeader.biSizeImage = sizeOfLine * height;

	// save as bitmap image
	FILE* file = nullptr;
	if (saveFile) {
		fopen_s(&file, fileName, "wb");

		*((DWORD*)(&bfh[2])) = 0x36 + sizeOfLine * height;
		fwrite(bfh, sizeof bfh, 1, file);
		fwrite(&(bi.bmiHeader), sizeof bi.bmiHeader, 1, file);
	}

	if (saveFile) {
		for (unsigned int i = 0; i < height; i++) {
			GetDIBits(hmdc, hbmp, i, 1, line, &bi, DIB_RGB_COLORS);
			fwrite(line, sizeOfLine, 1, file);
		}
	}
	else {
		GetDIBits(hmdc, hbmp, 0, height, line, &bi, DIB_RGB_COLORS);
	}

	if (saveFile) {
		fclose(file);
	}
	//free(line);
	//DeleteObject(hbmp);
	//DeleteDC(hmdc);
	return 0;
}

int WINAPI WinMain(HINSTANCE hCurInst, HINSTANCE hPrevInst, LPSTR lpsCmdLine, int nCmdShow)
{
	MSG msg;
	char fileName[1024];
	HWND hwnd = nullptr;
	HDC hdc = nullptr;
	int n = 0;

	// set shortcut keys
	// cf. http://kts.sakaiweb.com/virtualkeycodes.html
	RegisterHotKey(NULL, 1, 0, 0x2C); // Print screen
	RegisterHotKey(NULL, 2, MOD_ALT, 0x2C); // Alt + Print screen
	RegisterHotKey(NULL, 3, 0, 0x1B); // Escape

	clock_t begin, end;
	double elapsed_secs;

	while (GetMessage(&msg, NULL, 0, 0)) {
		if (msg.message == WM_HOTKEY) {
			switch (msg.wParam) {
			case  1:
				// print screen -> capture whole desktop window
				if (hwnd == nullptr) {
					hwnd = GetDesktopWindow();
					hdc = GetDC(NULL);
				}
				sprintf_s(fileName, "%s%03d.bmp", lpsCmdLine, n);


				begin = clock();
				for (int i = 0; i < 100; i++) {
					captureAndSaveImage(hwnd, hdc, fileName, false);
				}
				end = clock();
				elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
				Printf("elapsed time = %f s | average = %f s", elapsed_secs, elapsed_secs / 100);

				// ReleaseDC(NULL, hdc);
				n++;
				Printf("captured desktop window: %s", fileName);
				break;
			case  2:
				//// Alt + print screen -> capture foreground window
				//hwnd = GetForegroundWindow();
				//hdc = GetWindowDC(hwnd);
				//sprintf_s(fileName, "%s%03d.bmp", lpsCmdLine, n);
				//captureAndSaveImage(hwnd, hdc, fileName);
				//ReleaseDC(hwnd, hdc);
				//n++;
				//Printf("captured foreground window: %s", fileName);
				//break;
			case 3:
				// Escape -> Quit application
				PostQuitMessage(0);
			}
		}
	}

	return msg.wParam;
}
