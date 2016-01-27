[clipss]

;clipss.iniの説明です
;なおclipss.iniが存在しない場合はアプリケーションによって自動作成されます


;;;;;;;;;;;;;;;;;
;保存ファイル設定
;;;;;;;;;;;;;;;;;


;SaveDir
;画像を保存するディレクトリ
SaveDir=C:\

;TempDir
;一時ファイル用ディレクトリ
TempDir=C:\

;SaveFormat
;保存画像形式
;0=bmp
;1=jpeg
;2=png
SaveFormat=1

;PngOptimize
;PNG形式で保存する時にファイルサイズを最適化するかどうか(boolean)
;巨大な解像度の場合は重いので十分注意してください
;0 = NO
;0 != YES
PngOptimize=1

;UseZopfli
;PNGOptimizeが有効なときにZopfliを使うかどうか(boolean)
;更に重くなりますので注意してください
UseZopfli=1

;JpegQuality
;JPEGの品質指定(0-100)
JpegQuality=75


;PullWindow
;キャプチャする時にウインドウを手前に表示するかどうか(boolean)
;WINDOWMODEとCLIENTMODEの時のみ有効
;0 = NO
;0 != YES
PullWindow=1

;PostfixMode
;保存ファイル接尾語の設定
;0=何も付けない
;1=連番(0000 - 9999)
;2=日付/時刻(YYYYMMDDHHMMSS)
;3=UUID
PostfixMode=1

;LastNumber
;連番用最後に保存した連番数
;0 - 9999
LastNumber=0

;ICMMode
;カラーマネジメントを有効にするかどうか(boolean)
;プロファイルはWindowsで設定されたものを使用します
;0 = NO
;0 != YES
ICMMode=1;


;;;;;;;;;;;;;;;
;画面描画用設定
;;;;;;;;;;;;;;;

;FontName
;画面で表示するフォント名
FontName=Verdana

;FontSize
;フォントサイズ
;5 <= FontSize <= 32
FontSize=10

;ForegroundColor
;文字描画色など用の色(0x00BBGGRRを10進数で)
ForegroundColor=16777215

;BackgroundColor
;背景色(0x00BBGGRRを10進数で)
BackgroundColor=16727871

;ShadowColor
;影の色(0x00BBGGRRを10進数で)
ShadowColor=0
