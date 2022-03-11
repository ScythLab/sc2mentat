#pragma once
#include "tools.h"
#include "pix.h"
#include <windows.h>
#include <string>
#include <vector>
#include <d3d9.h>
#include <d3dx9.h>

namespace dx
{
	#define DEFAULT_FONT_SIZE 18

	struct Vertex2DTex
	{
		float x, y, z, rwh;
		float tx, ty;
		//D3DXVECTOR3 position;
		//float rwh;
		//D3DXVECTOR2 textureCoord;
	};

	struct Vertex2D
	{
		float x, y, z, rhw;
		unsigned long color;
	};

	class Render
	{
	private:
		struct FontInf
		{
			ID3DXFont* Font;
			int Size;

			FontInf(ID3DXFont* font, int size)
			{
				Font = font;
				Size = size;
			}
			~FontInf()
			{
				if (Font)
					Font->Release();
			}
		};

	public:
		Render() { }
		Render(IDirect3DDevice9* device);
		~Render();

		void beginRendering();
		void endRendering();

		void addFont(int size);
		void clearFonts();
		void drawLine(int x0, int y0, int x1, int y1, unsigned long color);
		void drawRect(int x, int y, int w, int h, unsigned long color);
		void drawFilledRect(int x, int y, int w, int h, unsigned long color);
		void drawOutlinedRect(int x, int y, int w, int h, unsigned long color);
		void drawText(LPCSTR text, int fontSize, int x, int y, unsigned long color, bool center = true, int outline = 0);
		void drawPix(PIX* pix, int x, int y, int zoom, int borderWidth);

		POINT getTextDimensions(LPCSTR text, int fontSize);
		POINT getTextDimensions(LPCSTR text, ID3DXFont* font);
		ID3DXFont* getFont(int size);

	private:
		IDirect3DDevice9* m_device = nullptr;
		LPD3DXSPRITE m_sprite = nullptr;
		ID3DXFont* m_deaultFont = nullptr;
		ID3DXLine* m_line = nullptr;
		std::vector<FontInf*> m_fontList;

		ID3DXFont* createFont(int size);
	};

	// Окно-оверлей.
	// WARNING: подходит только для игры в оконном режиме.
	// Для полноэкранного режима необходим перехват функций DirectX, т.е. подгрузка своей dll в процесс игры,
	// что потенциально может быть воспринято как чит и чревато баном.
	// Возможно, есть готовые библиотеки для этого (или использовать библиотеки из известных программ - Fraps, Discord).
	class Overlay
	{
	private:
		struct wnd_rect_t : public RECT
		{
			int width() { return right - left; }
			int height() { return bottom - top; }
		};

	public:
		Overlay(LPWSTR targetClass, LPWSTR targetWindow, bool topmost = true);
		~Overlay();

		static bool existsGame(LPCWSTR className, LPCWSTR windowName);

		Render* createRender();
		HWND getOverlayWnd();
		POINT getResolution() { return m_gameSize; }
		HWND getTargetWnd() { return m_targetWnd; }
		bool getGameResized() { return m_gameResized; }
		void setGameResized(bool value) { m_gameResized = value; }

	private:
		void createOverlay();
		void initDX9();

		void* m_trampoline = NULL; // Трамплин для метода m_wnd_proc
		POINT m_gameSize = { 0 };
		bool m_gameResized;
		ATOM m_hClass = NULL; // "Идентификатор" класса нашего основного окна

		bool m_topmost;

		HWND m_overlayWnd, m_targetWnd;
		wnd_rect_t m_overlayWndSize, m_targetWndSize;

		IDirect3D9* m_d3d = nullptr;
		IDirect3DDevice9* m_device = nullptr;

		LRESULT __thiscall wndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
	};

	PIX* getScreenShot(HWND hWnd);
}
