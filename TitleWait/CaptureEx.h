// CaptureEx.h: interface for the CaptureEx class.

// Taken from http://www.codeguru.com/cpp/g-m/gdi/capturingimages/print.php/c11231

// Author: Golan Shahar 1.1.2006

// CodeGuru license policy for submissions: "While we are talking about
// copyrights, you retain copyright of your article and code, but by
// submitting it to CodeGuru you give permission to use it in a fair manner
// and also permit all developers to freely use the code in their own
// applications -- even if they are commercial."

#include "windows.h"

#ifndef CAPTUREEX_H
#define CAPTUREEX_H

class CaptureEx  
{
public:
	CaptureEx();
	virtual ~CaptureEx();

	void SaveBmpToFile(LPCWSTR szFileName, int W, int H, int Bps, int* lpBits);

	void Get24BitBmp(const int &nWidth, const int &nHeight, const HBITMAP &hBitmap, BYTE *lpDesBits);

	void CaptureDesktop(double fPreviewRatio, LPCWSTR lpszFileName, BOOL bSaveToFile);

	void StretchDIBitsToHwnd(HWND hWnd, double fRatio, int Width, int Height, int Bps, BYTE *lpBits);

	BOOL CaptureWindow(HWND hWndSrc, double fPreviewRatio, LPCWSTR lpszFileName, BOOL bSaveToFile);

	void DisplayLastImage(HWND hWnd, double fRatio);

	void SaveLastCaptured(LPCWSTR szFileName);

private:
	BOOL Capture(HWND hwnd, HDC memDC);
	void UpdateLastImage(int Width, int Height, int Size, int Bps, BYTE* lpBits);

	BYTE *m_pLastImage;
	int m_Width;
	int m_Height;
	int m_Bpp;
};
#endif

