/*
 XFCN that returns the keycode of the currently pressed key on the keyboard
 If more than one key is pressed, returns whichever has the lowest keycode
*/

#include "HyperXCmd.h"

short firstKeyCode(long keyMap[4])
{
	short i;
	short bit;
	unsigned char *keyMapBytes;
	
	if (keyMap[0] == 0L && keyMap[1] == 0L && keyMap[2] == 0L && keyMap[3] == 0L) {
		return -1;
	}
	
	keyMapBytes = (unsigned char *)keyMap;
	
	for(i = 0; i < 16; ++i) {
		unsigned char byte = keyMapBytes[i];
		
		if (byte == 0) {
			continue;
		}
		
		for(bit = 0; bit < 8; ++bit) {
			if (((byte >> bit) & 1) != 0) {
				return bit + i*8;
			}
		}
	}
	
	return -1;
}

pascal void main(XCmdPtr paramPtr)
{
	long keyMap[4];
	long keyCode = 0;
	Str255 keyCodeStr;
	
	GetKeys(keyMap);
	keyCode = firstKeyCode(keyMap);
	
	if (keyCode == -1) {
		return;
	}
	
	NumToStr(paramPtr, keyCode, keyCodeStr);
	paramPtr->returnValue = PasToZero(paramPtr, keyCodeStr);
}
