/*
 XFCN that will determine if keys are pressed given keycodes
 Takes one parameter: a string of up to eight comma separated integers, each
 representing a key code
 Returns: a string of comma separated boolean values, each corresponding to the
 key code in the passed in string, where true indicates the key is down

 Example:
 The user is pressing Shift+A

 GetKeys("56,0,12")

 ...returns: "true,true,false"
*/

#include "HyperXCmd.h"

#define kMaxKeyCodeCount 8

Boolean keyIsDown(short keyCode, KeyMap *keyMap)
{
    short byteIndex;
    char theByte, theBit;
    char *thePointer;

    if (keyCode < 0) {
        return false;
    }

    byteIndex = keyCode >> 3;
    thePointer = (char *)(*keyMap);
    theByte = thePointer[byteIndex];
    theBit = 1L << (keyCode & 7);
    return ((theByte & theBit) != 0);
}

pascal void main(XCmdPtr paramPtr)
{
    KeyMap keyMap;
    short keyCodes[kMaxKeyCodeCount];
    long temp;
    int keyCodeCount, i;
    char *input;
    unsigned char numStr[5];
    unsigned char w;
    int inputLength;
    Handle handle;

    if (paramPtr->paramCount != 1 || paramPtr->params[0] == nil) {
        return;
    }

    w = 0;
    keyCodeCount = 0;
    input = *paramPtr->params[0];
    inputLength = StringLength(paramPtr, input);

    for(i = 0; i <= inputLength; ++i) {
        if (w >= 4) {
            return;
        }

        if (i != inputLength && input[i] != ',') {
            numStr[w+1] = input[i];
            ++w;
            continue;
        }

        if (w > 0) {
            numStr[0] = w;
            temp = StrToNum(paramPtr, numStr);

            if (temp < 0 || temp >= 128) {
                return;
            }

            keyCodes[keyCodeCount] = (short)temp;
        } else {
            keyCodes[keyCodeCount] = -1;
        }

        w = 0;
        ++keyCodeCount;

        if (keyCodeCount >= kMaxKeyCodeCount) {
            break;
        }
    }

    GetKeys(keyMap);
    w = 0;
    handle = NewHandle(keyCodeCount*6);

    for(i = 0; i < keyCodeCount; ++i) {
        Boolean down = keyIsDown(keyCodes[i], &keyMap);

        if (i != 0) {
            (*handle)[w++] = ',';
        }

        if (down) {
            (*handle)[w++] = 't';
            (*handle)[w++] = 'r';
            (*handle)[w++] = 'u';
            (*handle)[w++] = 'e';
        } else {
            (*handle)[w++] = 'f';
            (*handle)[w++] = 'a';
            (*handle)[w++] = 'l';
            (*handle)[w++] = 's';
            (*handle)[w++] = 'e';
        }
    }

    (*handle)[w] = '\0';
    paramPtr->returnValue = handle;
}
