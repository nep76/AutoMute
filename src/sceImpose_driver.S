	.set noreorder

#include "pspimport.s"

	IMPORT_START "sceImpose_driver",0x00090000

/*
	フラグ0x00090000が重要。
	遅延バインディングを行うらしい。
	これによって、プラグインロード時に読み込まれていない動的ライブラリの関数を呼んでいても、
	リンク失敗による例外が起こらない。

	0x40000000を入れてもうまくいくようで、あるのとないのでは、
	バインドする関数のカーネル/ユーザモードの切り替えのようなことをするっぽいが、今のところよくわからない。

	また、実際の関数自体はPSPSDKがインポートしてくれるので指定はいらないみたい。
	そもそも自分で指定すると
		Error, could not fixup imports, stubs out of order.
	といわれる。
	どういうこと？何が起きてるの？

	メモ
	ちゃんと指定したとおりにPRXがリンクしたかどうかは、
	prxtool -f <prx>

	prxtoolは
	svn co svn://svn.ps2dev.org/psp/trunk/prxtool
*/