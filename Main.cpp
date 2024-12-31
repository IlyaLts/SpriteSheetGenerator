/*
===============================================================================
    Copyright (C) 2024 Ilya Lyakhovets

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
===============================================================================
*/

#include <windows.h>
#include <cstdio>
#include <cmath>
#include <clocale>
#include "libtga/tga.h"

#define LTC_VERSION 1

int GetNumOfSprites(const wchar_t *name)
{
    int sprites = 0;

    for (int i = 0; ; i++)
    {
        wchar_t path[1024];
        wsprintf(path, L"%s%d.tga", name, i);

        FILE *f;
        _wfopen_s(&f, path, L"rb");
        if (!f)
            break;

        fclose(f);
        sprites++;
    }

    return sprites;
}

int NextPower2(int n)
{
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    return n + 1;
}

int wmain(int argc, WCHAR *argv[])
{
    setlocale(LC_ALL, ".UTF8");

    if (argc < 3)
    {
        wprintf(L"Usage: %s <tga sprite name> <format>\n\n"
                L"Formats:\n"
                L"0 - Uncompressed, 8-bit color-mapped image.\n"
                L"1 - Uncompressed, 24-bit or 32-bit true-color image.\n"
                L"2 - Uncompressed, 15-bit or 16-bit true-color image.\n"
                L"3 - Uncompressed, 16-bit black-and-white image.\n"
                L"4 - Uncompressed, 8-bit black-and-white image.\n"
                L"5 - Run-length encoded, 8-bit color-mapped image.\n"
                L"6 - Run-length encoded, 24-bit or 32-bit true-color image.\n"
                L"7 - Run-length encoded, 15-bit or 16-bit true-color image.\n"
                L"8 - Run-length encoded, 16-bit black-and-white image.\n"
                L"9 - Run-length encoded, 8-bit black-and-white image.\n", argv[0]);

        return -1;
    }
    
    tga_image output;
    wchar_t path[1024];
    wchar_t path2[1024];
    tga_type mode = static_cast<tga_type>(_wtoi(argv[2]));
    int numOfSprites = GetNumOfSprites(argv[1]);
    int columns;
    int rows;
    int currentColumn = 1;
    int cursor;
    memset(&output, 0, sizeof(output));

    if (!numOfSprites)
    {
        wprintf(L"%s - couldn't find sprites.\n", argv[1]);
        return -1;
    }

    wsprintf(path2, L"%s.tga.ltc", argv[1]);
    FILE *mf;
    _wfopen_s(&mf, path2, L"wb");
    if (!mf)
    {
        wprintf(L"%s - couldn't create UV map file.\n", path2);
        return -1;
    }

    for (int i = 0; i < numOfSprites; i++)
    {
        tga_image tga;
        wsprintf(path, L"%s%d.tga", argv[1], i);

        if (!wload_tga(path, &tga))
        {
            wprintf(L"%s - couldn't open.\n", path);
            return -1;
        }

        if (!output.data)
        {
            int version = LTC_VERSION;
            fwrite(&version, sizeof(version), 1, mf);
            fwrite(&numOfSprites, sizeof(numOfSprites), 1, mf);

            output.width = output.height = NextPower2(tga.width > tga.height ? tga.width : tga.height);

            while (1)
            {
                columns = static_cast<int>(floor(static_cast<float>(output.width) / static_cast<float>(tga.width)));
                rows = static_cast<int>(floor(static_cast<float>(output.height) / static_cast<float>(tga.height)));

                if (columns * rows >= numOfSprites)
                    break;

                output.width *= 2;
                output.height *= 2;
            }

            output.channels = tga.channels;
            int size = output.width * output.height * output.channels;
            output.data = new unsigned char[size];
            cursor = size - (output.width * tga.height * output.channels);
            memset(output.data, 0, size);
        }

        int outputWidth = output.width * output.channels;
        int tgaWidth = tga.width * tga.channels;

        for (unsigned int j = 0; j < tga.height; j++)
            memcpy(&output.data[cursor + outputWidth * j], &tga.data[tgaWidth * j], tgaWidth);

        int currentPixel = cursor / output.channels;

        int x1 = (currentPixel + output.width * (tga.height - 1)) % output.width;
        int y2 = static_cast<int>(ceil(static_cast<float>(((output.width * output.height) - currentPixel)) / output.width));
        int x2 = x1 + tga.width;
        int y1 = y2 - tga.height;

        float u1 = static_cast<float>(x1) / output.width;
        float v1 = static_cast<float>(y1) / output.height;
        float u2 = static_cast<float>(x2) / output.width;
        float v2 = static_cast<float>(y2) / output.height;

        fwrite(&u1, sizeof(u1), 1, mf);
        fwrite(&v1, sizeof(v1), 1, mf);
        fwrite(&u2, sizeof(u1), 1, mf);
        fwrite(&v2, sizeof(v2), 1, mf);

        if (currentColumn < columns)
        {
            cursor += tgaWidth;
            currentColumn++;
        }
        else
        {
            cursor -= outputWidth * tga.height;
            cursor -= tgaWidth * (currentColumn - 1);
            currentColumn = 1;
        }
    }

    fclose(mf);

    wsprintf(path, L"%s.tga", argv[1]);

    if (wsave_tga(path, &output, mode))
        wprintf(L"%s.tga - success\n", argv[1]);
    else
        wprintf(L"%s.tga - failure\n", argv[1]);

    return 0;
}
