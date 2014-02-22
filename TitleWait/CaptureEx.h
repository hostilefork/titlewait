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

#include "windows.h"

#ifndef CAPTUREEX_H
#define CAPTUREEX_H

BOOL TakeScreenshotToFile(WCHAR const * pszFile);

#endif
