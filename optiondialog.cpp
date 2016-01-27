//clipssの設定ダイアログとINIファイル操作の実装です
//BOM付きUTF-8での保存を推奨します
//
//Copyright (C) 2014 - 2016, guardiancrow

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include <shlobj.h>

#include <string>

#include "clipssdef.h"
#include "resource.h"
#include "strutil.hpp"
#include "optiondialog.h"

using namespace std;

//これらのパラメータの扱いは（構造体・クラス化するなど）何とかしたいですね
extern TCHAR szTempDir[MAX_PATH+1];
extern TCHAR szSaveDir[MAX_PATH+1];
extern int SaveFormat;
extern BOOL PngOptimize;
extern BOOL UseZopfli;
extern int nJpegQuality;
extern BOOL PullWindow;
extern int PostfixMode;
extern TCHAR szFontName[MAX_PATH+1];
extern int nFontSize;
extern COLORREF ForegroundColor;
extern COLORREF BackgroundColor;
extern COLORREF ShadowColor;
extern UINT uLastNumber;
extern BOOL ICMMode;
extern HINSTANCE hInst;


/// アプリケーションのあるディレクトリ文字列を取得します
///
/// @param pDir 文字列へのポインタ
/// @param uBufSize pDirのバッファサイズ
void GetProcessDirectory(char *pDir, unsigned int uBufSize)
{
	char *lpDir = new char[MAX_PATH+1];
	SecureZeroMemory(lpDir, MAX_PATH);
	GetModuleFileName(NULL, lpDir, MAX_PATH);
	PathRemoveFileSpec(lpDir);	//shlwapi.h 最後の\は無い

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
	strINIFileName += "\\clipss.ini";

	SecureZeroMemory(szSaveDir, MAX_PATH);
	SecureZeroMemory(szTempDir, MAX_PATH);
	SecureZeroMemory(szFontName, MAX_PATH);
	
	GetPrivateProfileString(_T("clipss"), _T("SaveDir"), lpProcessDir, szSaveDir, MAX_PATH, strINIFileName.c_str());
	GetPrivateProfileString(_T("clipss"), _T("TempDir"), lpProcessDir, szTempDir, MAX_PATH, strINIFileName.c_str());
	SaveFormat = GetPrivateProfileInt(_T("clipss"), _T("SaveFormat"), 1, strINIFileName.c_str());
	PngOptimize = GetPrivateProfileInt(_T("clipss"), _T("PngOptimize"), 0, strINIFileName.c_str());
	UseZopfli = GetPrivateProfileInt(_T("clipss"), _T("UseZopfli"), 0, strINIFileName.c_str());
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
	}else if(nFontSize > 32){
		nFontSize = 32;
	}
	if(uLastNumber > 9999){
		uLastNumber = 0;
	}

	return TRUE;
}


/// アプリケーションの設定を保存します
BOOL SaveProfiles(void)
{
	string strSaveFormat = toString(SaveFormat);
	string strPngOptimize = toString(PngOptimize);
	string strUseZopfli = toString(UseZopfli);
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
	strINIFileName += "\\clipss.ini";
	delete[] lpProcessDir;

	WritePrivateProfileString(_T("clipss"), _T("SaveDir"), szSaveDir, strINIFileName.c_str());
	WritePrivateProfileString(_T("clipss"), _T("TempDir"), szTempDir, strINIFileName.c_str());
	WritePrivateProfileString(_T("clipss"), _T("SaveFormat"), strSaveFormat.c_str(), strINIFileName.c_str());
	WritePrivateProfileString(_T("clipss"), _T("PngOptimize"), strPngOptimize.c_str(), strINIFileName.c_str());
	WritePrivateProfileString(_T("clipss"), _T("UseZopfli"), strUseZopfli.c_str(), strINIFileName.c_str());
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

/// 設定ダイアログのパラメータの初期化を行います
BOOL OptionDialog_InitSettings(HWND hDlg)
{
	//InitParams
	HWND hComboSaveFormat = GetDlgItem(hDlg, IDC_COMBOSAVEFORMAT);
	HWND hComboPostfix = GetDlgItem(hDlg, IDC_COMBOPOSTFIXMODE);

	TCHAR szStr[STRINGBUFFERSIZE+1];
	SecureZeroMemory(szStr, sizeof(szStr));

	LoadString(hInst, IDS_IMAGE_BMP, szStr, STRINGBUFFERSIZE);
	SendMessage(hComboSaveFormat, CB_ADDSTRING, 0, (LPARAM)szStr);

	LoadString(hInst, IDS_IMAGE_JPEG, szStr, STRINGBUFFERSIZE);
	SendMessage(hComboSaveFormat, CB_ADDSTRING, 0, (LPARAM)szStr);

	LoadString(hInst, IDS_IMAGE_PNG, szStr, STRINGBUFFERSIZE);
	SendMessage(hComboSaveFormat, CB_ADDSTRING, 0, (LPARAM)szStr);

	LoadString(hInst, IDS_POSTFIX_NONE, szStr, STRINGBUFFERSIZE);
	SendMessage(hComboPostfix, CB_ADDSTRING, 0, (LPARAM)szStr);

	LoadString(hInst, IDS_POSTFIX_CONSECUTIVENUMBER, szStr, STRINGBUFFERSIZE);
	SendMessage(hComboPostfix, CB_ADDSTRING, 0, (LPARAM)szStr);

	LoadString(hInst, IDS_POSTFIX_DATETIME, szStr, STRINGBUFFERSIZE);
	SendMessage(hComboPostfix, CB_ADDSTRING, 0, (LPARAM)szStr);
	
	LoadString(hInst, IDS_POSTFIX_HASH, szStr, STRINGBUFFERSIZE);
	SendMessage(hComboPostfix, CB_ADDSTRING, 0, (LPARAM)szStr);

	SecureZeroMemory(szStr, sizeof(szStr));

	return TRUE;
}

/// 設定ダイアログのパラメータを読み込みます
BOOL OptionDialog_LoadSettings(HWND hDlg)
{
	//LoadParams
	LoadProfiles();

	HWND hComboSaveFormat = GetDlgItem(hDlg, IDC_COMBOSAVEFORMAT);
	HWND hComboPostfix = GetDlgItem(hDlg, IDC_COMBOPOSTFIXMODE);

	SetDlgItemText(hDlg, IDC_EDITSAVEDIR, szSaveDir);
	SetDlgItemInt(hDlg, IDC_EDITJPEGQUALITY, nJpegQuality, TRUE);
	SetDlgItemInt(hDlg, IDC_EDITLASTNUMBER, uLastNumber, TRUE);

	if(PngOptimize){
		CheckDlgButton(hDlg, IDC_CHECKPNGOPTIMIZE, BST_CHECKED);
	}else{
		CheckDlgButton(hDlg, IDC_CHECKPNGOPTIMIZE, BST_UNCHECKED);
	}

	if(PullWindow){
		CheckDlgButton(hDlg, IDC_CHECKPULLWINDOW, BST_CHECKED);
	}else{
		CheckDlgButton(hDlg, IDC_CHECKPULLWINDOW, BST_UNCHECKED);
	}

	if(ICMMode){
		CheckDlgButton(hDlg, IDC_CHECKICMMODE, BST_CHECKED);
	}else{
		CheckDlgButton(hDlg, IDC_CHECKICMMODE, BST_UNCHECKED);
	}

	TCHAR szStr[STRINGBUFFERSIZE+1];
	SecureZeroMemory(szStr, sizeof(szStr));

	switch(SaveFormat){
	case IMAGE_BMP:
		LoadString(hInst, IDS_IMAGE_BMP, szStr, STRINGBUFFERSIZE);
		SendMessage(hComboSaveFormat, CB_SELECTSTRING, 0, (LPARAM)szStr);
		break;
	case IMAGE_JPEG:
		LoadString(hInst, IDS_IMAGE_JPEG, szStr, STRINGBUFFERSIZE);
		SendMessage(hComboSaveFormat, CB_SELECTSTRING, 0, (LPARAM)szStr);
		break;
	case IMAGE_PNG:
		LoadString(hInst, IDS_IMAGE_PNG, szStr, STRINGBUFFERSIZE);
		SendMessage(hComboSaveFormat, CB_SELECTSTRING, 0, (LPARAM)szStr);
		break;
	default:
		return FALSE;
	}

	switch(PostfixMode){
	case POSTFIX_NONE:
		LoadString(hInst, IDS_POSTFIX_NONE, szStr, STRINGBUFFERSIZE);
		SendMessage(hComboPostfix, CB_SELECTSTRING, 0, (LPARAM)szStr);
		break;
	case POSTFIX_CONSECUTIVENUMBER:
		LoadString(hInst, IDS_POSTFIX_CONSECUTIVENUMBER, szStr, STRINGBUFFERSIZE);
		SendMessage(hComboPostfix, CB_SELECTSTRING, 0, (LPARAM)szStr);
		break;
	case POSTFIX_DATETIME:
		LoadString(hInst, IDS_POSTFIX_DATETIME, szStr, STRINGBUFFERSIZE);
		SendMessage(hComboPostfix, CB_SELECTSTRING, 0, (LPARAM)szStr);
		break;
	case POSTFIX_HASH:
		LoadString(hInst, IDS_POSTFIX_HASH, szStr, STRINGBUFFERSIZE);
		SendMessage(hComboPostfix, CB_SELECTSTRING, 0, (LPARAM)szStr);
		break;
	default:
		return FALSE;
	}

	SecureZeroMemory(szStr, sizeof(szStr));

	//Enable/Disable
	OptionDialog_UpdateSaveFormat(hDlg);
	OptionDialog_UpdatePostfixMode(hDlg);
	
	return TRUE;
}

/// 別の保存形式が選択された時に他のコントロールを更新します
BOOL OptionDialog_UpdateSaveFormat(HWND hDlg)
{
	HWND hPngOptWnd = GetDlgItem(hDlg, IDC_CHECKPNGOPTIMIZE);
	HWND hJpegQualWnd = GetDlgItem(hDlg, IDC_EDITJPEGQUALITY);
	HWND hJpegQualSpinWnd = GetDlgItem(hDlg, IDC_SPINJPEGQUALITY);

	TCHAR SelItem[STRINGBUFFERSIZE+1];
	SecureZeroMemory(SelItem, sizeof(SelItem));
	GetDlgItemText(hDlg, IDC_COMBOSAVEFORMAT, SelItem, STRINGBUFFERSIZE);

	if(strcmpi(SelItem, "BMP") == 0){
		EnableWindow(hJpegQualWnd, FALSE);
		EnableWindow(hJpegQualSpinWnd, FALSE);
		EnableWindow(hPngOptWnd, FALSE);
	}else if(strcmpi(SelItem, "JPEG") == 0){
		EnableWindow(hJpegQualWnd, TRUE);
		EnableWindow(hJpegQualSpinWnd, TRUE);
		EnableWindow(hPngOptWnd, FALSE);
	}else if(strcmpi(SelItem, "PNG") == 0){
		EnableWindow(hJpegQualWnd, FALSE);
		EnableWindow(hJpegQualSpinWnd, FALSE);
		EnableWindow(hPngOptWnd, TRUE);
	}else{
		EnableWindow(hJpegQualWnd, FALSE);
		EnableWindow(hJpegQualSpinWnd, FALSE);
		EnableWindow(hPngOptWnd, FALSE);
	}

	OptionDialog_UpdatePngOptimize(hDlg);
	
	return TRUE;
}

/// PNG最適化の設定に変化があったときに他のコントロールを更新します
BOOL OptionDialog_UpdatePngOptimize(HWND hDlg)
{
	HWND hPngOptWnd = GetDlgItem(hDlg, IDC_CHECKPNGOPTIMIZE);
	HWND hZopfliWnd = GetDlgItem(hDlg, IDC_CHECKUSEZOPFLI);

	if(!IsWindowEnabled(hPngOptWnd)){
		EnableWindow(hZopfliWnd, FALSE);
		return TRUE;
	}
	if(IsDlgButtonChecked(hDlg, IDC_CHECKPNGOPTIMIZE) == BST_CHECKED){
		EnableWindow(hZopfliWnd, TRUE);
	}else{
		EnableWindow(hZopfliWnd, FALSE);
	}
	return TRUE;
}

/// 別のファイル接尾語の設定が選択された時に他のコントロールを更新します
BOOL OptionDialog_UpdatePostfixMode(HWND hDlg)
{
	HWND hLastNumberWnd = GetDlgItem(hDlg, IDC_EDITLASTNUMBER);
	HWND hLastNumberSpinWnd = GetDlgItem(hDlg, IDC_SPINLASTNUMBER);

	TCHAR SelItem[STRINGBUFFERSIZE+1];
	SecureZeroMemory(SelItem, sizeof(SelItem));
	GetDlgItemText(hDlg, IDC_COMBOPOSTFIXMODE, SelItem, STRINGBUFFERSIZE);

	TCHAR szStr[STRINGBUFFERSIZE+1];
	SecureZeroMemory(szStr, sizeof(szStr));
	LoadString(hInst, IDS_POSTFIX_CONSECUTIVENUMBER, szStr, STRINGBUFFERSIZE);
	if(strcmpi(SelItem, szStr) == 0){
		EnableWindow(hLastNumberWnd, TRUE);
		EnableWindow(hLastNumberSpinWnd, TRUE);
	}else{
		EnableWindow(hLastNumberWnd, FALSE);
		EnableWindow(hLastNumberSpinWnd, FALSE);
	}
	SecureZeroMemory(szStr, sizeof(szStr));

	return TRUE;
}

/// フォルダ選択ダイアログを表示し、利用者にフォルダの選択を促します
BOOL OptionDialog_ChooseFolder(HWND hDlg, LPTSTR lpszDir, UINT bufsize)
{
	if(hDlg == NULL || lpszDir == NULL || bufsize == 0 || bufsize > 4096){
		return FALSE;
	}

	LPTSTR PathName = 0;
	PathName = new TCHAR[bufsize+1];

	SecureZeroMemory(PathName, sizeof(TCHAR) * bufsize);
	GetDlgItemText(hDlg, IDC_EDITSAVEDIR, PathName, MAX_PATH);
	BROWSEINFO bi = {0};
	IMalloc* imalloc;
	imalloc = 0;

	TCHAR szStr[STRINGBUFFERSIZE+1];
	SecureZeroMemory(szStr, sizeof(szStr));
	LoadString(hInst, IDS_DIALOG_CHOOSESAVEDIR, szStr, STRINGBUFFERSIZE);

	bi.hwndOwner = hDlg;
	bi.lpszTitle = szStr;
	bi.lpfn = (BFFCALLBACK)&OptionDialog_ChooseFolderCallBack;
	bi.lParam = (LPARAM)PathName;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

	LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

	SecureZeroMemory(szStr, sizeof(szStr));

	if(pidl != 0){
		SHGetPathFromIDList(pidl, PathName);
		if(SUCCEEDED(SHGetMalloc(&imalloc))){
			imalloc->Free(pidl);
			imalloc->Release();
		}
		strncpy(lpszDir, PathName, bufsize);
		SecureZeroMemory(PathName, sizeof(TCHAR) * bufsize);
		delete [] PathName;
		return TRUE;
	}

	return FALSE;
}

BOOL CALLBACK OptionDialog_ChooseFolderCallBack(HWND hWnd, UINT msg, LPARAM lParam, LPARAM lpData)
{
	if(msg == BFFM_INITIALIZED){
		SendMessage(hWnd, BFFM_SETSELECTION, (WPARAM)TRUE, lpData);
	}
	return 0;
}

/// 別の保存フォルダが選択された時に他のコントロールを更新します
BOOL OptionDialog_UpdateSaveDir(HWND hDlg, LPTSTR lpszDir, UINT bufsize)
{
	if(!PathFileExists(lpszDir)){
		return FALSE;
	}else{
		SetDlgItemText(hDlg, IDC_EDITSAVEDIR, lpszDir);
	}

	return TRUE;
}

/// ダイアログで指定されたパラメータを保存します
/// 不正なパラメータを検出した場合は保存を中止しFALSEを返します
/// パラメータが正常で保存が成功した場合はTRUEを返します
BOOL OptionDialog_SaveSettings(HWND hDlg)
{
	//DirectoryExists?
	TCHAR SelSaveDir[MAX_PATH+1];
	SecureZeroMemory(SelSaveDir, sizeof(SelSaveDir));
	GetDlgItemText(hDlg, IDC_EDITSAVEDIR, SelSaveDir, MAX_PATH);

	if(!PathFileExists(SelSaveDir)){
		SecureZeroMemory(SelSaveDir, sizeof(SelSaveDir));
		return FALSE;
	}

	SecureZeroMemory(szSaveDir, sizeof(szSaveDir));
	strncpy(szSaveDir, SelSaveDir, sizeof(szSaveDir)-1);
	SecureZeroMemory(SelSaveDir, sizeof(SelSaveDir));

	TCHAR szStr[STRINGBUFFERSIZE+1];
	TCHAR SelItem[STRINGBUFFERSIZE+1];

	SecureZeroMemory(SelItem, sizeof(SelItem));
	GetDlgItemText(hDlg, IDC_COMBOSAVEFORMAT, SelItem, STRINGBUFFERSIZE);

	SecureZeroMemory(szStr, sizeof(szStr));
	LoadString(hInst, IDS_IMAGE_BMP, szStr, STRINGBUFFERSIZE);
	if(strcmpi(SelItem, szStr) == 0){
		SaveFormat = IMAGE_BMP;
	}

	SecureZeroMemory(szStr, sizeof(szStr));
	LoadString(hInst, IDS_IMAGE_JPEG, szStr, STRINGBUFFERSIZE);	
	if(strcmpi(SelItem, szStr) == 0){
		SaveFormat = IMAGE_JPEG;
	}
	
	SecureZeroMemory(szStr, sizeof(szStr));
	LoadString(hInst, IDS_IMAGE_PNG, szStr, STRINGBUFFERSIZE);
	if(strcmpi(SelItem, szStr) == 0){
		SaveFormat = IMAGE_PNG;
	}

	SecureZeroMemory(SelItem, sizeof(SelItem));
	GetDlgItemText(hDlg, IDC_COMBOPOSTFIXMODE, SelItem, STRINGBUFFERSIZE);

	SecureZeroMemory(szStr, sizeof(szStr));
	LoadString(hInst, IDS_POSTFIX_NONE, szStr, STRINGBUFFERSIZE);
	if(strcmpi(SelItem, szStr) == 0){
		PostfixMode = POSTFIX_NONE;
	}

	SecureZeroMemory(szStr, sizeof(szStr));
	LoadString(hInst, IDS_POSTFIX_CONSECUTIVENUMBER, szStr, STRINGBUFFERSIZE);
	if(strcmpi(SelItem, szStr) == 0){
		PostfixMode = POSTFIX_CONSECUTIVENUMBER;
	}

	SecureZeroMemory(szStr, sizeof(szStr));
	LoadString(hInst, IDS_POSTFIX_DATETIME, szStr, STRINGBUFFERSIZE);
	if(strcmpi(SelItem, szStr) == 0){
		PostfixMode = POSTFIX_DATETIME;
	}

	SecureZeroMemory(szStr, sizeof(szStr));
	LoadString(hInst, IDS_POSTFIX_HASH, szStr, STRINGBUFFERSIZE);
	if(strcmpi(SelItem, szStr) == 0){
		PostfixMode = POSTFIX_HASH;
	}

	UINT u = 0;
	u = GetDlgItemInt(hDlg, IDC_EDITJPEGQUALITY, NULL, FALSE);
	if(u > 100){
		u = 100;
	}
	nJpegQuality = (int)u;

	u = GetDlgItemInt(hDlg, IDC_EDITLASTNUMBER, NULL, FALSE);
	if(u > 9999){
		u = 0;
	}
	uLastNumber = u;

	SecureZeroMemory(SelItem, sizeof(SelItem));
	SecureZeroMemory(szStr, sizeof(szStr));

	if(IsDlgButtonChecked(hDlg, IDC_CHECKPNGOPTIMIZE) == BST_CHECKED){
		PngOptimize = TRUE;
	}else{
		PngOptimize = FALSE;
	}

	if(IsDlgButtonChecked(hDlg, IDC_CHECKUSEZOPFLI) == BST_CHECKED){
		UseZopfli = TRUE;
	}else{
		UseZopfli = FALSE;
	}


	if(IsDlgButtonChecked(hDlg, IDC_CHECKPULLWINDOW) == BST_CHECKED){
		PullWindow = TRUE;
	}else{
		PullWindow = FALSE;
	}
	
	if(IsDlgButtonChecked(hDlg, IDC_CHECKICMMODE) == BST_CHECKED){
		ICMMode = TRUE;
	}else{
		ICMMode = FALSE;
	}

	//SaveParams
	if(!SaveProfiles()){
		return FALSE;
	}
	
	return TRUE;
}

/// DialogProcです
INT_PTR CALLBACK OptionDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	TCHAR PathName[MAX_PATH+1];
	TCHAR szStr[STRINGBUFFERSIZE+1];
	NMUPDOWN nmud;
	BOOL b;

    switch (message)
    {
    case WM_INITDIALOG:
		OptionDialog_InitSettings(hDlg);
		OptionDialog_LoadSettings(hDlg);
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK){
			if(!OptionDialog_SaveSettings(hDlg)){
				SecureZeroMemory(szStr, sizeof(szStr));
				LoadString(hInst, IDS_ERR_INVALIDPARAM, szStr, STRINGBUFFERSIZE);
				MessageBox(hDlg, szStr, "CLIPSS", MB_OK);
				SecureZeroMemory(szStr, sizeof(szStr));
				return (INT_PTR)TRUE;
			}
			EndDialog(hDlg, LOWORD(wParam));
        }else if(LOWORD(wParam) == IDCANCEL){
			EndDialog(hDlg, LOWORD(wParam));
		}else if(LOWORD(wParam) == IDC_BUTTONSAVEDIR){
			OptionDialog_ChooseFolder(hDlg, PathName, MAX_PATH);
			OptionDialog_UpdateSaveDir(hDlg, PathName, MAX_PATH);
		}else if(LOWORD(wParam) == IDC_COMBOSAVEFORMAT){
			if(HIWORD(wParam) == CBN_SELCHANGE){
				OptionDialog_UpdateSaveFormat(hDlg);
			}
		}else if(LOWORD(wParam) == IDC_COMBOPOSTFIXMODE){
			if(HIWORD(wParam) == CBN_SELCHANGE){
				OptionDialog_UpdatePostfixMode(hDlg);
			}
		}else if(LOWORD(wParam) == IDC_CHECKPNGOPTIMIZE){
			OptionDialog_UpdatePngOptimize(hDlg);
        }else{
			return (INT_PTR)FALSE;
		}
		return (INT_PTR)TRUE;

	case WM_NOTIFY:
		int i;
		if(wParam == (WPARAM)IDC_SPINJPEGQUALITY){
			nmud = *(LPNMUPDOWN)lParam;
			if(nmud.hdr.code == UDN_DELTAPOS){
				i = (int)GetDlgItemInt(hDlg, IDC_EDITJPEGQUALITY, &b, TRUE);
				if(!b){
					break;
				}
				if(nmud.iDelta < 0){
					i++;
				}else if(nmud.iDelta > 0){
					i--;
				}
				if(i < 0){
					i = 0;
				}else if(i > 100){
					i = 100;
				}
				b = SetDlgItemInt(hDlg, IDC_EDITJPEGQUALITY, (UINT)i, TRUE);
			}
		}else if(wParam == (WPARAM)IDC_SPINLASTNUMBER){
			nmud = *(LPNMUPDOWN)lParam;
			if(nmud.hdr.code == UDN_DELTAPOS){
				i = (int)GetDlgItemInt(hDlg, IDC_EDITLASTNUMBER, &b, TRUE);
				if(!b){
					break;
				}
				if(nmud.iDelta < 0){
					i++;
				}else if(nmud.iDelta > 0){
					i--;
				}
				if(i < 0){
					i = 9999;
				}else if(i > 9999){
					i = 0;
				}
				b = SetDlgItemInt(hDlg, IDC_EDITLASTNUMBER, (UINT)i, TRUE);
			}
		}
		break;
    }
    return (INT_PTR)FALSE;
}
