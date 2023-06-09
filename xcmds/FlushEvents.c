/*
 Dirt simple XCMD that just calls FlushEvents to clear out all keyboard and mouse events.
 Great when paired with GetKeys or GetAnyKey XFCNs to stop any keystrokes entered
 when a script is running from piling up and all firing off on the next idle cycle.
*/

#include "HyperXCmd.h"

pascal void main(XCmdPtr paramPtr)
{
    FlushEvents(mDownMask | mUpMask | keyDownMask | keyUpMask | autoKeyMask, 0);
}
