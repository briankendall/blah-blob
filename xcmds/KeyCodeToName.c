/*
 XFCN that converts a key code into its name
 NB: Only includes names for keys in the domestic / US keyboard layout

 Example:
 KeyCodeToName(49)
 Returns: "Space"
*/

#include "HyperXCmd.h"

const unsigned char * keyName(short keyCode)
{
    switch(keyCode) {
        case 0: return "\pA";
        case 1: return "\pS";
        case 2: return "\pD";
        case 3: return "\pF";
        case 4: return "\pH";
        case 5: return "\pG";
        case 6: return "\pZ";
        case 7: return "\pX";
        case 8: return "\pC";
        case 9: return "\pV";
        case 11: return "\pB";
        case 12: return "\pQ";
        case 13: return "\pW";
        case 14: return "\pE";
        case 15: return "\pR";
        case 16: return "\pY";
        case 17: return "\pT";
        case 18: return "\p1";
        case 19: return "\p2";
        case 20: return "\p3";
        case 21: return "\p4";
        case 22: return "\p6";
        case 23: return "\p5";
        case 24: return "\p=";
        case 25: return "\p9";
        case 26: return "\p7";
        case 27: return "\p-";
        case 28: return "\p8";
        case 29: return "\p0";
        case 30: return "\p]";
        case 31: return "\pO";
        case 32: return "\pU";
        case 33: return "\p[";
        case 34: return "\pI";
        case 35: return "\pP";
        case 36: return "\pReturn";
        case 37: return "\pL";
        case 38: return "\pJ";
        case 39: return "\p'";
        case 40: return "\pK";
        case 41: return "\p;";
        case 42: return "\p\\";
        case 43: return "\p,";
        case 44: return "\p/";
        case 45: return "\pN";
        case 46: return "\pM";
        case 47: return "\p.";
        case 48: return "\pTab";
        case 49: return "\pSpace";
        case 50: return "\p`";
        case 51: return "\pBackspace";
        case 53: return "\pEscape";
        case 55: return "\pCommand";
        case 56: return "\pShift";
        case 57: return "\pCapslock";
        case 58: return "\pOption";
        case 59: return "\pControl";
        case 65: return "\pNumpad.";
        case 67: return "\pNumpad*";
        case 69: return "\pNumpad-";
        case 71: return "\pClear";
        case 75: return "\pNumpad/";
        case 76: return "\pEnter";
        case 78: return "\pNumpad+";
        case 81: return "\pNumpad=";
        case 82: return "\pNumpad0";
        case 83: return "\pNumpad1";
        case 84: return "\pNumpad2";
        case 85: return "\pNumpad3";
        case 86: return "\pNumpad4";
        case 87: return "\pNumpad5";
        case 88: return "\pNumpad6";
        case 89: return "\pNumpad7";
        case 91: return "\pNumpad8";
        case 92: return "\pNumpad9";
        case 96: return "\pF5";
        case 97: return "\pF6";
        case 98: return "\pF7";
        case 99: return "\pF3";
        case 100: return "\pF8";
        case 101: return "\pF9";
        case 103: return "\pF11";
        case 105: return "\pF13";
        case 107: return "\pF14";
        case 109: return "\pF10";
        case 111: return "\pF12";
        case 113: return "\pF15";
        case 114: return "\pHelp";
        case 115: return "\pHome";
        case 116: return "\pPage Up";
        case 117: return "\pDel";
        case 118: return "\pF4";
        case 119: return "\pEnd";
        case 120: return "\pF2";
        case 121: return "\pPage Down";
        case 122: return "\pF1";
        case 123: return "\pLeft";
        case 124: return "\pRight";
        case 125: return "\pDown";
        case 126: return "\pUp";
        default:
            return "\pUnknown key!";
    }
}

pascal void main(XCmdPtr paramPtr)
{
    char *input;
    short inputLength;
    short i;
    Str255 temp;
    long keyCodeLong;
    short keyCode;

    if (paramPtr->paramCount != 1 || paramPtr->params[0] == nil) {
        return;
    }

    input = *paramPtr->params[0];
    inputLength = StringLength(paramPtr, input);

    if (inputLength > 3) {
        return;
    }

    temp[0] = (unsigned char)inputLength;

    for(i = 0; i < inputLength; ++i) {
        temp[i+1] = input[i];
    }

    keyCodeLong = StrToNum(paramPtr, temp);

    if (keyCodeLong > 127) {
        return;
    }

    keyCode = (short)keyCodeLong;
    paramPtr->returnValue = PasToZero(paramPtr, (unsigned char *)keyName(keyCode));
}
