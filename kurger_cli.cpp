/*
Copyright 11/09/2025 https://github.com/su8/kurger
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
MA 02110-1301, USA.
*/
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <iostream>
#include <string>
#include <algorithm>
#include <sys/stat.h>
#include <sys/types.h>

#define VLA 4999

static size_t indexLastSep(const char *str);
static void pdf2img(const char *str, int spin1, int spin2, int resolution);

int main(int argc, char *argv[]) {
  if (argc < 9) { std::cout << "kurger from 1 to 10 resolution 100 file 'book.pdf' - the quotes are mandatory for the given file" << std::endl; return EXIT_FAILURE; }
  pdf2img(argv[8], std::strtol(argv[2], static_cast<char **>(nullptr), 10), std::strtol(argv[4], static_cast<char **>(nullptr), 10), std::strtol(argv[6], static_cast<char **>(nullptr), 10));
  return EXIT_SUCCESS;
}

static size_t indexLastSep(const char *str) {
  const char *ptr = str;
  size_t sepIndex = 0;
  size_t x = 0;

  for (; *ptr; x++, ptr++) {
#if defined(__OpenBSD__) || defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__)
    if ('/' == *ptr) { sepIndex = x; /* keep in mind that we use loop */ }
#else
    if ('\\' == *ptr) { sepIndex = x; }
#endif /*__OpenBSD__ || __linux__ || __FreeBSD__ || __NetBSD__ */
  }
    return sepIndex;
}

static void pdf2img(const char *str, int spin1, int spin2, int resolution) {
  char imageCombo[] = "png";
  char sdeviceCombo[] = "png16m";
  char pdfName[VLA+1] = {'\0'};
  char BaseName[VLA+1] = {'\0'};
  char params[VLA+1] = {'\0'};
  char ren1[VLA+1] = {'\0'};
  char ren2[VLA+1] = {'\0'};
  char createdDir[VLA+1] = {'\0'};
  int smallRange = (spin2 - spin1) + 2;
  int bigRange = spin1;
  int y = 0;
  size_t dirnameLen = indexLastSep(str);
  size_t x = 0;
  size_t z = 0;
  size_t fit = strlen(str) - 4; /* exclude the .pdf file extension */
  struct stat DiR;

  if (1850U <= fit)  { std::cout << "The given filename is too long!\n" << std::endl; return; }
  if (spin1 > spin2) { std::cout << "From page can't be greater than To page." << std::endl; return; }

  snprintf(pdfName, VLA, "%s", str);
  for (z = 0, x = dirnameLen+1; x < fit; x++, z++) { BaseName[z] = pdfName[x]; /* /path/to/some.pdf -> some      */ }
  BaseName[z] = '\0';

  if (250U < z) { std::cout << "The given filename is too long!" << std::endl; return; }

  snprintf(createdDir, VLA, "%s_converted", pdfName);
  stat(createdDir, &DiR);
  if (0 == S_ISDIR(DiR.st_mode))
   {
#if defined(__OpenBSD__) || defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__)
    mkdir(createdDir, 0700);
#define GS "gs"
#else
    mkdir(createdDir);
#define GS "C:\\gs\\bin\\gswin64c.exe"
#endif /*__OpenBSD__ || __linux__ || __FreeBSD__ || __NetBSD__ */
    }

  snprintf(params, VLA, GS " -dBATCH -dNOPAUSE -dQUIET -dFirstPage=%d -dLastPage=%d "
                    "-sOutputFile=\"%s\"_pAge_%%01d.%s -sDEVICE=%s -r%d "
                    "-dGraphicsAlphaBits=4 -sBandListStorage=memory "
                    "-dBufferSpace=99000 -dNumRenderingThreads=8 \"%s\"",
      spin1, spin2, pdfName, imageCombo, sdeviceCombo, resolution, str);
  std::cout << "Please wait until we convert the requested images." << std::endl;
  system(params);

  for (y = 1; y < smallRange; y++, bigRange++) {
    snprintf(ren1, VLA, "%s_pAge_%d.%s", pdfName, y, imageCombo);
    snprintf(ren2, VLA, "%s/%s_page_%d.%s", createdDir, BaseName, bigRange, imageCombo);
    remove(ren2);
    rename(ren1, ren2);
  }
  std::cout << "Done" << std::endl;
}