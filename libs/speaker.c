/* 謝辞： 下記のコードは，旧H8ボードマイコン用に，重村先生がコードを        */
/*        作成され，後に，現H8ボードマイコンのために，H22年度新田卒研の     */
/*        卒業研究生らによって，改変されたコードです。                      */
/*        謹んで，各位に対し，ここで，謝辞を申し上げます(新田＠徳山高専)    */
/*        卒業研究生作成の赤外線LEDの実装と，小職作成のマトリクスLEDの実装  */
/*        の整合性が取れていないため，残念ながら，一部のコードは，小職に    */
/*        よって，コメントアウトされてしまいました。                        */

#include "speaker.h"
#include "libsCore.h"

static int snd_cnt;					/* 音の継続時間 */
static char *snd_ptr;					/* 次の音の位置 */

/*
 * スピーカーに音を出力します。
 *   使用方法：トップハーフから 1/4 音符の 1/16 時間毎に呼び出します。
 */
void snd_mng(void) {
 /* 2MHz からの分周比      ラ    シ    ド    レ    ミ    ファ  ソ  */
  static int doremi1[] = { 4545, 4048, 3824, 3401, 3030, 2865, 2551 };
  static int doremi2[] = { 4291, 3824, 3610, 3215, 2865, 2702, 2409 };  /*＃*/
	
	/*
	 * 音階の決まり方（余談）
	 *   音階はスピーカーを動作させる周波数によって決定されます。
	 *   ((16 * (10^6)) / 8) / 4 545 = 440.044004 [Hz]	→ ラ
	 *   ((16 * (10^6)) / 8) / 4 048 = 494.071146 [Hz]	→ シ
	 *   ((16 * (10^6)) / 8) / 3 824 = 523.012552 [Hz]	→ ド
	 *   タイマーWはφ/8でカウントされるよう設定してあるので、このような
	 *   計算式によって、音階の周波数と一致していることが確認できます。
	 */
	
	static int chop;			/* 音の区切り   */
	static int sft;			        /* 音の高さをずらす */
	int *doremi = doremi1;
	int l = 16;				/* 音の長さ 1/4 音符 */
	int p = 0;				/* 音の高さ          */
	char c;					/* 解釈する文字      */
	
	if (snd_cnt==0 || --snd_cnt!=0) return;	/* まだ前の状態が継続 */
	
	TMRW = 0x48;				/* タイマーの停止 */	
	if (chop!=0) {				/* 音の出力が終了 */
		snd_cnt = chop;			/* 音の区切り     */
		chop = 0;
	} else {				/* 次の音を出力 */
		while ((c=*snd_ptr++) != '\0') {/* 楽譜から１文字取る */
			if ('A'<=c && c<='G') {	/* 音の名前ならタイマ */
				p = doremi[ c - 'A' ];	/* にセットする値を */
				break;			/* 決める           */
			} else if (c==' ') {
			  p = -1;			/* 休符の場合は負   */
				break;
			} else if (c == '#') {
			  doremi = doremi2;		/* 半音ずらす       */
			} else if (c == '^') {
			  sft++;			/* １オクターブ高く */
			} else if (c == '_') {
				sft--;			/* １オクターブ低く */
			} else if (c == '!') {
				l/=2;			/* 短く */
				if (l<1) l=1;
			} else if (c == '-') {
				l*=2;			/* 長く */
			} else if (c == '.') {
				l=l+l/2;		/* 符点(1.5倍) */
			}
		}
		
		if (p!=0) {				/* 音が決まったら */
			int i;				/* タイマＷで音を出す*/
			
			if (p>0) {
				if (sft>0) {	    /* オクターブ単位の変化 */
					for (i=0; i<sft; i++) p /= 2;
				} else {
					for (i=0; i>sft; i--) p *= 2;
				}
				GRA = p;		/* 周期             */
				GRB = p/2;		/* デューティ       */
				TMRW = 0xc8;		/* タイマスタート   */
			}
			snd_cnt = l - 1;	       	/* 音の継続時間     */
			chop   = 1;			/* 区切りの継続時間 */
		} else {				/* 楽譜が終わった   */
			snd_ptr  = NULL;
			sft  = 0;
			chop = 0;
			
#if 0		       
			/* タイマVは，常時1msで駆動する*/
			TCRV0 = 0x00;	      	/* タイマーVを停止 */
			TCRV1 = 0xe2;
#endif
			
		}
	}
}


/*
 * スピーカ用にタイマーVとタイマーWを初期化し、スピーカーで音楽を演奏します。
 *   使用方法：楽譜を渡すと非同期で演奏します。
 *   注意事項：赤外線通信と共存できません。
 *   戻り値：演奏開始できればTRUE、できなければFALSEを返します。
 *   引数：楽譜を表す文字列,以下の文字が使用できます。
 *     'ABCDEFG' : 音の名前
 *     ' '       : 休符
 *     '^_'      : 演奏ルーチンの状態を１オクターブ上げる／下げる
 *     '!-'      : 次の音の長さを半分にする／倍にする
 *     '.'       : 次の音の長さを1.5倍にする
 *     '#'       : ＃
 *
 *     例 カエルの歌 "CDEFEDC EFG^A_GFE C C C C !C!C!D!D!E!E!F!FEDC"
 */
int snd_play(char *score) {
#if 0
  /* 赤外線通信は，2011年度は，未サポート予定 */
	if(ir_check()) return FALSE;			/* 赤外線通信中 */
#endif
	if (snd_ptr != NULL) return FALSE;		/* 演奏中 */
	
	snd_ptr = score;
	snd_cnt = 1;
	
	/* タイマーW */
	TIOR0 = 0xb8;	/* FTIOBにGRBコンペアマッチによるトグル出力を行う */
	TIOR1 = 0x88;	/* FTIOC/FTIODにGRC/GRDコンペアマッチによるトグル */
			/* 出力を行わない */
	
#if 0
	/* 2011年度は，常時,タイマVは，1msで動作させる予定です。*/
	/* タイマーV
	 *   1 / (((16 * (10^6)) / 128) / 125) = 0.001
	 *     φ/128クロックのとき125カウントで1[ms]
	 */
	TCORA = 125;	/* コンペアマッチAはスピーカー用で、割り込み間隔は*/
	                /*約1ms */
	TCRV0 = 0x4b;	/* コンペアマッチAで割り込みとクリア  */
	TCRV1 = 0x01;	/* φ/128でカウント（制御ビットがTCRV0/TCRV1に*/
#endif

	/* GRA/GRBは音程によって差し替えるのでここで設定しない */
	
	/* 音楽開始 */
	return TRUE;
}

/*
 * 音楽の演奏を中止します。
 */
void snd_stop(void) {
	if (snd_ptr != NULL)
	  snd_ptr = "";
}

/*
 * 音楽を演奏中か調べます。
 *   戻り値：演奏中ならTRUE、そうでないならFALSEを返します。
 */
int snd_check(void) {
	return snd_ptr != NULL;
}



