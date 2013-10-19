// CaptureEx.cpp: implementation of the CaptureEx class.

// Taken from http://www.codeguru.com/cpp/g-m/gdi/capturingimages/print.php/c11231

// Author: Golan Shahar 1.1.2006

// CodeGuru license policy for submissions: "While we are talking about
// copyrights, you retain copyright of your article and code, but by
// submitting it to CodeGuru you give permission to use it in a fair manner
// and also permit all developers to freely use the code in their own
// applications -- even if they are commercial."

#include <cstdio>
#include "HelperFunctions.h"
#include "CaptureEx.h"

CaptureEx::CaptureEx()
{
	m_pLastImage = 0;
	m_Width = 0;
	m_Height = 0;
	m_Bpp = 0;
}

CaptureEx::~CaptureEx()
{
	if (m_pLastImage)
	{
		delete[] m_pLastImage;
		m_pLastImage = 0;
	}
}

void CaptureEx::SaveBmpToFile(
	LPCWSTR szFileName,
	int W,
	int H,
	int Bpp,
	int* lpBits)
{
	BITMAPINFO Bmi = {0};
	Bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	Bmi.bmiHeader.biWidth = W;
	Bmi.bmiHeader.biHeight = H;
	Bmi.bmiHeader.biPlanes = 1;
	Bmi.bmiHeader.biBitCount = Bpp; 
	Bmi.bmiHeader.biCompression = BI_RGB;
	Bmi.bmiHeader.biSizeImage = W * H * Bpp / 8; 

	FILE* fp = _wfopen(szFileName, L"wb");
	if(fp ==0)
		return;
	int h = Bmi.bmiHeader.biHeight;
	int w = Bmi.bmiHeader.biWidth;
	Bmi.bmiHeader.biHeight = -h;
	Bmi.bmiHeader.biWidth = w;

	BITMAPFILEHEADER bfh = {0};
	bfh.bfType = ('M' << 8) + 'B'; 
	bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER); 
	bfh.bfSize = Bmi.bmiHeader.biSizeImage + bfh.bfOffBits; 
   
	fwrite(&bfh, sizeof(bfh), 1, fp);
	fwrite(&Bmi.bmiHeader, sizeof(BITMAPINFOHEADER), 1, fp);
	fwrite(lpBits,Bmi.bmiHeader.biSizeImage, 1, fp);
	fclose(fp);
}

void CaptureEx::Get24BitBmp(
	const int &nWidth,
	const int &nHeight,
	const HBITMAP &hBitmap,
	BYTE *lpDesBits)
{
	HDC hDC = ::GetDC(0);

	HDC memDC1 = ::CreateCompatibleDC(hDC);
	HDC memDC2 = ::CreateCompatibleDC(hDC);

	BYTE *lpBits = NULL;

	BITMAPINFO bmi;
	::ZeroMemory(&bmi, sizeof(BITMAPINFO));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = nWidth;
	bmi.bmiHeader.biHeight = nHeight;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 24;
	bmi.bmiHeader.biCompression = BI_RGB;

	HBITMAP hDIBMemBM =
		::CreateDIBSection(
			0, &bmi, DIB_RGB_COLORS, (void**)&lpBits, NULL, NULL
		);
	
	HBITMAP hOldBmp1 = (HBITMAP)::SelectObject(memDC1, hDIBMemBM);

	HBITMAP hOldBmp2 = (HBITMAP)::SelectObject(memDC2, hBitmap);

	::BitBlt(memDC1, 0, 0, nWidth, nHeight, memDC2, 0, 0, SRCCOPY);

	for (int i = 0 ; i < nHeight; i++)
		::CopyMemory(
			&lpDesBits[i * 3 *nWidth],
			&lpBits[ nWidth * 3 * (nHeight - 1 - i)],
			nWidth * 3);

	// clean up
	::SelectObject(memDC1, hOldBmp1);
	::SelectObject(memDC2, hOldBmp2);
	::ReleaseDC(0, hDC);
	::DeleteObject(hDIBMemBM);
	::DeleteObject(hOldBmp1);
	::DeleteObject(hOldBmp2);
	::DeleteDC(memDC1);
	::DeleteDC(memDC2);
}

BOOL CaptureEx::CaptureWindow(
	HWND hWndSrc,
	double fPreviewRatio,
	LPCWSTR lpszFileName,
	BOOL bSaveToFile)
{
	RECT rc = {0};
	::GetWindowRect(hWndSrc,&rc);
	int Width	= rc.right - rc.left;
	int Height	= rc.bottom - rc.top;
	Width		= (Width / 4) * 4;

	HDC		hdc		= ::GetDC(0);
	HDC		memDC	= ::CreateCompatibleDC(hdc);
	HBITMAP memBM	= ::CreateCompatibleBitmap(hdc, Width, Height);
	HBITMAP hOld	= (HBITMAP)::SelectObject(memDC, memBM);

	int Bpp = ::GetDeviceCaps(hdc,BITSPIXEL);
	int size = (Bpp/8) * (Width * Height);
    BYTE *lpBits1 = new BYTE[size];    
    BYTE *lpBits2 = new BYTE[Width * Height*3];    
    BYTE *lpBits3 = new BYTE[Width * Height*3];    

	BOOL Ret = TRUE;
	HBITMAP hBmp = 0;
	int MinBlackPixels = Width * Height * 10;
	int Count = 0;
	int Size24 = Width * Height * 3;
	while (Count < 5)
	{
		Ret = Capture(hWndSrc, memDC);
		::GetBitmapBits(memBM, size, lpBits1);    
		hBmp = ::CreateBitmap(Width, Height, 1, Bpp, lpBits1);
		Get24BitBmp(Width, Height, hBmp, lpBits2);
		int BlackPixels = 0;
		for (int i = 0; i < Size24; i += 3)
		{
			if (lpBits2[i+2]==0 and lpBits2[i+1]==0 and lpBits2[i+0] == 0)
				BlackPixels++;
		}

		if (BlackPixels < MinBlackPixels)
		{
			MinBlackPixels = BlackPixels;
			::memcpy(lpBits3, lpBits2, Size24);
			Count=0;
		}
		Count++;
		::DeleteObject(hBmp);
	}
	::memcpy(lpBits2, lpBits3, Size24);

	UpdateLastImage(Width, Height, Size24, 24, lpBits2);

	if (bSaveToFile)
		SaveBmpToFile(lpszFileName, Width, Height, 24, (int*)lpBits2);

	delete[] lpBits1;
	delete[] lpBits2;
	delete[] lpBits3;
	::SelectObject(memDC, hOld);
	::DeleteObject(memBM);
	::DeleteObject(hBmp);
	::DeleteDC(memDC);
	::ReleaseDC(0, hdc);   

	return Ret;
}

void CaptureEx::CaptureDesktop(
	double fPreviewRatio,
	LPCWSTR lpszFileName,
	BOOL bSaveToFile)
{
	RECT rc;
	HWND hWnd = ::GetDesktopWindow();
	::GetWindowRect(hWnd, &rc); 

	int Width = rc.right - rc.left;
	int Height = rc.bottom - rc.top;

	HDC hDC = ::GetDC(0);
	HDC memDC = ::CreateCompatibleDC(hDC);
	HBITMAP memBM = ::CreateCompatibleBitmap(hDC, Width, Height);
	HBITMAP OldBM = (HBITMAP)::SelectObject(memDC, memBM);
	::BitBlt(memDC, 0, 0, Width, Height, hDC, rc.left, rc.top, SRCCOPY);

	int Bpp	= ::GetDeviceCaps(hDC,BITSPIXEL);
	int size = Bpp/8 * (Width * Height);
	BYTE *lpBits1 = new BYTE[size];
	::GetBitmapBits(memBM, size, lpBits1);

	HBITMAP hBmp = ::CreateBitmap(Width, Height, 1, Bpp, lpBits1);

    BYTE *lpBits2 = new BYTE[Width * Height * 3];    

	Get24BitBmp(Width, Height, hBmp, lpBits2);

	UpdateLastImage( Width, Height,Width * Height*3, 24, lpBits2);


	if (bSaveToFile)
		SaveBmpToFile(lpszFileName, Width, Height, 24, (int*)lpBits2);

	delete[] lpBits1;
	delete[] lpBits2;
	::SelectObject(hDC, OldBM);
	::DeleteObject(memBM);
	::DeleteObject(hBmp);
	::DeleteDC(memDC);
	::ReleaseDC(0, hDC);
}

void CaptureEx::StretchDIBitsToHwnd(
	HWND hWnd,
	double fRatio,
	int Width,
	int Height,
	int Bpp,
	BYTE *lpBits)
{
	HDC hDC = ::GetDC(hWnd);
	BITMAPINFO bmp = {0};
    bmp.bmiHeader.biSize = sizeof(BITMAPINFO);
    bmp.bmiHeader.biWidth = Width;
    bmp.bmiHeader.biHeight = -Height;
    bmp.bmiHeader.biPlanes = 1;
    bmp.bmiHeader.biBitCount = Bpp;
    bmp.bmiHeader.biCompression = BI_RGB;
    bmp.bmiHeader.biSizeImage =
		(bmp.bmiHeader.biWidth * bmp.bmiHeader.biHeight * Bpp /8);

	double fScaleWidth	= Width * fRatio;
	double fScaleHeight	= Height * fRatio;

	::SetWindowPos(
		hWnd,
		0, // hwndInsertAfter
		0, 0,
		(int)fScaleWidth, (int)fScaleHeight,
		SWP_NOMOVE | SWP_NOZORDER
	);
	::StretchDIBits(
		hDC,
		0, 0, (int)fScaleWidth, (int)fScaleHeight,
		0, 0, Width, Height,
		lpBits, &bmp, DIB_RGB_COLORS, SRCCOPY
	);	

	::ReleaseDC(hWnd,hDC);
}

BOOL CaptureEx::Capture(HWND hwnd, HDC memDC)
{
	typedef BOOL (WINAPI *tPrintWindow)(HWND, HDC, UINT);

    tPrintWindow pPrintWindow = 0;
    HINSTANCE handle = ::LoadLibrary(L"User32.dll");
    if ( handle == 0 ) 
		return FALSE;

	pPrintWindow = (tPrintWindow)::GetProcAddress(handle, "PrintWindow");     
    int Ret = TRUE;
	if (pPrintWindow) 
		Ret = pPrintWindow(hwnd, memDC,0 );
	else
	{
		debugInfo(L"cant gain address of PrintWindow(..) api\nplease update your sdk");
		Ret = FALSE;
	}
	::FreeLibrary(handle);
	return (Ret? TRUE : FALSE);
}

void CaptureEx::UpdateLastImage(
	int Width,
	int Height,
	int Size,
	int Bpp,
	BYTE* lpBits)
{
	if (m_pLastImage)
	{
		delete[] m_pLastImage;
		m_pLastImage = 0;
	}

	if (m_pLastImage == 0) // first time
		m_pLastImage = new BYTE[Size];

	memcpy(m_pLastImage, lpBits, Size);
	m_Width	= Width;
	m_Height = Height;
	m_Bpp = Bpp;
}

void CaptureEx::DisplayLastImage(HWND hWnd, double fRatio)
{
	if (m_pLastImage)
		StretchDIBitsToHwnd(hWnd, fRatio, m_Width, m_Height, m_Bpp, m_pLastImage);
}

void CaptureEx::SaveLastCaptured(LPCWSTR szFileName)
{
	if (m_pLastImage)
		SaveBmpToFile(szFileName, m_Width, m_Height, m_Bpp, (int*)m_pLastImage);
}
