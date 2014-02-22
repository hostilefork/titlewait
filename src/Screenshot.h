//
// Screenshot.h
// Copyright (c) 2008 HostileFork.com
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

#ifndef __TITLEWAIT_SCREENSHOT_H__
#define __TITLEWAIT_SCREENSHOT_H__

#include "windows.h"

BOOL TakeScreenshotToFile(WCHAR const * pszFile);

#endif
