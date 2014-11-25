//clipssはWindows用画面キャプチャソフトウエアです
//詳細は添付ドキュメントをご覧ください
//
//Copyright (C) 2014, guardiancrow

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <stdlib.h>	//_splitpath
#include <rpc.h>	//uuid

#include <iostream>
#include <fstream>
#include <string>

#include "strutil.hpp"
#include "pngutil.h"
#include "jpegutil.h"

using namespace std;

//EntryPoints
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK LayeredWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK EnumWindowsWindowRectProc(HWND hWnd, LPARAM lParam);
BOOL CALLBACK EnumWindowsClientRectProc(HWND hWnd, LPARAM lParam);

ATOM RegisterMainWindow(HINSTANCE hInstance);
ATOM RegisterLayeredWindow(HINSTANCE hInstance);
BOOL ShowRubberBand(HDC hdc, LPRECT lprect, BOOL Erase);
BOOL SaveBitmap(const char *filename, BITMAPINFO bmpinfo, LPBYTE bm);
BOOL CaptureRect(HDC hdc, LPRECT lprect, LPCTSTR filename);
BOOL BuildSaveFileName(char *filename, unsigned int uBufSize);

BOOL GetTopWindowRect(LPRECT lprect, WORD x, WORD y);
BOOL GetTopClientRect(LPRECT lprect, WORD x, WORD y);
BOOL SetTopWindowPos(WORD x, WORD y);

BOOL LoadProfiles(void);
BOOL SaveProfiles(void);

void GetProcessDirectory(char *pDir, unsigned int uBufSize);

//Global Params
//iniに読み書きする設定用

///一時ファイル用ディレクトリ
TCHAR szTempDir[MAX_PATH+1];

///画像保存用ディレクトリ
TCHAR szSaveDir[MAX_PATH+1];

///保存する画像形式用
int SaveFormat;

///PNGで保存する時に最適な圧縮を探すかどうか
BOOL PngOptimize;

///JPEGの品質
int nJpegQuality;

///キャプチャする時にウインドウを前面に移動するかどうか
BOOL PullWindow;

///ファイル接尾語の設定用
int PostfixMode;

///使用するフォント名
TCHAR szFontName[MAX_PATH+1];

///使用するフォントのサイズ
int nFontSize;

///文字などの描画色
COLORREF ForegroundColor;

///背景色
COLORREF BackgroundColor;

///影の色
COLORREF ShadowColor;

///前回の連番数
UINT uLastNumber;

///ICMModeの有効無効フラグ
BOOL ICMMode;

//ここまで設定用　以下iniに書き込まないグローバル変数

///メインウインドウのクラス名
LPCTSTR szMainWindowClass = _T("clipss");

///レイヤーウインドウのクラス名
LPCTSTR szLayeredWindowClass = _T("clipssLay");

///アプリケーション名
LPCTSTR szAppName = _T("CLIPSS");

///仮想スクリーンのオフセット座標値を保持するグローバル変数
int nOffSetX;
///仮想スクリーンのオフセット座標値を保持するグローバル変数
int nOffSetY;

///メインウインドウハンドルのグローバル変数
HWND hMainWnd;
///レイヤーウインドウハンドルのグローバル変数
HWND hLayeredWnd;
///領域選択のためのウインドウハンドル：グローバル変数
HWND hWndRect;


///各クリップモードの定義です
enum CLIPSS_MODE{
	RECTMODE=0,
	WINDOWMODE,
	CLIENTMODE,
	FULLSCREENMODE,
	DISABLEMODE
};
static CLIPSS_MODE SelectMode = DISABLEMODE;

///保存画像形式の定義です
enum IMAGEFORMAT{
	IMAGE_BMP=0,
	IMAGE_JPEG,
	IMAGE_PNG
};

///ファイル接尾語モードの定義です
enum POSTFIX_MODE{
	///無し(上書き)
	POSTFIX_NONE=0,
	///連番
	POSTFIX_CONSECUTIVENUMBER,
	///日付時刻
	POSTFIX_DATETIME,
	///ハッシュ
	POSTFIX_HASH
};


//Macros

/// HWNDがオーナーウインドウかどうかを検査します
#define IsWindowOwner(h) (GetWindow(h,GW_OWNER) == NULL)
#ifdef __MINGW32__
/// MinGWではSecureZeroMemoryは定義されていません
#define SecureZeroMemory(p,s) RtlFillMemory((p),(s),0)
#endif


///RECTを検査し、上下・左右の大小を揃えます
///
///(上＜下)(左＜右)
///
///@param lprect 検査するRECT型で検査後大小が整った値になります
inline void ValidateRect(LPRECT lprect)
{
	int i;

	if(!lprect){
		return;
	}

	if(lprect->right < lprect->left){
		i = lprect->left;
		lprect->left = lprect->right;
		lprect->right = i;
	}
	if(lprect->bottom < lprect->top){
		i = lprect->top;
		lprect->top = lprect->bottom;
		lprect->bottom = i;
	}
}

///メインウインドウを登録します
///@param hInstance アプリケーションのHINSTANCE 
ATOM RegisterMainWindow(HINSTANCE hInstance)
{
	WNDCLASS wc;

	wc.style = 0;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = (HICON)LoadImage(NULL, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
	wc.hCursor = LoadCursor(NULL, IDC_CROSS);
	wc.hbrBackground = 0;
	wc.lpszMenuName = 0;
	wc.lpszClassName = szMainWindowClass;

	return RegisterClass(&wc);
}

///レイヤーウインドウを登録します
///@param hInstance アプリケーションのHINSTANCE
ATOM RegisterLayeredWindow(HINSTANCE hInstance)
{
	WNDCLASS wc;

	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = LayeredWndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = (HICON)LoadImage(NULL, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
	wc.hCursor       = LoadCursor(NULL, IDC_CROSS);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName  = 0;
	wc.lpszClassName = szLayeredWindowClass;

	return RegisterClass(&wc);
}

///泣く子も黙るWinMainです
int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPTSTR lpCmdLine,
	int nCmdShow)
{
	MSG msg;
	int x, y, w, h;
	
	nOffSetX = nOffSetY = 0;
	hMainWnd = hLayeredWnd = NULL;
	SecureZeroMemory(szSaveDir, MAX_PATH);
	SecureZeroMemory(szTempDir, MAX_PATH);
	SecureZeroMemory(szFontName, MAX_PATH);

	//iniから設定を読み込みます
	LoadProfiles();

	RegisterMainWindow(hInstance);
	RegisterLayeredWindow(hInstance);

	//デスクトップを測定
	x = GetSystemMetrics(SM_XVIRTUALSCREEN);
	y = GetSystemMetrics(SM_YVIRTUALSCREEN);
	w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	h = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	
	nOffSetX = x;
	nOffSetY = y;

	/// メインウインドウは透明です
	/// 透明ウインドウは古いWindowsではサポートされていないので
	/// 多分XPぐらいからでしか動かないと思います
	hMainWnd = CreateWindowEx(
		WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
		szMainWindowClass, NULL, WS_POPUP,
		0, 0, 0, 0,
		NULL, NULL, hInstance, NULL);
	if(!hMainWnd)
	{
		return FALSE;
	}
	MoveWindow(hMainWnd, x, y, w, h, FALSE);
	ShowWindow(hMainWnd, SW_SHOW);
	UpdateWindow(hMainWnd);

	// メインウインドウが透明であるためキーボードの情報はWM_TIMERで受け取ります
	SetTimer(hMainWnd, 1, 100, NULL);
	
	/// レイヤーウインドウはレイヤー属性を与え、メインウインドウと関連づけます
	/// こちらもXPぐらい？からの機能だと記憶しています
	hLayeredWnd = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_NOACTIVATE,
		szLayeredWindowClass, NULL, WS_POPUP,
		0, 0, 0, 0,
		hMainWnd, NULL, hInstance, NULL);
	if(!hLayeredWnd)
	{
		DestroyWindow(hMainWnd);
		return FALSE;
	}
	SetLayeredWindowAttributes(hLayeredWnd, RGB(255, 0, 0), 128, LWA_COLORKEY | LWA_ALPHA);

	//メッセージループ
	while(GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	
	return (int)msg.wParam;
}


///泣く子がそろそろ寝て起きる頃なWndProcです
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static RECT clipRect = {0,0,0,0};
	HDC hdc;
	CLIPSS_MODE PrevMode;
	POINT pt = {0,0};
	char *lpszfilename;

	switch(msg){
	case WM_MOUSEMOVE:
		if(SelectMode == WINDOWMODE){
			GetTopWindowRect(&clipRect, LOWORD(lParam), HIWORD(lParam));
		}else if(SelectMode == CLIENTMODE){
			GetTopClientRect(&clipRect, LOWORD(lParam), HIWORD(lParam));
		}else if(SelectMode == RECTMODE){
			clipRect.right = LOWORD(lParam) + nOffSetX;
			clipRect.bottom = HIWORD(lParam) + nOffSetY;
		}
		hdc = GetDC(NULL);
		ShowRubberBand(hdc, &clipRect, FALSE);
		ReleaseDC(NULL, hdc);
		break;
	case WM_LBUTTONDOWN:
		SelectMode = RECTMODE;
		clipRect.left = LOWORD(lParam) + nOffSetX;
		clipRect.top = HIWORD(lParam) + nOffSetY;
		SetCapture(hWnd);
		break;
	case WM_LBUTTONUP:
		if(SelectMode != RECTMODE){
			break;
		}
		//選択矩形保存の際はここに処理が来ます
		clipRect.right = LOWORD(lParam) + nOffSetX;
		clipRect.bottom = HIWORD(lParam) + nOffSetY;
		SelectMode = DISABLEMODE;
		ReleaseCapture();
		hdc = GetDC(NULL);

		//ラバーバンド消去
		ShowRubberBand(hdc, &clipRect, TRUE);
		ShowWindow(hWnd, SW_HIDE);

		//キャプチャ書き出し後、終了
		lpszfilename = new char[MAX_PATH+1];
		BuildSaveFileName(lpszfilename, MAX_PATH);
		CaptureRect(hdc, &clipRect, lpszfilename);
		delete [] lpszfilename;
		ReleaseDC(NULL, hdc);
		SaveProfiles();
		DestroyWindow(hWnd);
		break;
	case WM_TIMER:
		PrevMode = SelectMode;
		//ESCAPEで終了
		if(GetKeyState(VK_ESCAPE) & 0x8000){
			SaveProfiles();
			DestroyWindow(hWnd);
		//左Ctrlでクライアントモード
		}else if(GetKeyState(VK_LCONTROL) & 0x8000){
			SelectMode = CLIENTMODE;
			GetCursorPos(&pt);
			GetTopClientRect(&clipRect, (WORD)pt.x, (WORD)pt.y);
		//左Shiftでウインドウモード
		}else if(GetKeyState(VK_LSHIFT) & 0x8000){
			SelectMode = WINDOWMODE;
			GetCursorPos(&pt);
			GetTopWindowRect(&clipRect, (WORD)pt.x, (WORD)pt.y);
		//左Altでフルスクリーンモード
		}else if(GetKeyState(VK_LMENU) & 0x8000){
			SelectMode = FULLSCREENMODE;
			clipRect.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
			clipRect.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
			clipRect.right = GetSystemMetrics(SM_CXVIRTUALSCREEN) - 1;
			clipRect.bottom = GetSystemMetrics(SM_CYVIRTUALSCREEN) - 1;
		//右Ctrl 右Shft 右Altでモードキャンセル
		}else if(GetKeyState(VK_RCONTROL) & 0x8000){
			SelectMode = DISABLEMODE;
			clipRect.left = clipRect.top = clipRect.right = clipRect.bottom = 0;
		}else if(GetKeyState(VK_RSHIFT) & 0x8000){
			SelectMode = DISABLEMODE;
			clipRect.left = clipRect.top = clipRect.right = clipRect.bottom = 0;
		}else if(GetKeyState(VK_RMENU) & 0x8000){
			SelectMode = DISABLEMODE;
			clipRect.left = clipRect.top = clipRect.right = clipRect.bottom = 0;
		}
		if(SelectMode != PrevMode){
			hdc = GetDC(NULL);
			ShowRubberBand(hdc, &clipRect, FALSE);
			ReleaseDC(NULL, hdc);
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

///レイヤーウインドウのコールバック関数です
///ほぼラバーバンドに合わせた大きさですのでその領域内での動作を制御します
LRESULT CALLBACK LayeredWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	RECT clipRect = {0, 0, 0, 0};
	HBRUSH hBrush;
	HPEN hPen;
	HFONT hFont;
	char szWidth[MAX_PATH], szHeight[MAX_PATH], szMode[MAX_PATH];
	SIZE sizeWidth = {0,0}, sizeHeight={0,0};
	char *lpszfilename;
	int nFontHeight;
	int nWidth, nHeight;
	POINT pt;

	switch (msg)
	{
	case WM_LBUTTONUP:
		//WINDOWMODEとCLIENTMODEはレイヤーウインドウのMoveWindowにより
		//ここにメッセージを受け取りますのでここでキャプチャします
		pt.x = LOWORD(lParam);
		pt.y = HIWORD(lParam);
		ClientToScreen(hWnd, &pt);
		if(SelectMode == WINDOWMODE){
			GetTopWindowRect(&clipRect, (WORD)pt.x, (WORD)pt.y);
		}else if(SelectMode == CLIENTMODE){
			GetTopClientRect(&clipRect, (WORD)pt.x, (WORD)pt.y);
		}else if(SelectMode == FULLSCREENMODE){
			clipRect.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
			clipRect.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
			clipRect.right = GetSystemMetrics(SM_CXVIRTUALSCREEN) - 1;
			clipRect.bottom = GetSystemMetrics(SM_CYVIRTUALSCREEN) - 1;
		}

		//PullWindow判定
		if(PullWindow && !(SelectMode == FULLSCREENMODE)){
			SetTopWindowPos((WORD)pt.x, (WORD)pt.y);
		}

		//キャプチャと書き出し、後終了
		hdc = GetDC(NULL);
		ShowRubberBand(hdc, &clipRect, TRUE);
		ShowWindow(hMainWnd, SW_HIDE);
		Sleep(100);
		lpszfilename = new char[MAX_PATH+1];
		SecureZeroMemory(lpszfilename, MAX_PATH);
		BuildSaveFileName(lpszfilename, MAX_PATH);
		CaptureRect(hdc, &clipRect, lpszfilename);
		delete [] lpszfilename;
		ReleaseDC(NULL, hdc);
		SaveProfiles();
		PostQuitMessage(0);
		break;
	case WM_ERASEBKGND:
		//ユーザーが操作・選択していることをユーザーに示す矩形はここで描画します
		GetClientRect(hWnd, &clipRect);
		hdc = GetDC(hWnd);
		hBrush = CreateSolidBrush(BackgroundColor);
		SelectObject(hdc, hBrush);
		hPen = CreatePen(PS_DASH, 1, ForegroundColor);
		SelectObject(hdc, hPen);

		//矩形の描画
		Rectangle(hdc, 0, 0, clipRect.right, clipRect.bottom);

		//フォントを作成しサイズなどから描画位置を決定
		nFontHeight = -MulDiv(nFontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
		hFont = CreateFont(nFontHeight, 0, 0, 0,
			FW_REGULAR, FALSE, FALSE, FALSE,
			ANSI_CHARSET,
			OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS,
			PROOF_QUALITY,
			FIXED_PITCH | FF_MODERN,
			szFontName);

		SelectObject(hdc, hFont);
		nWidth  = clipRect.right  - clipRect.left;
		nHeight = clipRect.bottom - clipRect.top;

		//描画情報を用意します
		SecureZeroMemory(szWidth, MAX_PATH);
		SecureZeroMemory(szHeight, MAX_PATH);
		SecureZeroMemory(szMode, MAX_PATH);
		wsprintf(szWidth, "W:%d", nWidth);
		wsprintf(szHeight, "H:%d", nHeight);
		if(SelectMode == RECTMODE){
			wsprintf(szMode, "RECTANGLE MODE");
		}else if(SelectMode == WINDOWMODE){
			wsprintf(szMode, "WINDOW MODE");
		}else if(SelectMode == CLIENTMODE){
			wsprintf(szMode, "CLIENT MODE");
		}else if(SelectMode == FULLSCREENMODE){
			wsprintf(szMode, "FULLSCREEN MODE");
		}

		//サイズ情報の描画位置を決定（モード情報のY座標も）
		GetTextExtentPoint32(hdc, szWidth, strlen(szWidth), &sizeWidth);
		GetTextExtentPoint32(hdc, szHeight, strlen(szHeight), &sizeHeight);
		sizeWidth.cy *= 2;
		sizeWidth.cx += nFontSize;
		sizeWidth.cy += nFontSize;
		sizeHeight.cx += nFontSize;
		sizeHeight.cy += nFontSize;

		//ここから情報を描画
		SetBkMode(hdc, TRANSPARENT);

		//影付け
		SetTextColor(hdc, ShadowColor);
		TextOut(hdc, clipRect.right - sizeWidth.cx + 1, clipRect.bottom - sizeWidth.cy + 1, (LPCTSTR)szWidth, strlen(szWidth));
		TextOut(hdc, clipRect.right - sizeHeight.cx + 1, clipRect.bottom - sizeHeight.cy + 1, (LPCTSTR)szHeight, strlen(szHeight));
		TextOut(hdc, nFontSize + 1, clipRect.bottom - sizeHeight.cy + 1, (LPCTSTR)szMode, strlen(szMode));
		TextOut(hdc, nFontSize + 1, nFontSize + 1, (LPCTSTR)szAppName, strlen(szAppName));

		//本文
		SetTextColor(hdc, ForegroundColor);
		TextOut(hdc, clipRect.right - sizeWidth.cx, clipRect.bottom - sizeWidth.cy, (LPCTSTR)szWidth, strlen(szWidth));
		TextOut(hdc, clipRect.right - sizeHeight.cx, clipRect.bottom - sizeHeight.cy, (LPCTSTR)szHeight, strlen(szHeight));
		TextOut(hdc, nFontSize, clipRect.bottom - sizeHeight.cy, (LPCTSTR)szMode, strlen(szMode));
		TextOut(hdc, nFontSize, nFontSize, (LPCTSTR)szAppName, strlen(szAppName));

		//後始末
		DeleteObject(hPen);
		DeleteObject(hBrush);
		DeleteObject(hFont);
		ReleaseDC(hWnd, hdc);
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

/// デバイスコンテキストから矩形で指定された部分をくり抜きファイルに保存します
/// @param hdc デバイスコンテキスト
/// @param lprect 切り抜く矩形情報へのポインタ
/// @param filename 出力ファイル名
/// @return TRUE:成功 FALSE:失敗
BOOL CaptureRect(HDC hdc, LPRECT lprect, LPCTSTR filename)
{
	HDC bufferDC;
	HBITMAP bufferBitmap;
	BITMAPINFO bmpInfo;
	LPBYTE lpBm;
	int nWidth, nHeight;
	RECT rect = {0,0,0,0};

	if(!hdc || !lprect || !filename){
		return FALSE;
	}

	nWidth = nHeight = 0;
	lpBm = NULL;

	rect = *lprect;
	ValidateRect(&rect);

	nWidth  = rect.right - rect.left + 1;
	nHeight = rect.bottom - rect.top + 1;

	if(nWidth == 0 || nHeight == 0) {
		return FALSE;
	}

	if (ICMMode){
		SetICMMode(hdc, ICM_ON);
	}else{
		SetICMMode(hdc, ICM_OFF);
	}

	bufferDC = CreateCompatibleDC(hdc);
	bufferBitmap = CreateCompatibleBitmap(hdc, nWidth, nHeight);
	SelectObject(bufferDC, bufferBitmap);

	BitBlt(bufferDC, 0, 0, nWidth, nHeight, hdc, rect.left, rect.top, SRCCOPY);

	lpBm = (LPBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nWidth * nHeight * 4);
	SecureZeroMemory(&bmpInfo, sizeof(BITMAPINFO));
	bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmpInfo.bmiHeader.biWidth = nWidth;
	bmpInfo.bmiHeader.biHeight = nHeight;
	bmpInfo.bmiHeader.biPlanes = 1;
	bmpInfo.bmiHeader.biBitCount = 24;//32;
	bmpInfo.bmiHeader.biCompression = BI_RGB;

	GetDIBits(bufferDC, bufferBitmap, 0, nHeight, lpBm, &bmpInfo, DIB_RGB_COLORS);

	switch(SaveFormat){
	case IMAGE_BMP:
		SaveBitmap(filename, bmpInfo, lpBm);
		break;
	case IMAGE_JPEG:
		DIBtoJPEG(filename, &bmpInfo, lpBm, nJpegQuality);
		break;
	case IMAGE_PNG:
		if(PngOptimize){
			DIBtoPNG_Optimize(filename, &bmpInfo, lpBm);
		}else{
			DIBtoPNG(filename, &bmpInfo, lpBm);
		}
		break;
	}

	HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, lpBm);
	DeleteDC(bufferDC);
	DeleteObject(bufferBitmap);

	return TRUE;
}

///設定に基づき保存ファイル名を構成します
BOOL BuildSaveFileName(char *filename, unsigned int uBufSize)
{
	UUID uuid;
	string str, strExt;
	SYSTEMTIME st;
	unsigned char *lpUuidString;

	strExt = "";
	switch(SaveFormat){
	case IMAGE_BMP:
		strExt = "bmp";
		break;
	case IMAGE_JPEG:
		strExt = "jpg";
		break;
	case IMAGE_PNG:
		strExt = "png";
		break;
	}

	str = szSaveDir;
	str += "clipss";

	//sprintfなんてなかった
	if(PostfixMode == POSTFIX_CONSECUTIVENUMBER){
		str += "_";
		char szNumber[5];
		SecureZeroMemory(szNumber, 4);
		wsprintf(szNumber, "%04u", uLastNumber);
		str += szNumber;
		uLastNumber++;
	}else if(PostfixMode == POSTFIX_DATETIME){
		str += "_";
		GetLocalTime(&st);
		char szNumber[15];
		SecureZeroMemory(szNumber, 14);
		wsprintf(szNumber, "%04u%02u%02u%02u%02u%02u", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
		str += szNumber;
	}else if(PostfixMode == POSTFIX_HASH){
		UuidCreate(&uuid);
		UuidToString(&uuid, &lpUuidString);
		str += "_";
		str += (char*)lpUuidString;
		RpcStringFree(&lpUuidString);
	}
	str += ".";
	str += strExt;

	memcpy(filename, str.c_str(), uBufSize);

	return TRUE;
}

///ラバーバンドを表示/非表示します
///実際の処理はラバーバンド表示用であるレイヤーウインドウを表示/非表示します
BOOL ShowRubberBand(HDC hdc, LPRECT lprect, BOOL Erase)
{
	static BOOL FirstTime = TRUE;
	RECT rect  = {0,0,0,0};

	//最初の時はレイヤーウインドウを表示します
	if(FirstTime){
		ShowWindow(hLayeredWnd, SW_SHOW);
		UpdateWindow(hLayeredWnd);
		FirstTime = FALSE;
	}

	//削除はレイヤーウインドウを非表示にすることで行います
	if(Erase){
		ShowWindow(hLayeredWnd, SW_HIDE);
		FirstTime = TRUE;
	}

	rect = *lprect;
	ValidateRect(&rect);

	//レイヤーウインドウのサイズをラバーバンドにあわせます
	MoveWindow(hLayeredWnd,
			rect.left, rect.top,
			rect.right - rect.left + 1,	rect.bottom - rect.top + 1,
			TRUE);

	return TRUE;
}

///DIBをBMP形式で保存します
BOOL SaveBitmap(const char *filename, BITMAPINFO bmpInfo, LPBYTE bm)
{
	if(bmpInfo.bmiHeader.biBitCount != 24 && bmpInfo.bmiHeader.biBitCount != 32){
		return FALSE;
	}

	unsigned short outbpp = 24;
	unsigned long srcrowstride = (bmpInfo.bmiHeader.biWidth * bmpInfo.bmiHeader.biBitCount + 31L) / 32 * 4;
	unsigned long rowstride = (bmpInfo.bmiHeader.biWidth * outbpp + 31L) / 32 * 4;
	unsigned long fileheadersize = 14;
	unsigned long infoheadersize = 40;
	unsigned long headersize = fileheadersize + infoheadersize;
	unsigned long dwClrUsed = 0;
	unsigned short wPlanes = 1;
	unsigned short wReserved = 0;
	unsigned long dwReserved = 0;
	
	unsigned long bmsize = rowstride * (unsigned)bmpInfo.bmiHeader.biHeight;
	unsigned long fullsize = fileheadersize + infoheadersize + bmsize;

	ofstream fos(filename, ios::out | ios::binary);
	if(!fos){
		return FALSE;
	}
	
	//BMPFileHeader
	fos.write("BM", 2);
	fos.write((char*)&fullsize, sizeof(unsigned long));
	fos.write((char*)&wReserved, sizeof(unsigned short));
	fos.write((char*)&wReserved, sizeof(unsigned short));
	fos.write((char*)&headersize, sizeof(unsigned long));

	//BMPInfoHeader
	fos.write((char*)&infoheadersize, sizeof(unsigned long));
	fos.write((char*)&bmpInfo.bmiHeader.biWidth, sizeof(unsigned long));
	fos.write((char*)&bmpInfo.bmiHeader.biHeight, sizeof(unsigned long));
	fos.write((char*)&wPlanes, sizeof(unsigned short));
	fos.write((char*)&outbpp, sizeof(unsigned short));
	fos.write((char*)&dwReserved, sizeof(unsigned long));
	fos.write((char*)&bmsize, sizeof(unsigned long));
	fos.write((char*)&dwReserved, sizeof(unsigned long));
	fos.write((char*)&dwReserved, sizeof(unsigned long));
	fos.write((char*)&dwClrUsed, sizeof(unsigned long));
	fos.write((char*)&dwReserved, sizeof(unsigned long));

	if(bmpInfo.bmiHeader.biBitCount == 32){
		//to 24bpp
		char *destrow = (char*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,rowstride);
		for(unsigned long y=0;y<(unsigned)bmpInfo.bmiHeader.biHeight;y++){
			char *bmCurrent = reinterpret_cast<char*>(bm + srcrowstride * y);
			for(unsigned long x=0;x<(unsigned)bmpInfo.bmiHeader.biWidth;x++){
				destrow[x*3+0] = bmCurrent[x*4+0];
				destrow[x*3+1] = bmCurrent[x*4+1];
				destrow[x*3+2] = bmCurrent[x*4+2];
			}
			fos.write(destrow, rowstride);
		}
		HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, destrow);
	}else{
		fos.write(reinterpret_cast<char*>(bm), rowstride * bmpInfo.bmiHeader.biHeight);
	}

	fos.flush();
	fos.close();
	
	return TRUE;
}

///ウインドウハンドルがタスクに表示されているアプリケーションかどうかを判別します
///ダイアログ判別はしていません
///
///タスクバーに表示されているアプリケーションかどうか、
///またはタスクマネージャーのアプリケーションタブのタスクに表示されている
///アプリケーションかどうかの判別をします
///もっとスマートな実装があるかもしれません
inline BOOL CheckTaskApplication(HWND hWnd)
{
	//long style = GetWindowLong(hWnd, GWL_STYLE);
	long exstyle = GetWindowLong(hWnd, GWL_EXSTYLE);
	if(!IsWindowVisible(hWnd) || !IsWindowOwner(hWnd) || /*IsIconic(hWnd) ||*/ !IsWindowEnabled(hWnd)){
		return FALSE;
	}
	if(exstyle & WS_EX_TRANSPARENT || exstyle & WS_EX_TOOLWINDOW){
		return FALSE;
	}
	return TRUE;
}

///ウインドウ領域を選択するための内部的なコールバック関数です
BOOL CALLBACK EnumWindowsWindowRectProc(HWND hWnd, LPARAM lParam)
{
	RECT rect = {0,0,0,0};
	POINT pt = {0,0};
	//POINT ptScreen = {0,0};
	pt.x = LOWORD(lParam);
	pt.y = HIWORD(lParam);
	//TCHAR szText[MAX_PATH+1];

	if(!CheckTaskApplication(hWnd)){
		return TRUE;
	}
	if(!GetWindowRect(hWnd, &rect)){
		return TRUE;
	}
	if(rect.left == 0 && rect.top == 0 && rect.right == 0 && rect.bottom == 0){
		return TRUE;
	}

	if(PtInRect(&rect, pt)){
		//z-orderの上位から順に走査していくようなので最初のものを見えているウインドウとします
		hWndRect = hWnd;
		return FALSE;
	}
	return TRUE;
}

///クライアント領域を選択するための内部的なコールバック関数です
BOOL CALLBACK EnumWindowsClientRectProc(HWND hWnd, LPARAM lParam)
{
	RECT rect = {0,0,0,0};
	POINT pt = {0,0};
	POINT ptScreen = {0,0};
	pt.x = LOWORD(lParam);
	pt.y = HIWORD(lParam);

	if(!CheckTaskApplication(hWnd)){
		return TRUE;
	}
	if(!GetClientRect(hWnd, &rect)){
		return TRUE;
	}
	if(rect.left == 0 && rect.top == 0 && rect.right == 0 && rect.bottom == 0){
		return TRUE;
	}
	ClientToScreen(hWnd, &ptScreen);
	rect.left += ptScreen.x;
	rect.right += ptScreen.x;
	rect.top += ptScreen.y;
	rect.bottom += ptScreen.y;

	if(PtInRect(&rect, pt)){
		//z-orderの上位から順に走査していくようなので最初のものを見えているウインドウとします
		hWndRect = hWnd;
		return FALSE;
	}
	return TRUE;
}

///(x,y)に見えている「ウインドウの領域」を取得します
BOOL GetTopWindowRect(LPRECT lprect, WORD x, WORD y)
{
	RECT rect = {0,0,0,0};
	LPARAM l = ((DWORD)x | ((DWORD)y << 16));
	hWndRect = NULL;

	EnumWindows(EnumWindowsClientRectProc, l);

	if(hWndRect){
		GetWindowRect(hWndRect, &rect);
	}

	*lprect = rect;
	return (hWndRect)?TRUE:FALSE;
}

///(x,y)に見えているウインドウの「クライアント領域」を取得します
BOOL GetTopClientRect(LPRECT lprect, WORD x, WORD y)
{
	RECT rect = {0,0,0,0};
	LPARAM l = ((DWORD)x | ((DWORD)y << 16));
	hWndRect = NULL;

	EnumWindows(EnumWindowsClientRectProc, l);

	if(hWndRect){
		POINT pt = {0,0};
		ClientToScreen(hWndRect, &pt);
		GetClientRect(hWndRect, &rect);
		rect.left += pt.x;
		rect.right += pt.x;
		rect.top += pt.y;
		rect.bottom += pt.y;
	}

	*lprect = rect;
	return (hWndRect)?TRUE:FALSE;
}

///(x,y)に見えているウインドウを全面に表示します
///
///描画のタイミングで必ずしも前面に移動するとは限らないので呼び出し側でこの後にSleep(100)ぐらい入れた方がいいかもしれません
///@param x X座標
///@param y Y座標
BOOL SetTopWindowPos(WORD x, WORD y)
{
	RECT rect = {0,0,0,0};
	GetTopWindowRect(&rect, x, y);
	if(hWndRect){
		SetWindowPos(hWndRect, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		SetForegroundWindow(hWndRect);
		UpdateWindow(hWndRect);
		return TRUE;
	}
	return FALSE;
}


///アプリケーションのあるディレクトリ文字列を取得します
///
///@param pDir 文字列へのポインタ
///@param uBufSize pDirのバッファサイズ
void GetProcessDirectory(char *pDir, unsigned int uBufSize)
{
	char *lpDir = new char[MAX_PATH+1];
	char szProcessPath[MAX_PATH+1];
	char szDrive[MAX_PATH+1];
	char szDir[MAX_PATH+1];

	SecureZeroMemory(lpDir, MAX_PATH);
	SecureZeroMemory(szProcessPath, MAX_PATH);
	SecureZeroMemory(szDrive, MAX_PATH);
	SecureZeroMemory(szDir, MAX_PATH);
	GetModuleFileName(NULL, szProcessPath, MAX_PATH);

	//PathRemoveFileSpec() はshlwapi.libが要るので使用せず・・・
	_splitpath(szProcessPath, szDrive, szDir, NULL, NULL);
	_makepath(lpDir, szDrive, szDir, NULL, NULL);

	if(uBufSize < MAX_PATH){
		memcpy(pDir, lpDir, uBufSize);
	}else{
		memcpy(pDir, lpDir, MAX_PATH);
	}
	delete [] lpDir;
}


/// アプリケーションの設定を読み込みます
BOOL LoadProfiles(void)
{
	SaveFormat = 0;
	PngOptimize = 0;
	PullWindow = 0;
	char *lpProcessDir = new char[MAX_PATH+1];
	GetProcessDirectory(lpProcessDir, MAX_PATH);
	string strINIFileName = lpProcessDir;
	strINIFileName += "clipss.ini";

	SecureZeroMemory(szSaveDir, MAX_PATH);
	SecureZeroMemory(szTempDir, MAX_PATH);
	SecureZeroMemory(szFontName, MAX_PATH);
	
	GetPrivateProfileString(_T("clipss"), _T("SaveDir"), lpProcessDir, szSaveDir, MAX_PATH, strINIFileName.c_str());
	GetPrivateProfileString(_T("clipss"), _T("TempDir"), lpProcessDir, szTempDir, MAX_PATH, strINIFileName.c_str());
	SaveFormat = GetPrivateProfileInt(_T("clipss"), _T("SaveFormat"), 1, strINIFileName.c_str());
	PngOptimize = GetPrivateProfileInt(_T("clipss"), _T("PngOptimize"), 0, strINIFileName.c_str());
	nJpegQuality = GetPrivateProfileInt(_T("clipss"), _T("JpegQuality"), 75, strINIFileName.c_str());
	PullWindow = GetPrivateProfileInt(_T("clipss"), _T("PullWindow"), 1, strINIFileName.c_str());
	PostfixMode = GetPrivateProfileInt(_T("clipss"), _T("PostfixMode"), 1, strINIFileName.c_str());
	GetPrivateProfileString(_T("clipss"), _T("FontName"), _T("Verdana"), szFontName, MAX_PATH, strINIFileName.c_str());
	nFontSize = GetPrivateProfileInt(_T("clipss"), _T("FontSize"), 10, strINIFileName.c_str());
	ForegroundColor = GetPrivateProfileInt(_T("clipss"), _T("ForegroundColor"), 0xFFFFFF, strINIFileName.c_str());
	BackgroundColor = GetPrivateProfileInt(_T("clipss"), _T("BackgroundColor"), 0xFF3F3F, strINIFileName.c_str());
	ShadowColor = GetPrivateProfileInt(_T("clipss"), _T("ShadowColor"), 0x000000, strINIFileName.c_str());
	uLastNumber = GetPrivateProfileInt(_T("clipss"), _T("LastNumber"), 0, strINIFileName.c_str());
	ICMMode = GetPrivateProfileInt(_T("clipss"), _T("ICMMode"), 1, strINIFileName.c_str());

	delete [] lpProcessDir;

	if(nJpegQuality < 0){
		nJpegQuality = 0;
	}else if(nJpegQuality > 100){
		nJpegQuality = 100;
	}
	if(nFontSize < 5){
		nFontSize = 5;
	}else if(nFontSize > 64){
		nFontSize = 64;
	}
	if(uLastNumber > 9999){
		uLastNumber = 0;
	}

	return TRUE;
}


///アプリケーションの設定を保存します
BOOL SaveProfiles(void)
{
	string strSaveFormat = toString(SaveFormat);
	string strPngOptimize = toString(PngOptimize);
	string strJpegQuality = toString(nJpegQuality);
	string strPullWindow = toString(PullWindow);
	string strPostfixMode = toString(PostfixMode);
	string strFontSize = toString(nFontSize);
	string strForegroundColor = toString(ForegroundColor);
	string strBackgroundColor = toString(BackgroundColor);
	string strShadowColor = toString(ShadowColor);
	string strLastNumber = toString(uLastNumber);
	string strICMMode = toString(ICMMode);

	char *lpProcessDir = new char[MAX_PATH + 1];
	GetProcessDirectory(lpProcessDir, MAX_PATH);
	string strINIFileName = lpProcessDir;
	strINIFileName += "clipss.ini";
	delete[] lpProcessDir;

	WritePrivateProfileString(_T("clipss"), _T("SaveDir"), szSaveDir, strINIFileName.c_str());
	WritePrivateProfileString(_T("clipss"), _T("TempDir"), szTempDir, strINIFileName.c_str());
	WritePrivateProfileString(_T("clipss"), _T("SaveFormat"), strSaveFormat.c_str(), strINIFileName.c_str());
	WritePrivateProfileString(_T("clipss"), _T("PngOptimize"), strPngOptimize.c_str(), strINIFileName.c_str());
	WritePrivateProfileString(_T("clipss"), _T("JpegQuality"), strJpegQuality.c_str(), strINIFileName.c_str());
	WritePrivateProfileString(_T("clipss"), _T("PullWindow"), strPullWindow.c_str(), strINIFileName.c_str());
	WritePrivateProfileString(_T("clipss"), _T("PostfixMode"), strPostfixMode.c_str(), strINIFileName.c_str());
	WritePrivateProfileString(_T("clipss"), _T("FontName"), szFontName, strINIFileName.c_str());
	WritePrivateProfileString(_T("clipss"), _T("FontSize"), strFontSize.c_str(), strINIFileName.c_str());
	WritePrivateProfileString(_T("clipss"), _T("ForegroundColor"), strForegroundColor.c_str(), strINIFileName.c_str());
	WritePrivateProfileString(_T("clipss"), _T("BackgroundColor"), strBackgroundColor.c_str(), strINIFileName.c_str());
	WritePrivateProfileString(_T("clipss"), _T("ShadowColor"), strShadowColor.c_str(), strINIFileName.c_str());
	WritePrivateProfileString(_T("clipss"), _T("LastNumber"), strLastNumber.c_str(), strINIFileName.c_str());
	WritePrivateProfileString(_T("clipss"), _T("ICMMode"), strICMMode.c_str(), strINIFileName.c_str());

	return TRUE;
}

