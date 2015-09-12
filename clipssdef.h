//Copyright (C) 2014 - 2015, guardiancrow

#pragma once

/// 文字列のバッファサイズです
#define STRINGBUFFERSIZE	1024

/// タスクトレイ用の定義です
#define WM_NOTIFYICON (WM_USER+123)
#define ID_TASKTRAY 0

/// HWNDがオーナーウインドウかどうかを検査します
#define IsWindowOwner(h) (GetWindow(h,GW_OWNER) == NULL)
#ifdef __MINGW32__
/// MinGWではSecureZeroMemoryは定義されていません
#define SecureZeroMemory(p,s) RtlFillMemory((p),(s),0)
#endif

///各クリップモードの定義です
enum CLIPSS_MODE{
	RECTMODE=0,
	WINDOWMODE,
	CLIENTMODE,
	FULLSCREENMODE,
	IDLEMODE,
	READYMODE//DISABLEMODE
};

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
