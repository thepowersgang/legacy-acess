
#ifndef _KBDUS_H
#define _KBDUS_H

// - BASE (NO PREFIX)
Uint8	gpKBDUS1[256] = {
	//00
	0, KEY_F9, 0, KEY_F5, KEY_F3, KEY_F1, KEY_F2, KEY_F12, 0, KEY_F10, KEY_F8, KEY_F6, KEY_F4, '\t', '`', 0,
	//10
	'\t', KEY_LALT, KEY_LSHIFT, 0, KEY_LCTRL, 'q', '1', 0, 0, 0, 'z', 's', 'a', 'w', '2', KEY_MENU,
	//20
	0, 'c', 'x', 'd', 'e', '4', '3',  0,  0, ' ', 'v', 'f', 't', 'r', '5',  0,
	//30
	0, 'n', 'b', 'h', 'g', 'y', '6',  0,  0,   0, 'm', 'j', 'u', '7', '8',  0,
	//40
	0, ',', 'k', 'i', 'o', '0', '9',  0,  0, '.',   '/', 'l', ';', 'p', '-',  0,
	//50
	0,  0, '\'',   0, '[', '=',   0,  0,  KEY_CAPSLOCK, KEY_RSHIFT, '\n', ']', 0,'\\', 0, 0,
	//60
	0, 0, 0, 0, 0, 0,'\b',  0,  0, KEY_KPEND, 0,KEY_KPLEFT, KEY_KPHOME, 0, 0, 0,
	//70
	KEY_KPINS, KEY_KPDEL, KEY_KPDOWN, KEY_KP5, KEY_KPRIGHT, KEY_KPUP, KEY_ESC,  0, KEY_F11, '+', KEY_KPPGDN, '-', '*', KEY_KPPGUP, 0, 0,
	//80
	0, 0, 0, KEY_F7	
};
// - 0xE0 Prefixed
Uint8	gpKBDUS2[256] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, KEY_RALT, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, KEY_WIN,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, KEY_MENU,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, KEY_KPSLASH, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, KEY_END, 0, KEY_LEFT, KEY_HOME, 0, 0, 0,
	KEY_INS, KEY_DEL, KEY_DOWN, 0, KEY_RIGHT, KEY_UP, 0, 0, 0, 0, KEY_PGDOWN, 0, 0, KEY_PGUP, 0, 0
};
Uint8	gpKBDUS3[256] = {0};

Uint8	*gpKBDUS_2[3] = { gpKBDUS1, gpKBDUS2, gpKBDUS3 };

#endif
