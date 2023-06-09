/*
 XFCN that returns the width and height of the screen in pixels
*/

#include "HyperXCmd.h"

struct QDGlobals {
    char privates[76];
    long randSeed;
    BitMap screenBits;
    Cursor arrow;
    Pattern dkGray;
    Pattern ltGray;
    Pattern gray;
    Pattern black;
    Pattern white;
    GrafPtr thePort;
};
typedef struct QDGlobals QDGlobals;
typedef QDGlobals *QDGlobalsPtr;

pascal void main(XCmdPtr paramPtr)
{
    Str255 output;
    Str255 heightStr;
    short width, height;
    short widthLength, heightLength;
    short i;
    QDGlobalsPtr qdGlobals;

    // How to get access to the QuickDraw globals from an XCMD / XFCN:
    // Apparently the value of SetCurrentA5() will be the address of
    // thePort, and we can use that to find the rest of the QuickDraw
    // global variables
    qdGlobals = (QDGlobalsPtr)(*((Handle)SetCurrentA5()) - sizeof(QDGlobals) + sizeof(thePort));

    width = qdGlobals->screenBits.bounds.right - qdGlobals->screenBits.bounds.left;
    height = qdGlobals->screenBits.bounds.bottom - qdGlobals->screenBits.bounds.top;
    NumToStr(paramPtr, width, output);
    NumToStr(paramPtr, height, heightStr);

    widthLength = output[0];
    heightLength = heightStr[0];
    output[widthLength+1] = ',';

    for(i = 0; i < heightLength; ++i) {
        output[widthLength+2+i] = heightStr[i+1];
    }

    output[0] = widthLength+heightLength+1;

    paramPtr->returnValue = PasToZero(paramPtr, output);
}
