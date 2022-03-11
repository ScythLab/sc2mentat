#include "ocr.h"
#include "tools.h"
#include "log.h"
#include <windows.h>
#include <string>

namespace ocr
{
	// ����� �� �������
	#define DIGIT_NONE -1
	// ��������� ���� ������� ��� ������, ������ ����� ������:
	// - ������� ���;
	// - ������ ������� ����;
	// - ����� 8 ����� ����� �������������� � ��� 8, � ��� 3.
	#define DIGIT_MULTIPLE -2

	int debugFileCounter = 0;
	bool debugEnabled = false;

	int comparePix(Pix *pattern, Pix *image, int x, int white, int* whitePixels)
	{
		if (pattern->w + x > image->w)
			return 0;

		int samePixels = 0; // ���-�� ���������� ������� �������� ������� � ������������
		*whitePixels = 0; // "������" ����� (���-�� ������� ��������).
		RGBA_Quad* colors = (RGBA_Quad*)pattern->data;
		for (l_uint32 yy = 0; yy < pattern->h; yy++)
		{
			RGBA_Quad* imColors = (RGBA_Quad*)image->data + yy * image->w + x;
			for (l_uint32 xx = 0; xx < pattern->w; xx++)
			{
				l_uint8 colorPattern = (colors->green + colors->red + colors->reserved) / 3;
				l_uint8 colorImage = (imColors->green + imColors->red + imColors->reserved) / 3;
				bool isPatternWhite = colorPattern > white;
				bool isImageWhite = colorImage >= colorPattern; // �� ����������� ������� ������ ������ ������ ���� ����� �� ��� �������, ��� � �������.
				if (isPatternWhite)
				{
					(*whitePixels)++;
					if (isImageWhite)
						samePixels++;
				}
				// INFO: ������ ��������� ������ ������� �������, �.�. � ����������� ����� ���������� ������� ���.
				//else
				//{
				//	if (isImageWhite)
				//		samePixels--;
				//}

				colors++;
				imColors++;
			}
		}

		//log_dbg_printf("%d) white: %d / %d = %d", x, whitePixels, samePixels, 100 * samePixels / whitePixels);
		return 100 * samePixels / *whitePixels;
	}

	//---------------------------------------------------------------------------//
	//---------------------------------- Font -----------------------------------//
	//---------------------------------------------------------------------------//

	Font::Font(LPCWSTR path, std::vector<LPCWSTR> files, char* symbols, int size, int white)
	{
		if (strlen(symbols) != size)
			throw std::invalid_argument(std::format(lng::string(lng::LS_FONT_SIZE_INVALID), symbols, size));

		symbols_ = (char*)malloc(size + 1);
		memset(symbols_, 0, size + 1);
		images_ = (Pix**)malloc(size * sizeof(Pix*));
		memset(images_, 0, size * sizeof(Pix*));

		strcpy(symbols_, symbols);
		size_ = size;
		white_ = white;
		height_ = 0;

		TCHAR fileName[MAX_PATH];
		for (int i = 0; i < size; i++)
		{
			wcscpy(fileName, path);
			wcscat(fileName, L"\\");
			wcscat(fileName, files[i]);
			Pix* im = pixRead(fileName);
			if (!im)
				throw std::invalid_argument(std::format(lng::string(lng::LS_FONT_LOAD_FAIL), files[i]));

			images_[i] = im;
			height_ = max(height_, im->h);
		}
	}

	Font::~Font()
	{
		free(symbols_); symbols_ = NULL;
		for (int i = 0; i < size_; i++)
		{
			pixDestroy(images_ + i);
		}
		free(images_); images_ = NULL;
	}

	//---------------------------------------------------------------------------//
	//--------------------------------- Engine ----------------------------------//
	//---------------------------------------------------------------------------//

	Engine::Engine(LPCWSTR fontPath)
	{
		wcscpy(fontPath_, fontPath);
	}

	Engine::~Engine()
	{
		clear();
	}

	void Engine::clear()
	{
		SafeVectorDelete(fontList_);
	}

	int Engine::addFont(LPCWSTR subPath, std::vector<LPCWSTR> files, char* symbols, int size, int white)
	{
		TCHAR fontPath[MAX_PATH];
		wcscpy(fontPath, fontPath_);
		wcscat(fontPath, subPath);

		Font* font = new Font(fontPath, files, symbols, size, white);
		fontList_.push_back(font);
		return fontList_.size() - 1;
	}

	int Engine::getText(Pix* image, int x, int y, int w, int fontIndex, char* text, int textSize)
	{
		#define ERASE_TEXT {textLen = 0; *text = 0; }

		memset(text, 0, textSize);
		if (fontIndex > fontList_.size())
			return -1;
		Font* font = fontList_[fontIndex];

		int h = font->getHeight();
		Pix* im = pixClipRectangle(image, x, y, w, h);
		if (!im)
			return -1;

		//pixWrite("test.bmp", im, IFF_BMP);

		int textLen = 0;

		int xOffset = 0;
		int prevBestDigit = -1;
		while (true)
		{
			int maxPercent = -1;
			int bestOffset = -1;
			int bestDigit = -1;
			int maxSymbolSize = -1;

			// ������� ��������� ��������, �.�. ����� ������� ��������� ����� ���� ������ ����������.
			// ������ "/" �������� �� ���������� ������ �� ���� ��������: ���������� ���� ������� ��������� ���, �� ����� ��������� �������� ��� ������� � �������.
			for (int x = 0; x < 10 && maxPercent < 100; x++)
			{
				int percent = -1;
				int symbolSize = -1;
				int digit = getSymbol(im, xOffset + x, &percent, &symbolSize, font);

				// ���� ������������ ��������� ��������, ������ � ����� � ����������� ������ ������ � ����� ������ ����������.
				if (DIGIT_MULTIPLE == digit)
				{
					//// DEBUG: ���������� "������" ����.
					//if (textLen) // && debugEnabled
					//{
					//	debugFileCounter++;
					//	char fileName[100];
					//	sprintf(fileName, "debug_%d_%s.bmp", debugFileCounter, text);
					//	pixWrite(fileName, im, IFF_BMP);
					//}

					ERASE_TEXT;
					break;
				}

				// INFO: ����� ������������ �������� �� �� "�������" �� ���� �������� ����������.
				if (digit >= 0 && percent > maxPercent) // || (percent == maxPercent && symbolSize > maxSymbolSize))
				{
					bestOffset = x;
					bestDigit = digit;
					maxPercent = percent;
					maxSymbolSize = symbolSize;
				}
			}

			if (maxPercent != 100 || -1 == bestDigit)
			{
				// ���� ��������� ������ � 100%, �� �������� ����� ��������� ��������,
				// �� ������ ������ ������� ���������.
				if (maxPercent > 90)
				{
					// TODO: ���������������� ���.
					//log_dbg_printf("#%d with %d%%; text: %s", bestDigit, maxPercent, text);

					ERASE_TEXT;
				}

				break;
			}

			text[textLen++] = font->getSymbol(bestDigit);
			if (textLen >= textSize - 1)
				break;

			//log_dbg_printf("found %d with %d%%", bestDigit, maxPercent);
			xOffset += font->getImage(bestDigit)->w + bestOffset;
			prevBestDigit = bestDigit;
		}

		pixDestroy(&im);

		return textLen;
	}

	int Engine::getSymbol(Pix *image, int x, int* maxPercent, int* maxSymbolSize, Font* font)
	{
		*maxSymbolSize = -1;
		*maxPercent = -1;
		int bestDigit = DIGIT_NONE;
		int matchCount = 0; // ���-�� ����/�������� �� 100% ��������� � ������������
		for (int d = 0; d < font->getSize(); d++)
		{
			int symbolSize;
			int percent = comparePix(font->getImage(d), image, x, font->getWhite(), &symbolSize);
			if (100 == percent)
				matchCount++;
			if (percent > *maxPercent)// || (percent == *maxPercent && symbolSize > *maxSymbolSize))
			{
				*maxPercent = percent;
				*maxSymbolSize = symbolSize;
				bestDigit = d;
			}
		}

		// ���� ��������� ���� ������������ �� 100% ���������,
		// ������ � �������� ������� ����� ������ � ����� ��������� ������ ���������.
		// BUG: ��������� ����� �������� �������������� �� ������ � ������������� �������,
		// �� �� ������ ������ �� ����.
		if (matchCount > 1)
		{
			//log_dbg_printf("matchCount: %d", matchCount);
			return DIGIT_MULTIPLE;
		}

		return bestDigit;
	}
}
