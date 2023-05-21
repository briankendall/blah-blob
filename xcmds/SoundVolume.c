/*
 XCMD that returns the system sound volume when no arguments are passed in,
 or sets the volume when a single argument is passed in
*/

#include "HyperXCmd.h"
#include "Sound.h"

void returnSoundVolume(XCmdPtr paramPtr)
{
    Str255 soundVolumeStr;
    short soundVolume;

    GetSoundVol(&soundVolume);

    if (soundVolume < 0) {
        soundVolume = 0;
    } else if (soundVolume > 7) {
        soundVolume = 7;
    }

    NumToStr(paramPtr, (long)soundVolume, soundVolumeStr);
    paramPtr->returnValue = PasToZero(paramPtr, soundVolumeStr);
}

pascal void main(XCmdPtr paramPtr)
{
    char *input;
    short inputLength;
    short newVolume;

    if (paramPtr->paramCount == 0) {
        returnSoundVolume(paramPtr);
        return;
    }

    if (paramPtr->paramCount != 1 || paramPtr->params[0] == nil) {
        return;
    }

    input = *paramPtr->params[0];
    inputLength = StringLength(paramPtr, input);

    if (inputLength > 1) {
        return;
    }

    newVolume = (short)(input[0] - '0');

    if (newVolume < 0 || newVolume > 7) {
        return;
    }

    SetSoundVol(newVolume);
}
