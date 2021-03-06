//clipssはWindows用画面キャプチャソフトウエアです
//詳細は添付ドキュメントをご覧ください
//BOM付きUTF-8での保存を推奨します
//
//Copyright (C) 2014 - 2016, guardiancrow

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <tchar.h>
//#include <stdlib.h>	//_splitpath
#include <rpc.h>	//uuid
#include <shellapi.h>
//#include <shlwapi.h>
//#include <shlobj.h>

#include <iostream>
#include <fstream>
#include <string>

#include "clipssdef.h"
#include "resource.h"
#include "strutil.hpp"
#include "pngutil.h"
#include "pngrecomp.h"
#include "jpegutil.h"
#include "optiondialog.h"

//#define WM_NOTIFYICON (WM_USER+123)
//#define ID_TASKTRAY 0


using namespace std;

//EntryPoints
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK LayeredWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK TipLayeredWndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK OptionDialogProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK EnumWindowsWindowRectProc(HWND hWnd, LPARAM lParam);
BOOL CALLBACK EnumWindowsClientRectProc(HWND hWnd, LPARAM lParam);

ATOM RegisterMainWindow(HINSTANCE hInstance);
ATOM RegisterLayeredWindow(HINSTANCE hInstance);
ATOM RegisterTipLayeredWindow(HINSTANCE hInstance);

BOOL ShowRubberBand(HDC hdc, LPRECT lprect, BOOL Erase);
BOOL SaveBitmap(const char *filename, BITMAPINFO bmpinfo, LPBYTE bm);
BOOL CaptureRect(HDC hdc, LPRECT lprect, LPCTSTR filename);
BOOL BuildSaveFileName(char *filename, unsigned int uBufSize);

BOOL GetTopWindowRect(LPRECT lprect, WORD x, WORD y);
BOOL GetTopClientRect(LPRECT lprect, WORD x, WORD y);
BOOL SetTopWindowPos(WORD x, WORD y);
BOOL AddNotifyIcon(HWND hWnd, unsigned int uID, HICON hIcon, TCHAR *szTip);
BOOL DeleteNotifyIcon(HWND hWnd, unsigned int uID);
BOOL SwitchIdleMode(void);
BOOL CalcTipWindowSize(HWND hWnd, SIZE *lpSize);
BOOL BuildPopupMenu(HMENU *pHPopup);
BOOL BuildTooltipString(TCHAR** ppszTip);

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

///PNG圧縮でzopfliを使うかどうか
BOOL UseZopfli;

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

///チップウインドウのクラス名
LPCTSTR szTipLayeredWindowClass = _T("clipssTip");

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
///チップウインドウハンドルのグローバル変数
HWND hTipWnd;

///領域選択のためのウインドウハンドル：グローバル変数
HWND hWndRect;

///タイマーのID：グローバル変数
UINT_PTR uIDTimer;

///現在のインスタンス
HINSTANCE hInst;

///現在のキャプチャモード
static CLIPSS_MODE SelectMode = READYMODE;


///RECTを検査し、上下・左右の大小を揃えます
///
///(上＜下)(左＜右)
///
///@param lprect 検査するRECT型で検査後大小が整った値になります
inline void _ValidateRect(LPRECT lprect)
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

	hInst = hInstance;

	//二重起動を抑止します
	//SECURITY_ATTRIBUTESがNULL（他ユーザーの使用は想定しない）
	//ハンドルの閉じ忘れに注意
	HANDLE hMutex = CreateMutex(NULL, TRUE, "clipss_mutex");
	if(hMutex == NULL){
		return FALSE;
	}else if(GetLastError() == ERROR_ALREADY_EXISTS){
		CloseHandle(hMutex);
		return FALSE;
	}
	
	nOffSetX = nOffSetY = 0;
	hMainWnd = hLayeredWnd = hTipWnd = NULL;
	SecureZeroMemory(szSaveDir, MAX_PATH);
	SecureZeroMemory(szTempDir, MAX_PATH);
	SecureZeroMemory(szFontName, MAX_PATH);

	//iniから設定を読み込みます
	LoadProfiles();

	RegisterMainWindow(hInstance);
	RegisterLayeredWindow(hInstance);
	RegisterTipLayeredWindow(hInstance);

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
		CloseHandle(hMutex);
		return FALSE;
	}
	MoveWindow(hMainWnd, x, y, w, h, FALSE);
	ShowWindow(hMainWnd, SW_SHOW);
	UpdateWindow(hMainWnd);

	// メインウインドウが透明であるためキーボードの情報はWM_TIMERで受け取ります
	uIDTimer = 0;
	uIDTimer = SetTimer(hMainWnd, 1, 100, NULL);
	
	/// レイヤーウインドウはレイヤー属性を与え、メインウインドウと関連づけます
	/// こちらもXPぐらい？からの機能だと記憶しています
	hLayeredWnd = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_NOACTIVATE,
		szLayeredWindowClass, NULL, WS_POPUP,
		0, 0, 0, 0,
		hMainWnd, NULL, hInstance, NULL);
	if(!hLayeredWnd)
	{
		DestroyWindow(hMainWnd);
		CloseHandle(hMutex);
		return FALSE;
	}
	SetLayeredWindowAttributes(hLayeredWnd, RGB(255, 0, 0), 128, LWA_COLORKEY | LWA_ALPHA);


	hTipWnd = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_NOACTIVATE,
		szTipLayeredWindowClass, NULL, WS_POPUP,
		0, 0, 0, 0,
		hMainWnd, NULL, hInstance, NULL);
	SetLayeredWindowAttributes(hTipWnd, RGB(0, 255, 0), 128, LWA_COLORKEY | LWA_ALPHA);
	if(!hTipWnd)
	{
		DestroyWindow(hLayeredWnd);
		DestroyWindow(hMainWnd);
		CloseHandle(hMutex);
		return FALSE;
	}
	ShowWindow(hTipWnd, SW_SHOW);
	UpdateWindow(hTipWnd);

	TCHAR* lpszTip = new TCHAR[64];
	BuildTooltipString(&lpszTip);
	AddNotifyIcon(hMainWnd, ID_TASKTRAY,
				(HICON)LoadImage(NULL, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED),
				//(TCHAR*)szAppName);
				lpszTip);
	delete [] lpszTip;

	//メッセージループ
	while(GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	DeleteNotifyIcon(hMainWnd, ID_TASKTRAY);
	CloseHandle(hMutex);
	
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

		//TipWindowの処理
		if(SelectMode == READYMODE){
			SIZE size;
			size.cx = size.cy = 0;
			CalcTipWindowSize(hTipWnd, &size);
			MoveWindow(hTipWnd,
				LOWORD(lParam) + 24, HIWORD(lParam) + 24,
				size.cx, size.cy, TRUE);
			ShowWindow(hTipWnd, SW_SHOW);
		}else{
			MoveWindow(hTipWnd, 0, 0, 0, 0, TRUE);
			ShowWindow(hTipWnd, SW_HIDE);
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
		SelectMode = READYMODE;
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
		//DestroyWindow(hWnd);

		//IDLEMODEに
		SwitchIdleMode();
		break;
	case WM_TIMER:
		PrevMode = SelectMode;
		//ESCAPEで中断
		if(GetKeyState(VK_ESCAPE) & 0x8000){
			SwitchIdleMode();
			UpdateWindow(hWnd);
			//SaveProfiles();
			//DestroyWindow(hWnd);
		//Ctrlでクライアントモード
		}else if(GetKeyState(VK_CONTROL) & 0x8000){
			SelectMode = CLIENTMODE;
			GetCursorPos(&pt);
			GetTopClientRect(&clipRect, (WORD)pt.x, (WORD)pt.y);
		//Shiftでウインドウモード
		}else if(GetKeyState(VK_SHIFT) & 0x8000){
			SelectMode = WINDOWMODE;
			GetCursorPos(&pt);
			GetTopWindowRect(&clipRect, (WORD)pt.x, (WORD)pt.y);
		//Altでフルスクリーンモード
		}else if(GetKeyState(VK_MENU) & 0x8000){
			SelectMode = FULLSCREENMODE;
			clipRect.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
			clipRect.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
			clipRect.right = GetSystemMetrics(SM_CXVIRTUALSCREEN) - 1;
			clipRect.bottom = GetSystemMetrics(SM_CYVIRTUALSCREEN) - 1;
		//SPACEで各モードから矩形選択モードに戻る
		}else if(GetKeyState(VK_SPACE) & 0x8000){
			SelectMode = READYMODE;
			clipRect.left = clipRect.top = clipRect.right = clipRect.bottom = 0;
		}

		if(SelectMode != READYMODE){
			ShowWindow(hTipWnd, SW_HIDE);
		}

		//if(SelectMode != PrevMode){
			hdc = GetDC(NULL);
			ShowRubberBand(hdc, &clipRect, FALSE);
			ReleaseDC(NULL, hdc);
		//}
		break;
	case WM_COMMAND:
        int wmId;
		wmId = LOWORD(wParam);
		PrevMode = SelectMode;
        switch (wmId) {
			case IDM_RECTMODE:
				SelectMode = READYMODE;
				clipRect.left = clipRect.top = clipRect.right = clipRect.bottom = 0;
				ShowWindow(hWnd, SW_SHOW);
				break;
			case IDM_CLIENTMODE:
				SelectMode = CLIENTMODE;
				GetCursorPos(&pt);
				GetTopClientRect(&clipRect, (WORD)pt.x, (WORD)pt.y);
				ShowWindow(hWnd, SW_SHOW);
				break;
			case IDM_WINDOWMODE:
				SelectMode = WINDOWMODE;
				GetCursorPos(&pt);
				GetTopWindowRect(&clipRect, (WORD)pt.x, (WORD)pt.y);
				ShowWindow(hWnd, SW_SHOW);
				break;
			case IDM_FULLSCREENMODE:
				SelectMode = FULLSCREENMODE;
				clipRect.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
				clipRect.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
				clipRect.right = GetSystemMetrics(SM_CXVIRTUALSCREEN) - 1;
				clipRect.bottom = GetSystemMetrics(SM_CYVIRTUALSCREEN) - 1;
				ShowWindow(hWnd, SW_SHOW);
				break;
			case IDM_OPENFOLDER:
				ShellExecute(hWnd, "open", szSaveDir, NULL, NULL, SW_SHOWNORMAL);
				break;
			case IDM_OPTION:
				DialogBox(hInst, MAKEINTRESOURCE(IDD_OPTIONDIALOG), hWnd, OptionDialogProc);
				break;
			case IDM_ABOUT:
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, msg, wParam, lParam);
        }
		if(SelectMode != PrevMode){
			if(uIDTimer == 0){
				uIDTimer = SetTimer(hWnd, 1, 100, NULL);
			}
			hdc = GetDC(NULL);
			ShowRubberBand(hdc, &clipRect, TRUE);
			ReleaseDC(NULL, hdc);
		}
        break;
	case WM_NOTIFYICON:
		if (wParam == ID_TASKTRAY) {
			if (lParam == WM_RBUTTONUP) {
				POINT pt={0};
				GetCursorPos(&pt);

				HMENU hPopup = NULL;
				if(!BuildPopupMenu(&hPopup) || hPopup == NULL){
					break;
				}
				SetForegroundWindow(hWnd);
				TrackPopupMenu(hPopup, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
				MSG msg;
				if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
				DestroyMenu(hPopup);
			}
			break;
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
	HBRUSH hBrush, hPrevBrush;
	HPEN hPen, hPrevPen;
	HFONT hFont, hPrevFont;
	char szWidth[MAX_PATH], szHeight[MAX_PATH], szMode[MAX_PATH], szFormat[MAX_PATH];
	SIZE sizeWidth = {0,0}, sizeHeight={0,0}, sizeFormat={0,0};
	char *lpszfilename;
	int nFontHeight;
	int nWidth, nHeight;
	POINT pt;

	switch (msg)
	{
	case WM_LBUTTONUP:
		//WINDOWMODE CLIENTMODE FULLSCREENMODEはレイヤーウインドウのMoveWindowにより
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
		//PostQuitMessage(0);
		
		//IDLEMODEに
		SwitchIdleMode();
		break;
	case WM_ERASEBKGND:
		//ユーザーが操作・選択していることをユーザーに示す矩形はここで描画します
		hPen = hPrevPen = NULL;
		hBrush = hPrevBrush = NULL;
		hFont = hPrevFont = NULL;
		GetClientRect(hWnd, &clipRect);
		hdc = GetDC(hWnd);
		hBrush = CreateSolidBrush(BackgroundColor);
		hPrevBrush = (HBRUSH)SelectObject(hdc, hBrush);
		hPen = CreatePen(PS_DASH, 1, ForegroundColor);
		hPrevPen = (HPEN)SelectObject(hdc, hPen);

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

		hPrevFont = (HFONT)SelectObject(hdc, hFont);
		nWidth  = clipRect.right  - clipRect.left;
		nHeight = clipRect.bottom - clipRect.top;

		//描画情報を用意します
		SecureZeroMemory(szWidth, MAX_PATH);
		SecureZeroMemory(szHeight, MAX_PATH);
		SecureZeroMemory(szMode, MAX_PATH);
		SecureZeroMemory(szFormat, MAX_PATH);
		wsprintf(szWidth, "W:%d", nWidth);
		wsprintf(szHeight, "H:%d", nHeight);
		switch(SelectMode){
		case RECTMODE:
			wsprintf(szMode, "RECTANGLE MODE");
			break;
		case WINDOWMODE:
			wsprintf(szMode, "WINDOW MODE");
			break;
		case CLIENTMODE:
			wsprintf(szMode, "CLIENT MODE");
			break;
		case FULLSCREENMODE:
			wsprintf(szMode, "FULLSCREEN MODE");
			break;
		default:
			wsprintf(szMode, "");
			break;
		}
		switch(SaveFormat){
		case IMAGE_BMP:
			wsprintf(szFormat, "BMP");
			break;
		case IMAGE_JPEG:
			wsprintf(szFormat, "JPEG(Q:%d)", nJpegQuality);
			break;
		case IMAGE_PNG:
			wsprintf(szFormat, "PNG(O:%s)", (PngOptimize)?"YES":"NO");
			break;
		}

		//サイズ情報の描画位置を決定（モード情報のY座標も）
		GetTextExtentPoint32(hdc, szWidth, strlen(szWidth), &sizeWidth);
		GetTextExtentPoint32(hdc, szHeight, strlen(szHeight), &sizeHeight);
		GetTextExtentPoint32(hdc, szFormat, strlen(szFormat), &sizeFormat);
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
		TextOut(hdc, clipRect.right - sizeFormat.cx - nFontSize + 1, nFontSize + 1, (LPCTSTR)szFormat, strlen(szFormat));

		//本文
		SetTextColor(hdc, ForegroundColor);
		TextOut(hdc, clipRect.right - sizeWidth.cx, clipRect.bottom - sizeWidth.cy, (LPCTSTR)szWidth, strlen(szWidth));
		TextOut(hdc, clipRect.right - sizeHeight.cx, clipRect.bottom - sizeHeight.cy, (LPCTSTR)szHeight, strlen(szHeight));
		TextOut(hdc, nFontSize, clipRect.bottom - sizeHeight.cy, (LPCTSTR)szMode, strlen(szMode));
		TextOut(hdc, nFontSize, nFontSize, (LPCTSTR)szAppName, strlen(szAppName));
		TextOut(hdc, clipRect.right - sizeFormat.cx - nFontSize, nFontSize, (LPCTSTR)szFormat, strlen(szFormat));

		//後始末
		SelectObject(hdc, hPrevPen);
		SelectObject(hdc, hPrevBrush);
		SelectObject(hdc, hPrevFont);
		DeleteObject(hPen);
		DeleteObject(hBrush);
		DeleteObject(hFont);
		hPen = NULL;
		hBrush = NULL;
		hFont = NULL;
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
	int nWidth, nHeight, nRet;
	RECT rect = {0,0,0,0};

	if(!hdc || !lprect || !filename){
		return FALSE;
	}

	nWidth = nHeight = 0;
	lpBm = NULL;

	rect = *lprect;
	_ValidateRect(&rect);

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

	nRet = -1;
	switch(SaveFormat){
	case IMAGE_BMP:
		nRet = SaveBitmap(filename, bmpInfo, lpBm)?0:-1;
		break;
	case IMAGE_JPEG:
		nRet = DIBtoJPEG(filename, &bmpInfo, lpBm, nJpegQuality);
		break;
	case IMAGE_PNG:
		if(PngOptimize){
			if(UseZopfli){
				UUID uuid;
				unsigned char *lpUuidString = NULL;
				string TempString;
				UuidCreate(&uuid);
				UuidToString(&uuid, &lpUuidString);
				TempString += "_clipss_";
				TempString += (char*)lpUuidString;
				TempString += ".png";
				RpcStringFree(&lpUuidString);
				nRet = DIBtoPNG(TempString.c_str(), &bmpInfo, lpBm);
				if(nRet != 0){
					break;
				}
				nRet = pngrecomp_zopfli(TempString.c_str(), (char*)filename, 15);
				std::remove(TempString.c_str());
			}else{
				nRet = DIBtoPNG_Optimize(filename, &bmpInfo, lpBm);
			}
		}else{
			nRet = DIBtoPNG(filename, &bmpInfo, lpBm);
		}
		break;
	}

	HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, lpBm);
	DeleteDC(bufferDC);
	DeleteObject(bufferBitmap);

	return (nRet == 0)?TRUE:FALSE;
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
	str += "\\";
	str += "clipss";

	//sprintfなんてなかった
	if(PostfixMode == POSTFIX_CONSECUTIVENUMBER){
		str += "_";
		char szNumber[5];
		SecureZeroMemory(szNumber, 4);
		wsprintf(szNumber, "%04u", uLastNumber);
		str += szNumber;
		uLastNumber++;
		SecureZeroMemory(szNumber, 4);
	}else if(PostfixMode == POSTFIX_DATETIME){
		str += "_";
		GetLocalTime(&st);
		char szNumber[15];
		SecureZeroMemory(szNumber, 14);
		wsprintf(szNumber, "%04u%02u%02u%02u%02u%02u", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
		str += szNumber;
		SecureZeroMemory(szNumber, 14);
	}else if(PostfixMode == POSTFIX_HASH){
		lpUuidString = NULL;
		UuidCreate(&uuid);
		UuidToString(&uuid, &lpUuidString);
		str += "_";
		str += (char*)lpUuidString;
		RpcStringFree(&lpUuidString);
		lpUuidString = NULL;
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
	_ValidateRect(&rect);

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

		//１ドット狭める
		//Windows10で縁が取れるのでこれが正解？
		rect.left++;
		rect.top++;
		rect.right--;
		rect.bottom--;
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


///チップウインドウを登録します
///@param hInstance アプリケーションのHINSTANCE
ATOM RegisterTipLayeredWindow(HINSTANCE hInstance)
{
	WNDCLASS wc;

	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = TipLayeredWndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = (HICON)LoadImage(NULL, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
	wc.hCursor       = LoadCursor(NULL, IDC_CROSS);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName  = 0;
	wc.lpszClassName = szTipLayeredWindowClass;

	return RegisterClass(&wc);
}

//手抜きチップウインドウ用文言集
LPCTSTR ppszTips[]={
	"ドラッグアンドドロップ : [RECT MODE]始点終点の矩形領域をキャプチャします",
	"Ctrlボダン : [CLIENT MODE]ウインドウ選択後左クリックでクライアント領域をキャプチャします",
	"Shiftボタン : [WINDOW MODE]ウインドウ選択後左クリックでウインドウ領域をキャプチャします",
	"Altボタン : [FULLSCREEN MODE]左クリックで画面全体をキャプチャします",
	"Spaceボタン : [RECT MODE]に戻ります",
	"Escボタン : キャプチャを中断します。再開はトレイアイコンから",
	"",
	"アイコンが十字の時は各モード中です",
	"現在の選択領域が邪魔で他の領域が選択できない時は再度各ボタンを押してみてください",
	"アプリケーションの終了は（モード中は中断後に）タスクトレイから[終了]を選択してください"
};

const unsigned int uTipsLength = 10;

LRESULT CALLBACK TipLayeredWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	RECT clipRect = {0, 0, 0, 0};
	HBRUSH hBrush, hPrevBrush;
	HPEN hPen, hPrevPen;
	HFONT hFont, hPrevFont;
	int nFontHeight;

	switch (msg)
	{
	case WM_ERASEBKGND:
		hPen = hPrevPen = NULL;
		hBrush = hPrevBrush = NULL;
		hFont = hPrevFont = NULL;
		GetClientRect(hWnd, &clipRect);
		hdc = GetDC(hWnd);
		hBrush = CreateSolidBrush(0x3F3FFF);
		hPrevBrush = (HBRUSH)SelectObject(hdc, hBrush);
		hPen = CreatePen(PS_DASH, 1, ForegroundColor);
		hPrevPen = (HPEN)SelectObject(hdc, hPen);

		//矩形の描画
		Rectangle(hdc, 0, 0, clipRect.right, clipRect.bottom);

		//フォントを作成しサイズなどから描画位置を決定
		nFontHeight = -MulDiv(nFontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
		hFont = CreateFont(nFontHeight, 0, 0, 0,
			FW_REGULAR, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS,
			PROOF_QUALITY,
			FIXED_PITCH | FF_MODERN,
			"Meiryo UI");
		hPrevFont = (HFONT)SelectObject(hdc, hFont);

		//ここから情報を描画
		SetBkMode(hdc, TRANSPARENT);

		for(unsigned int i=0;i<uTipsLength;i++){
			//影付け
			SetTextColor(hdc, ShadowColor);
			TextOut(hdc, nFontSize + 1, nFontSize * (2 * (i + 2) + 1) + 1, (LPCTSTR)ppszTips[i], lstrlen(ppszTips[i]));

			//本文
			SetTextColor(hdc, ForegroundColor);
			TextOut(hdc, nFontSize, nFontSize * (2 * (i + 2) + 1), (LPCTSTR)ppszTips[i], lstrlen(ppszTips[i]));
		}
		
		//影付け
		SetTextColor(hdc, ShadowColor);
		TextOut(hdc, nFontSize + 1, nFontSize + 1, (LPCTSTR)szAppName, lstrlen(szAppName));
		
		//本文
		SetTextColor(hdc, ForegroundColor);
		TextOut(hdc, nFontSize, nFontSize, (LPCTSTR)szAppName, lstrlen(szAppName));
		
		//後始末
		SelectObject(hdc, hPrevPen);
		SelectObject(hdc, hPrevBrush);
		SelectObject(hdc, hPrevFont);
		DeleteObject(hPen);
		DeleteObject(hBrush);
		DeleteObject(hFont);
		hPen = NULL;
		hBrush = NULL;
		hFont = NULL;
		ReleaseDC(hWnd, hdc);
		break;

	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

// タスクトレイのツールチップに表示するライブラリとバージョン情報を作成します
BOOL BuildTooltipString(TCHAR** ppszTip)
{
	if(ppszTip == NULL)
		return FALSE;

	string str = szAppName;
	char *libpngver = new char[64];
	char *libjpegver = new char[64];
	libpngVersionString(&libpngver);
	libjpegVersionString(&libjpegver);

	str += ":";
	str += CLIPSS_VER_STRING;
	str += "\nlibpng:";
	str += libpngver;
#ifdef USE_MOZJPEG
	str += "\nmozjpeg:";
#else
	str += "\nlibjpeg:";
#endif
	str += libjpegver;
	delete [] libpngver;
	delete [] libjpegver;

	memset(*ppszTip, 0, sizeof(TCHAR)*64);
	if(str.length() >= 64){
		strncpy(*ppszTip, str.c_str(), 63);
	}else{
		strncpy(*ppszTip, str.c_str(), str.length());
	}
	return TRUE;
}

//タスクトレイに登録します
BOOL AddNotifyIcon(HWND hWnd, unsigned int uID, HICON hIcon, TCHAR *szTip)
{
	NOTIFYICONDATA nid;

	SecureZeroMemory(&nid, sizeof(NOTIFYICONDATA));
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hWnd;
	nid.uID = uID;
	nid.hIcon = hIcon;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = WM_NOTIFYICON;
#if defined (_MSC_VER)
	//StringCchCopy(nid.szTip, ARRAYSIZE(nid.szTip), szTip); //safe copy
#else
	if(szTip != NULL){
		size_t n = 0;
		if(strlen(szTip) >= 64)
			n = 63;
		else
			n = strlen(szTip);
		strncpy(nid.szTip, szTip, n);
	}
#endif

	return Shell_NotifyIcon(NIM_ADD, &nid);
}

//タスクトレイから削除します
BOOL DeleteNotifyIcon(HWND hWnd, unsigned int uID)
{
	NOTIFYICONDATA nid;

	SecureZeroMemory(&nid, sizeof(NOTIFYICONDATA));
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hWnd;
	nid.uID = uID;
	nid.uFlags = 0;

	return Shell_NotifyIcon(NIM_DELETE, &nid);
}

//アイドルモードに移行します
//複数箇所ありましたのでまとめました
//他のモード移行もまとめた方がよさそうですね
BOOL SwitchIdleMode(void)
{
	SelectMode = IDLEMODE;
	ShowWindow(hLayeredWnd, SW_HIDE);
	ShowWindow(hMainWnd, SW_HIDE);
	if(uIDTimer != 0){
		KillTimer(hMainWnd, uIDTimer);
		uIDTimer = 0;
	}
	return TRUE;
}

//チップウインドウのサイズを計算します
// @param hWnd チップウインドウハンドル
// @param lpSize 計算されたSIZEを受け取るポインタ
BOOL CalcTipWindowSize(HWND hWnd, SIZE *lpSize){
	HDC hdc;
	HFONT hFont, hPrevFont;
	int nFontHeight;
	SIZE size, sizeLongest;

	if(lpSize == NULL){
		return FALSE;
	}

	hFont = hPrevFont = NULL;
	hdc = GetDC(hWnd);
	nFontHeight = -MulDiv(nFontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
	hFont = CreateFont(nFontHeight, 0, 0, 0,
		FW_REGULAR, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		PROOF_QUALITY,
		FIXED_PITCH | FF_MODERN,
		"Meiryo UI");
	hPrevFont = (HFONT)SelectObject(hdc, hFont);

	sizeLongest.cx = sizeLongest.cy = 0;
	for(unsigned int i=0;i<uTipsLength;i++){
		size.cx = size.cy = 0;
		GetTextExtentPoint32(hdc, (LPCTSTR)ppszTips[i], lstrlen(ppszTips[i]), &size);
		if(size.cx > sizeLongest.cx)
			sizeLongest.cx = size.cx;
	}
	SelectObject(hdc, hPrevFont);
	DeleteObject(hFont);
	hFont = NULL;
	ReleaseDC(hWnd, hdc);

	sizeLongest.cy = nFontSize * (2 * (uTipsLength + 2) + 1);

	sizeLongest.cx += (nFontSize * 2);
	sizeLongest.cy += (nFontSize * 2);

	*lpSize = sizeLongest;

	return TRUE;
}

BOOL BuildPopupMenu(HMENU *pHPopup)
{
	if(pHPopup == NULL){
		return FALSE;
	}

	TCHAR szStr[STRINGBUFFERSIZE+1];
	
	//ポップアップメニューを動的に作成
	HMENU hPopup = CreatePopupMenu();
	
	SecureZeroMemory(szStr, sizeof(szStr));
	LoadString(hInst, IDS_MENU_RECTMODE, szStr, STRINGBUFFERSIZE);
	InsertMenu(hPopup, 0, MF_BYCOMMAND | MF_STRING, IDM_RECTMODE, szStr);
	SecureZeroMemory(szStr, sizeof(szStr));
	LoadString(hInst, IDS_MENU_WINDOWMODE, szStr, STRINGBUFFERSIZE);
	InsertMenu(hPopup, 0, MF_BYCOMMAND | MF_STRING, IDM_WINDOWMODE, szStr);
	SecureZeroMemory(szStr, sizeof(szStr));
	LoadString(hInst, IDS_MENU_CLIENTMODE, szStr, STRINGBUFFERSIZE);
	InsertMenu(hPopup, 0, MF_BYCOMMAND | MF_STRING, IDM_CLIENTMODE, szStr);
	SecureZeroMemory(szStr, sizeof(szStr));
	LoadString(hInst, IDS_MENU_FULLSCREENMODE, szStr, STRINGBUFFERSIZE);
	InsertMenu(hPopup, 0, MF_BYCOMMAND | MF_STRING, IDM_FULLSCREENMODE, szStr);
	InsertMenu(hPopup, 0, MF_SEPARATOR, IDM_SEPARATOR, NULL);
	SecureZeroMemory(szStr, sizeof(szStr));
	LoadString(hInst, IDS_MENU_OPENFOLDER, szStr, STRINGBUFFERSIZE);
	InsertMenu(hPopup, 0, MF_BYCOMMAND | MF_STRING, IDM_OPENFOLDER, szStr);
	SecureZeroMemory(szStr, sizeof(szStr));
	LoadString(hInst, IDS_MENU_OPTION, szStr, STRINGBUFFERSIZE);
	InsertMenu(hPopup, 0, MF_BYCOMMAND | MF_STRING, IDM_OPTION, szStr);
	InsertMenu(hPopup, 0, MF_SEPARATOR, IDM_SEPARATOR, NULL);
	SecureZeroMemory(szStr, sizeof(szStr));
	LoadString(hInst, IDS_MENU_EXIT, szStr, STRINGBUFFERSIZE);
	InsertMenu(hPopup, 0, MF_BYCOMMAND | MF_STRING, IDM_EXIT, szStr);

	SecureZeroMemory(szStr, sizeof(szStr));
	*pHPopup = hPopup;

	return TRUE;
}

