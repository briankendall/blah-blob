# Blah Blob!

![banner image](https://github.com/briankendall/blah-blob/blob/master/banner.png)

Blah Blob! is a short and silly platformer game made in HyperCard for classic Mac OS.

You control the titular Blah Blob as you bounce through twenty levels of increasingly devious obstacles.

**To download the game, check out its [itch.io page](https://bribrikendall.itch.io/blah-blob).**

There you'll find Windows, macOS, and Linux versions that, when launched, will automatically start the game in an emulated environment. Or, if you should happen to have a PowerMac with a G4 processor or better sitting around, you can download the actual HyperCard stack and play it on that provided you have a copy of HyperCard or HyperCard Player of version 2.3 or later. (A G3 also might be fast enough to play the game, but I haven't personally tested this.)

This repo contains all of the source code used to create Blah Blob, including:

- The modified version of Mini vMac 37 beta
- Several original XCMDs and XFCNs
- The actual stack of the game without any protection or changes to the user level

### Modified Mini vMac

In order to make the game as easy and pleasant to play as possible in an emulated environment, I made a few modifications to the source code for Mini vMac 37, which is currently the latest version of the Mini vMac available. While this version is still in beta, it seemed to work perfectly fine for playing Blah Blob without any issues (save for some errors when calculating trigonometric functions which have been worked around in the stack).

Since these modifications might be useful to someone else, I'll try to summarize them here:

1. All the builds of Mini vMac for each platform have been configured using Mini vMac's build configuration tool to emulate a Macintosh II with a 512x342 monochrome display, magnification enabled by default, and the default speed set to "All out". The various projects or Makefiles were then edited by hand to fix compilation issues, add resources, and fix source file paths.

2. For platforms where the application can't modify its disk images, namely macOS (due to that breaking code signatures) and Linux (due to running as a AppImage), the custom build of Mini vMac will automatically copy its two disk images, BlahBlob.dsk and BlahBlobData.dsk, to an appropriate place in the user's home directory if they do not already exist, along with a file indicating the version of Blah Blob that created these images to the same directory.

3. If the version number is lower than the current version, it increases the version number and replaces BlahBlob.dsk. (But not BlahBlobData.dsk! Overwriting that would destroy the user's settings and high scores.)

4. It will look for a Mac ROM file in the Resources directory of the app bundle. (macOS only)

5. When set to full screen, the emulator video will scale to fill the screen rather than staying at its current size.

6. Alt+Enter and Command+F (on macOS) can be used to toggle full screen, since those keystrokes are more conventional that Control+F.

7. The name of the process and titles of the window have been changed to "Blah Blob".

### XCMDs and XFCNs

In the course of developing Blah Blob I wrote a few XCMDs and XFCNs. None of them provide any features that are especially unique, but they were fun to write and made certain features in the game a lot easier.

I've added the C source code to them in this repo, in the xcmds folder, with their line-endings modified to be unix style newlines rather than classic Mac style carriage returns. I've also included a Stuffit Archive containing the original versions of the source files along with their projects and compiled XCMDs / XFCNs. The projects are in Think C 6.0 format, and should compile if you have a copy of Think C 6.0 and the appropriate Mac libraries that should be included with it. Later versions of Think C might work too.

### The HyperCard stack

The original stack of Blah Blob is contained in a Stuffit archive (to preserve its resource fork). It requires HyperCard 2.2 or later to run. Be sure to increase the memory of HyperCard to 3000K, otherwise you'll get memory errors when you try to run the stack! It also requires System 7 or later, and SoundManager 3 or later (in order for the MusicBox XCMD to work).

### Third party software used

Blah Blob makes use of some third party XCMDs and XFCNs which I'll credit here:

- MusicBox XCMD 3.0 by Alex Metcalf
- PlayMOD XFCN 2.0 by Kevin Harris
- QuitTheFinder by Jonathan Abourbih
- Shutdown XCMD by Peter Pawlowski
- StrWidth and FindFolder by Frédéric Rinaldi
- FileExists by Guy Kuo

I technically owe both Alex Metcalf and Kevin Harris 15 USD in shareware fees for my use of these XCMDs! If anyone knows if they're still around and can put me in touch with them, I'd like to pay my fee. Or at the very least thank them for their software. Hopefully appreciation that comes thirty years late will still be appreciated!

The music in Blah Blob is performed using the Sound-Trecker driver by Frank Seide. I do believe I owe him something for the driver as well, be it a shareware fee or, as Frank requests in one of his "read me" documents, a trinket of some kind from my hometown. If he's still out there too I'd love to send him something.

Blah Blob was also created with help from the following classic Mac software:

LightningPaint by Humayun S. Lari    
PlayerPRO by Antoine Rosset    
Symantec Think C    
ResEdit    
...and of course, HyperCard