//clipssの設定ダイアログとINIファイル操作の実装です
//
//Copyright (C) 2014 - 2015, guardiancrow

#pragma once

#ifdef __cplusplus
extern "C"{
#endif

void GetProcessDirectory(char *pDir, unsigned int uBufSize);

BOOL LoadProfiles(void);
BOOL SaveProfiles(void);

BOOL OptionDialog_InitSettings(HWND hDlg);
BOOL OptionDialog_LoadSettings(HWND hDlg);
BOOL OptionDialog_SaveSettings(HWND hDlg);
BOOL OptionDialog_UpdateSaveFormat(HWND hDlg);
BOOL OptionDialog_UpdatePostfixMode(HWND hDlg);
BOOL OptionDialog_ChooseFolder(HWND hDlg, LPTSTR lpszDir, UINT bufsize);
BOOL CALLBACK OptionDialog_ChooseFolderCallBack(HWND, UINT, LPARAM, LPARAM);
BOOL OptionDialog_UpdateSaveDir(HWND hDlg, LPTSTR lpszDir, UINT bufsize);

#ifdef __cplusplus
}
#endif
