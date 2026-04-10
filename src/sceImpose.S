	.set noreorder

#include "pspimport.s"

	IMPORT_START "sceImpose_driver",0x00090000

/*
	フラグ0x00090000がミソで、遅延バインディングを行うらしい。

	プラグインからsceImposeを使う場合、プラグインがロードされる段階では
	sceImpose関連のモジュールが読み込まれていないため、このようにしないとロードに失敗する。
    更にPSPSDKのsceImpose_driver.Sでは、0x00010011(これの詳細もよくわからない)になっているので、
	このようなファイルで自分でフラグを指定する必要がある。

	0x40090000でもうまくいくようで、0x4000があるのとないのでは、
	バインドする関数のカーネル/ユーザモードの切り替えのようなことをするっぽいが、今のところよくわからない。

	また、実際の関数自体はPSPSDKがインポートしてくれるので指定はいらないみたい。

	メモ
	ちゃんと指定したとおりにPRXがリンクしたかどうかは、
	prxtool -f <prx>

	prxtoolは
	svn co svn://svn.ps2dev.org/psp/trunk/prxtool
*/