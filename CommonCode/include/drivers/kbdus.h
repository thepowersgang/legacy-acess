
#ifndef _KBDUS_H
#define _KBDUS_H

// - BASE (NO PREFIX)
Uint8	gpKBDUS1[256] = {
	// 00
	0,  KEY_ESC, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
	// 10
	'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', KEY_LCTRL, 'a', 's',
	// 20
	'd', 'f', 'g', 'h', 'j', 'k', 'l', ';','\'', '`', KEY_LSHIFT,'\\', 'z', 'x', 'c', 'v',
	// 30
	'b','n','m',',','.','/',KEY_RSHIFT,KEY_KPSTAR,KEY_LALT,' ',KEY_CAPSLOCK,KEY_F1,KEY_F2,KEY_F3,KEY_F4, KEY_F5,
	// 40
	KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_NUMLOCK, KEY_SCROLLLOCK, KEY_KPHOME, KEY_KPUP, KEY_KPPGUP, KEY_KPMINUS, KEY_KPLEFT, KEY_KP5, KEY_KPRIGHT, KEY_KPPLUS, KEY_KPEND,	/* 4F9 - Keypad End key*/
	// 50
    KEY_KPDOWN, KEY_KPPGDN, KEY_KPINS, KEY_KPDEL, 0, 0, 0, KEY_F11, KEY_F12, 0
};
// - 0xE0 Prefixed
Uint8	gpKBDUS2[256] = {
//   	0  1  2   3  4   5  6  7   8  9   A  B   C  D   E  F
/*00*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //0-F
/*10*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, KEY_KPENTER, KEY_RCTRL, 0, 0,
/*20*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/*30*/	0, 0, 0, 0, 0, KEY_KPSLASH, 0, 0, KEY_RALT, 0, 0, 0, 0, 0, 0, 0,
/*40*/	0, 0, 0, 0, 0, 0, 0, KEY_HOME, KEY_UP, KEY_PGUP, 0, KEY_LEFT, 0, KEY_RIGHT, 0, KEY_END,
/*50*/	KEY_DOWN, KEY_PGDOWN, KEY_INS, KEY_DEL, 0, 0, 0, 0, 0, 0, KEY_WIN, 0, 0, KEY_MENU
};
Uint8	gpKBDUS3[256] = {
//   	0  1  2   3  4   5  6  7   8  9   A  B   C  D   E  F
/*00*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //0-F
/*10*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, KEY_PAUSE
};


Uint8	*gpKBDUS[3] = { gpKBDUS1, gpKBDUS2, gpKBDUS3 };

#endif
