#include "dx.h"
#include "lang.h"
#include <iostream>
#include <dwmapi.h>

#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")
#pragma comment(lib, "dwmapi.lib")

#define WINDOW_CLASS_NAME L"dx.overlay"
#define COLOR_3D_RED   0xFFFF0000
#define COLOR_3D_ALPHA 0xFF000000

namespace dx
{
	//------------------------------------------------------------------------//
	//------------------------------- Renderer -------------------------------//
	//------------------------------------------------------------------------//

	Render::Render(IDirect3DDevice9* device)
	{
		if (!device)
			throw std::invalid_argument(lng::string(lng::LS_OVERLAY_DEVICE_NULL));

		m_device = device;

		if (FAILED(D3DXCreateLine(m_device, &m_line)))
			throw std::exception(lng::string(lng::LS_OVERLAY_LINE_CREATE_FAIL));

		// Шрифт по умолчанию
		m_deaultFont = createFont(DEFAULT_FONT_SIZE);

		if (FAILED(D3DXCreateSprite(m_device, &m_sprite)))
			throw std::exception(lng::string(lng::LS_OVERLAY_SPRITE_CREATE_FAIL));
	}

	Render::~Render()
	{
		clearFonts();
		m_deaultFont->Release();
		if (m_line)
			m_line->Release();
		if (m_sprite)
			m_sprite->Release();
	}

	ID3DXFont* Render::createFont(int size)
	{
		ID3DXFont* font;
		// Лучше использовать моноширные шрифты для удобного формирования таблиц: Courier New, Consolas или Lucida Console.
		if (FAILED(D3DXCreateFont(m_device, size, NULL, FW_HEAVY, 1, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Courier New", &font)))
			throw std::exception(lng::string(lng::LS_OVERLAY_FONT_CREATE_FAIL));

		return font;
	}

	void Render::addFont(int size)
	{
		ID3DXFont* font = createFont(size);
		m_fontList.push_back(new FontInf(font, size));
	}

	void Render::clearFonts()
	{
		SafeVectorDelete(m_fontList);
	}

	void Render::beginRendering()
	{
		m_device->Clear(NULL, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.f, 0);
		m_device->BeginScene();
	}

	void Render::endRendering()
	{
		m_device->EndScene();
		m_device->Present(NULL, NULL, NULL, NULL);
	}

	void Render::drawLine(int x0, int y0, int x1, int y1, unsigned long color)
	{
		D3DXVECTOR2 lines[2] = {
			D3DXVECTOR2(x0, y0),
			D3DXVECTOR2(x1, y1)
		};

		m_line->Begin();
		m_line->Draw(lines, 2, color);
		m_line->End();
	}

	void Render::drawRect(int x, int y, int w, int h, unsigned long color)
	{
		drawLine(x, y, x + w, y, color);
		drawLine(x, y, x, y + h, color);
		drawLine(x + w, y, x + w, y + h, color);
		drawLine(x, y + h, x + w + 1, y + h, color);
	}

	void Render::drawFilledRect(int x, int y, int w, int h, unsigned long color)
	{
		Vertex2D qV[4] = {
			{ float(x),     float(y + h), 0.f, 1.f, color },
			{ float(x),     float(y),     0.f, 1.f, color },
			{ float(x + w), float(y + h), 0.f, 1.f, color },
			{ float(x + w), float(y) ,    0.f, 1.f, color }
		};

		m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
		m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		m_device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
		m_device->SetTexture(0, NULL);
		m_device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, qV, sizeof(Vertex2D));
	}

	void Render::drawOutlinedRect(int x, int y, int w, int h, unsigned long color)
	{
		drawRect(x - 1, y - 1, w + 2, h + 2, COLOR_3D_ALPHA);
		drawRect(x + 1, y + 1, w - 2, h - 2, COLOR_3D_ALPHA);
		drawRect(x, y, w, h, color);
	}

	ID3DXFont* Render::getFont(int size)
	{
		if (!m_fontList.size())
			return m_deaultFont;

		for (size_t i = 0; i < m_fontList.size(); i++)
		{
			if (m_fontList[i]->Size == size)
				return m_fontList[i]->Font;
		}

		return m_deaultFont;
	}

	void Render::drawPix(PIX* pix, int x, int y, int zoom, int borderWidth)
	{
		l_uint8* buff = NULL;
		int size = pixWrite(pix, IFF_BMP, &buff);
		if (!size || !buff)
			return;

		IDirect3DTexture9 *texture = NULL;
		if (SUCCEEDED(D3DXCreateTextureFromFileInMemory(m_device, buff, size, &texture)))
		{
			Vertex2DTex qV[4] = {
				{ 0.0f,                 0.0f,                  0.f,   1.f,   0.0f, 0.0f },
				{ float(pix->w * zoom), 0.0f,                  0.f,   1.f,   1.0f, 0.0f },
				{ 0.0f,                 float(pix->h * zoom),  0.f,   1.f,   0.0f, 1.0f },
				{ float(pix->w * zoom), float(pix->h * zoom) , 0.f,   1.f,   1.0f, 1.0f }
			};

			for (int i = 0; i < 4; i++)
			{
				Vertex2DTex* v = qV + i;
				v->x += (x + borderWidth);
				v->y += (y + borderWidth);
			}

			drawFilledRect(x, y, pix->w * zoom + borderWidth * 2, pix->h * zoom + borderWidth * 2, COLOR_3D_RED);

			m_device->SetTexture(0, texture);
			m_device->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
			m_device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, qV, sizeof(Vertex2DTex));
			
			texture->Release();
		}

		free(buff);
	}

	void Render::drawText(LPCSTR text, int fontSize, int x, int y, unsigned long color, bool center, int outline)
	{
		ID3DXFont* font = getFont(fontSize);
		if (!font)
			return;

		// Добавим альфа канал.
		color |= COLOR_3D_ALPHA;

		if (center)
		{
			POINT size = getTextDimensions(text, font);
			x -= (size.x) / 2;
		}

		auto _text = [&](LPCSTR _text, int _x, int _y, unsigned long _color)
		{
			RECT r{ _x, _y, _x, _y };
			font->DrawTextA(NULL, _text, -1, &r, DT_NOCLIP, _color);
		};

		//m_sprite->Begin(D3DXSPRITE_ALPHABLEND);

		if (outline > 0)
		{
			// BUG: Топорное решение.
			for (int xx = -outline; xx <= outline; xx++)
			{
				for (int yy = -outline; yy <= outline; yy++)
				{
					_text(text, x + xx, y + yy, COLOR_3D_ALPHA);
				}
			}
		}

		_text(text, x, y, color);

		//D3DXMATRIX matrixScale, matrixTranslate;
		//D3DXMatrixTranslation(&matrixTranslate, 10, 10, 0);
		//D3DXMatrixScaling(&matrixScale, 2, 2, 1);
		//D3DXMATRIX matrix = matrixTranslate * matrixScale;
		//m_sprite->SetTransform(&matrix);
		//m_sprite->End();
		//m_sprite->Draw();
	}

	POINT Render::getTextDimensions(LPCSTR text, int fontSize)
	{
		POINT size = { 0 };
		ID3DXFont* font = getFont(fontSize);
		if (!font)
			return size;

		return getTextDimensions(text, font);
	}

	POINT Render::getTextDimensions(LPCSTR text, ID3DXFont* font)
	{
		RECT r;
		font->DrawTextA(NULL, text, -1, &r, DT_CALCRECT, 0xFFFFFFFF);
		//Point size
		return { r.right - r.left, r.top - r.bottom };
	}

	//------------------------------------------------------------------------//
	//------------------------------- Overlay --------------------------------//
	//------------------------------------------------------------------------//

	Overlay::Overlay(LPWSTR targetClass, LPWSTR targetWindow, bool topmost)
	{
		m_topmost = topmost;

		if ((!targetWindow || !*targetWindow) && (!targetClass || !*targetClass))
			throw std::invalid_argument(lng::string(lng::LS_OVERLAY_TARGETWND_EMPTY));

		m_targetWnd = FindWindowW(targetClass, targetWindow);
		if (!m_targetWnd)
			throw std::invalid_argument(std::format(lng::string(lng::LS_OVERLAY_FINDWND_FAIL), targetWindow, targetClass));

		if (IsIconic(m_targetWnd))
		{
			// Если окно свернуто, то GetWindowRect в left и top вернет -32000, и не определит размер окна,
			// так что сделаем наше окно во весь экран.
			m_targetWndSize.top = 0;
			m_targetWndSize.bottom = GetSystemMetrics(SM_CYSCREEN);
			m_targetWndSize.left = 0;
			m_targetWndSize.right = GetSystemMetrics(SM_CXSCREEN);
			m_gameSize.x = 0;
			m_gameSize.y = 0;
		}
		else
		{
			GetClientRect(m_targetWnd, &m_targetWndSize);
			m_gameSize.x = m_targetWndSize.width();
			m_gameSize.y = m_targetWndSize.height();
		}
		m_gameResized = false;

		void* funcAddr;
		LABEL_ADDR(funcAddr, wndProc);
		m_trampoline = makeTrampoline(this, funcAddr);

		createOverlay();
	}

	Overlay::~Overlay()
	{
		if (m_overlayWnd)
		{
			DestroyWindow(m_overlayWnd);
			m_overlayWnd = NULL;
		}

		if (m_hClass)
		{
			UnregisterClass(WINDOW_CLASS_NAME, NULL);
			m_hClass = NULL;
		}

		if (m_d3d)
			m_d3d->Release();

		if (m_device)
			m_device->Release();

		freeTrompoline(&m_trampoline);
	}

	bool Overlay::existsGame(LPCWSTR className, LPCWSTR windowName)
	{
		return (NULL != FindWindowW(className, windowName));
	}

	//As this creates a new object, be careful where you call this.
	Render* Overlay::createRender()
	{
		return new Render(m_device);
	}

	HWND Overlay::getOverlayWnd()
	{
		return m_overlayWnd;	//Return our window handle
	}

	void Overlay::createOverlay()
	{
		WNDCLASSEX wc;
		memset(&wc, 0, sizeof(wc));
		wc.cbSize = sizeof(wc);

		//Create our window class
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = (WNDPROC)m_trampoline;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = NULL;
		wc.hIcon = NULL;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = HBRUSH(RGB(0, 0, 0));
		wc.lpszClassName = WINDOW_CLASS_NAME;
		wc.hIconSm = NULL;

		//Register our window class
		m_hClass = RegisterClassEx(&wc);
		if (!m_hClass)
			throw std::exception(lng::string(lng::LS_OVERLAY_CLASS_REGISTER_FAIL));

		//Make the window transparent
		DWORD ex_styles = WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW;

		//Add WS_EX_TOPMOST if we choose
		if (m_topmost)
			ex_styles |= WS_EX_TOPMOST;

		HWND hParent = NULL; // (m_topmost) ? NULL : m_target_wnd;

		// BUG: Можно создавать плавающее окно и постоянно помещать его перед окном с игрой,
		// нормально запустить этот вариант не получилось, поэтому пошел другим путем:
		// - указать для оверлея родительским окном - игровое.
		// Работает хорошо, но если задать hWndParent в CreateWindowEx,
		// то игровое окно подвисает и в течение нескольких секунд после подключения оверлея необходимо кликнуть в игровое окно.
		// Если же в CreateWindowEx указать нулевое hWndParent, а потом  вызвать SetParent, то все работает хорошо.
		// Но SetParent необходимо вызвать между DwmExtendFrameIntoClientArea и SetLayeredWindowAttributes.
		// Это проверялось только на одном компе и вполне возможно на других ОС будут проблемы.

		//Create our window
		m_overlayWnd = CreateWindowEx(ex_styles, WINDOW_CLASS_NAME, L"", WS_POPUP | WS_VISIBLE,
			m_targetWndSize.left, m_targetWndSize.top, m_targetWndSize.width(), m_targetWndSize.height(), hParent, NULL, NULL, NULL);
		if (!m_overlayWnd)
			throw std::exception(lng::string(lng::LS_OVERLAY_WND_CREATE_FAIL));

		//Let DWM handle our window
		MARGINS m = { m_targetWndSize.left, m_targetWndSize.top, m_targetWndSize.width(), m_targetWndSize.height() };
		DwmExtendFrameIntoClientArea(m_overlayWnd, &m);

		if (!m_topmost)
			SetParent(m_overlayWnd, m_targetWnd);

		//Set window to use alpha channel
		SetLayeredWindowAttributes(m_overlayWnd, RGB(0, 0, 0), 255, LWA_ALPHA);

		//Show our window
		ShowWindow(m_overlayWnd, SW_SHOW);

		// При переключении с рабочего стола на игру наш оверлей может не получать сообщений (не будет вызова WndProc),
		// и поэтому оверлей не появится над игрой.
		SetTimer(m_overlayWnd, 0, 1000, NULL);

		//Initialize DirectX
		initDX9();
	}

	void Overlay::initDX9()
	{
		//Create DirectX object
		m_d3d = Direct3DCreate9(D3D_SDK_VERSION);

		if (!m_d3d)
			throw std::exception(lng::string(lng::LS_OVERLAY_OBJECT_CREATE_FAIL));

		//Create DirectX present parameters struct
		D3DPRESENT_PARAMETERS d3d_pp;
		ZeroMemory(&d3d_pp, sizeof(d3d_pp));

		//Set our device parameters
		d3d_pp.Windowed = true;
		d3d_pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		d3d_pp.BackBufferFormat = D3DFMT_A8R8G8B8;
		d3d_pp.BackBufferWidth = m_targetWndSize.width();
		d3d_pp.BackBufferHeight = m_targetWndSize.height();
		d3d_pp.hDeviceWindow = m_overlayWnd;
		d3d_pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

		//Create DirectX device
		if (FAILED(m_d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_overlayWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3d_pp, &m_device)))
		{
			m_d3d->Release();
			throw std::exception(lng::string(lng::LS_OVERLAY_DEVICE_CREATE_FAIL));
		}
	}

	LRESULT __thiscall Overlay::wndProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		// Проверим изменение размера игрового окна.
		if (!IsIconic(m_targetWnd))
		{
			wnd_rect_t r;
			GetClientRect(m_targetWnd, &r);
			if (r.width() != m_gameSize.x || r.height() != m_gameSize.y)
			{
				m_gameSize.x = r.width();
				m_gameSize.y = r.height();
				m_gameResized = true;
			}
		}

		//if (WM_DISPLAYCHANGE == msg)
		//{
		//	log_dbg_printf("WM_DISPLAYCHANGE: %dx%dx%d", LOWORD(lparam), HIWORD(lparam), wparam);
		//}

		return DefWindowProc(wnd, msg, wparam, lparam);
	}

	//------------------------------------------------------------------------//
	//------------------------------- Functions ------------------------------//
	//------------------------------------------------------------------------//

	PIX* getScreenShot(HWND hWnd)
	{
		PIX *pix;

		// Определение контекстов
		HDC wndDC = GetDC(hWnd);
		HDC memoryDC = CreateCompatibleDC(wndDC);

		// Фиксация размеров экрана
		// TODO: Необходимо использовать размер окна.
		int screenWidth = GetSystemMetrics(SM_CXSCREEN);
		int screenHeight = GetSystemMetrics(SM_CYSCREEN);

		int bytesPerPixel = 4;
		int bitCount = bytesPerPixel * 8;
		DWORD screenshotSize = screenWidth * screenHeight * bytesPerPixel; // Ширина * Высота * Количество_цветов_на_пиксель

		// Создание и частичное заполнение структуры формата
		BITMAPINFO bmi;
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = screenWidth;
		bmi.bmiHeader.biHeight = -screenHeight; // Отрицательное значение высоты, чтобы изображение не было перевёрнутым
		bmi.bmiHeader.biSizeImage = screenshotSize; // Ширина * Высота * Количество_цветов_на_пиксель
		bmi.bmiHeader.biCompression = BI_RGB;
		bmi.bmiHeader.biBitCount = bitCount;
		bmi.bmiHeader.biPlanes = 1;

		unsigned char *imageBuffer; // Указатель на блок данных BGR, управляемый HBITMAP (да, именно BGR - не RGB)
		HBITMAP hBitmap = CreateDIBSection(wndDC, &bmi, DIB_RGB_COLORS, (void**)&imageBuffer, 0, 0);
		HGDIOBJ oldBitmapHandle = SelectObject(memoryDC, hBitmap);
		BitBlt(memoryDC, 0, 0, screenWidth, screenHeight, wndDC, 0, 0, SRCCOPY);
		SelectObject(memoryDC, oldBitmapHandle);

		// Контексты больше не нужны
		DeleteDC(memoryDC);
		ReleaseDC(hWnd, wndDC);

		// Установим непрозрачность
		for (int i = 0; i < screenshotSize; i += 4)
		{
			imageBuffer[i + 3] = 255;
		}

		pix = pixCreate(screenWidth, screenHeight, bitCount);
		pix->xres = 72;
		pix->yres = 72;
		pix->informat = IFF_BMP;
		memcpy(pix->data, imageBuffer, screenshotSize);
		pixEndianByteSwap(pix);

		//saveBmp("screen.bmp", ScreenWidth, ScreenHeight, bytesPerPixel, ImageBuffer);

		// imageBuffer нам больше не нужен, так что можем освободить битмап
		DeleteObject(hBitmap);

		return pix;
	}

}
