//
// Screenshot.cpp
// Copyright (c) 2008-2014 HostileFork.com
//
// This file is part of TitleWait
// See http://titlewait.hostilefork.com
//
// TitleWait is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// TitleWait is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with TitleWait.  If not, see <http://www.gnu.org/licenses/>.
//


//
// Originally I used
//   http://www.codeguru.com/cpp/g-m/gdi/capturingimages/print.php/c11231
//
// However, when restoring the code in 2014 I found it wasn't working.
// I went to MSDN as a more authoritative source, which provided code
// that seemed to work better for this simple application.
//
// There are obviously a lot of things that could be added; like trying
// to detect which specific window areas on the screen are related to
// the application (a tricky problem in general) or saving to other
// formats.  But this just uses BMP and adds no additional dependencies.
//


#include "HelperFunctions.h"

#include "Screenshot.h"


//
// From http://lars.werner.no/?p=627
//
HBITMAP ScreenShot(HWND hParent, int x, int y, int nWidth, int nHeight)
{
    // Get a DC from the parent window
    HDC hDC = GetDC(hParent);

    // Create a memory DC to store the picture to
    HDC hMemDC = CreateCompatibleDC(hDC);

    // Create the actual picture
    HBITMAP hBackground = CreateCompatibleBitmap(hDC, nWidth, nHeight );

    // Select the object and store what we got back
    HBITMAP hOld = (HBITMAP)SelectObject(hMemDC, hBackground);

    // Actually paint into the MemDC (result will be in the selected object)
    // Note: We ask to return on 0,0,Width,Height and take a blit from x,y
    BitBlt(hMemDC, 0, 0, nWidth, nHeight, hDC, x, y, SRCCOPY);

    // Restore the old bitmap (if any)
    SelectObject(hMemDC, hOld);

    // Release the DCs we created
    ReleaseDC(hParent, hMemDC);
    ReleaseDC(hParent, hDC);

    // Return the picture (not a clean method, but you get the drill)
    return hBackground;
}


//
// From http://msdn.microsoft.com/en-us/library/windows/desktop/dd145119(v=vs.85).aspx
//
PBITMAPINFO CreateBitmapInfoStruct(HWND hwnd, HBITMAP hBmp)
{
    BITMAP bmp;
    PBITMAPINFO pbmi;
    WORD    cClrBits;

    // Retrieve the bitmap color format, width, and height.
    Verify(L"GetObject",GetObject(hBmp, sizeof(BITMAP), (LPSTR)&bmp));

    // Convert the color format to a count of bits.
    cClrBits = (WORD)(bmp.bmPlanes * bmp.bmBitsPixel);
    if (cClrBits == 1)
        cClrBits = 1;
    else if (cClrBits <= 4)
        cClrBits = 4;
    else if (cClrBits <= 8)
        cClrBits = 8;
    else if (cClrBits <= 16)
        cClrBits = 16;
    else if (cClrBits <= 24)
        cClrBits = 24;
    else cClrBits = 32;

    // Allocate memory for the BITMAPINFO structure. (This structure
    // contains a BITMAPINFOHEADER structure and an array of RGBQUAD
    // data structures.)

     if (cClrBits < 24) {
         pbmi = (PBITMAPINFO) LocalAlloc(LPTR,
                    sizeof(BITMAPINFOHEADER) +
                    sizeof(RGBQUAD) * (1<< cClrBits));
     } else {
        // There is no RGBQUAD array for these formats:
        // 24-bit-per-pixel or 32-bit-per-pixel

        pbmi = (PBITMAPINFO) LocalAlloc(LPTR, sizeof(BITMAPINFOHEADER));
    }

    // Initialize the fields in the BITMAPINFO structure.

    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth = bmp.bmWidth;
    pbmi->bmiHeader.biHeight = bmp.bmHeight;
    pbmi->bmiHeader.biPlanes = bmp.bmPlanes;
    pbmi->bmiHeader.biBitCount = bmp.bmBitsPixel;
    if (cClrBits < 24)
        pbmi->bmiHeader.biClrUsed = (1<<cClrBits);

    // If the bitmap is not compressed, set the BI_RGB flag.
    pbmi->bmiHeader.biCompression = BI_RGB;

    // Compute the number of bytes in the array of color
    // indices and store the result in biSizeImage.
    // The width must be DWORD aligned unless the bitmap is RLE
    // compressed.
    pbmi->bmiHeader.biSizeImage =
        ((pbmi->bmiHeader.biWidth * cClrBits +31) & ~31) /8
        * pbmi->bmiHeader.biHeight;

    // Set biClrImportant to 0, indicating that all of the
    // device colors are important.
    pbmi->bmiHeader.biClrImportant = 0;
    return pbmi;
 }


//
// From http://msdn.microsoft.com/en-us/library/windows/desktop/dd145119(v=vs.85).aspx
//
void CreateBMPFile(
    HWND hwnd, WCHAR const * pszFile, PBITMAPINFO pbi, HBITMAP hBMP, HDC hDC
) {
    HANDLE hf;                 // file handle
    BITMAPFILEHEADER hdr;       // bitmap file-header
    PBITMAPINFOHEADER pbih;     // bitmap info-header
    LPBYTE lpBits;              // memory pointer
    DWORD dwTotal;              // total count of bytes
    DWORD cb;                   // incremental count of bytes
    BYTE *hp;                   // byte pointer
    DWORD dwTmp;

    pbih = (PBITMAPINFOHEADER) pbi;
    lpBits = (LPBYTE) GlobalAlloc(GMEM_FIXED, pbih->biSizeImage);

    Verify(L"GlobalAlloc", lpBits != NULL);

    // Retrieve the color table (RGBQUAD array) and the bits
    // (array of palette indices) from the DIB.
    Verify(L"GetDIBits", GetDIBits(
        hDC, hBMP, 0, (WORD) pbih->biHeight, lpBits, pbi, DIB_RGB_COLORS)
    );

    // Create the .BMP file.
    hf = CreateFile(pszFile,
                   GENERIC_READ | GENERIC_WRITE,
                   (DWORD) 0,
                    NULL,
                   CREATE_ALWAYS,
                   FILE_ATTRIBUTE_NORMAL,
                   (HANDLE) NULL);
    Verify(L"CreateFile", hf != INVALID_HANDLE_VALUE);

    hdr.bfType = 0x4d42;        // 0x42 = "B" 0x4d = "M"
    // Compute the size of the entire file.
    hdr.bfSize = (DWORD) (sizeof(BITMAPFILEHEADER) +
                 pbih->biSize + pbih->biClrUsed
                 * sizeof(RGBQUAD) + pbih->biSizeImage);
    hdr.bfReserved1 = 0;
    hdr.bfReserved2 = 0;

    // Compute the offset to the array of color indices.
    hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) +
                    pbih->biSize + pbih->biClrUsed
                    * sizeof (RGBQUAD);

    // Copy the BITMAPFILEHEADER into the .BMP file.
    Verify(L"WriteFile", WriteFile(hf, (LPVOID) &hdr, sizeof(BITMAPFILEHEADER),
        (LPDWORD) &dwTmp,  NULL));

    // Copy the BITMAPINFOHEADER and RGBQUAD array into the file.
    Verify(L"WriteFile", WriteFile(hf, (LPVOID) pbih, sizeof(BITMAPINFOHEADER)
                  + pbih->biClrUsed * sizeof (RGBQUAD),
                  (LPDWORD) &dwTmp, ( NULL)));

    // Copy the array of color indices into the .BMP file.
    dwTotal = cb = pbih->biSizeImage;
    hp = lpBits;
    Verify(L"WriteFile",
        WriteFile(hf, (LPSTR) hp, (int) cb, (LPDWORD) &dwTmp,NULL)
    );

    // Close the .BMP file.
     Verify(L"CloseHandle", CloseHandle(hf));

    // Free memory.
    GlobalFree((HGLOBAL)lpBits);
}


//
// Simple invocation of the mixture of the above routines to get a screenshot
//
// Downside: Just a BMP.
// Upside: No dependencies on GDI+ or any libraries
//
BOOL TakeScreenshotToFile(WCHAR const * pszFile) {
    HWND hwndDesktop = GetDesktopWindow();
    RECT rc;
    GetWindowRect(hwndDesktop, &rc);

    HDC hdc = GetDC(hwndDesktop);
    HBITMAP hbitmap = ScreenShot(
        hwndDesktop, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top
    );
    PBITMAPINFO pbitmapinfo = CreateBitmapInfoStruct(hwndDesktop, hbitmap);
    CreateBMPFile(hwndDesktop, pszFile, pbitmapinfo, hbitmap, hdc);
    ReleaseDC(hwndDesktop, hdc);
    DeleteObject(hbitmap);
    LocalFree(pbitmapinfo);

    // For now, assume if any of the above failed it would terminate program
    return TRUE;
}
