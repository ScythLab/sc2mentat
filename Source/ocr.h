#pragma once
#include "pix.h"
#include <list>
#include <vector>
#include <windows.h>

namespace ocr
{
	extern bool debugEnabled;

	class Font
	{
	private:
		Pix** images_;
		char* symbols_;
		int size_;
		l_uint32 height_;
		int white_;

	public:
		Font(LPCWSTR path, std::vector<LPCWSTR> files, char* symbols, int size, int white);
		~Font();

		l_uint32 getHeight() { return height_; };
		int getSize() { return size_; };
		int getWhite() { return white_; };
		char getSymbol(int index) { return symbols_[index]; };
		Pix* getImage(int index) { return images_[index]; };
	};

	class Engine
	{
	private:
		std::vector<Font*> fontList_;
		TCHAR fontPath_[MAX_PATH];

		int getSymbol(Pix *image, int x, int* maxPercent, int* maxSymbolSize, Font* font);

	public:
		Engine(LPCWSTR fontPath);
		~Engine();

		void clear();
		int addFont(LPCWSTR subPath, std::vector<LPCWSTR> files, char* symbols, int size, int white);
		int getText(Pix* image, int x, int y, int w, int fontIndex, char* text, int textSize);
		int getFontHeight(int index) { return fontList_[index]->getHeight(); }
	};
}
