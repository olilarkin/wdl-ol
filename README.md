WDL-OL / IPlug

IPlug is a simple-to-use C++ framework for developing cross platform audio plugins and targeting multiple plugin APIs with the same code. Originally developed by Schwa/Cockos, IPlug has been enhanced by various contributors. IPlug depends on WDL, and that is why this project is called WDL-OL,  although most of the differences from Cockos' WDL are to do with IPlug.
This version of IPlug targets VST2, VST3, AudioUnit RTAS and AAX (Native) APIs. It can also produce standalone Windows/macOS audio/midi apps. 

**NOTE: This project is currently going through a massive re-factoring with some great new features coming. "IPlug2" will be released sometime in 2018. In the meantime I have updated this master branch which was very stale and was not compiling with current versions of Xcode and Visual Studio. There are probably a few things not working so well and out of date information, but I would much rather spend my time on IPlug2, which will provide options to maintain backwards compatibility as best as possible.** 

**Much of this work is not exactly fun. If you appreciate having a completely free open source and easy-to-use C++ plug-in framework, free of commercial interest, please consider supporting my efforts financially via my patreon: https://patreon.com/olilarkin (even a token contribution of $1/month means a lot**

This version of WDL/IPlug shares the same license as the Cockos edition. Several of the added features are based on the work of other people. See individual source code files for any extra license information.

Cockos WDL Page: http://www.cockos.com/wdl

Discuss WDL on the WDL forum http://forum.cockos.com/forumdisplay.php?f=32

--------------------------------------------

Cockos WDL License

Copyright (C) 2005 and later Cockos Incorporated

Portions copyright other contributors, see each source file for more information

This software is provided 'as-is', without any express or implied warranty.  In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
1. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
1. This notice may not be removed or altered from any source distribution.

WDL includes the following 3rd party libraries (which are all similarly licensed):

* JNetLib http://www.nullsoft.com/free/jnetlib
* LibPNG http://www.libpng.org/pub/png
* GifLib http://sourceforge.net/projects/libungif
* JPEGLib http://www.ijg.org
* zlib http://www.zlib.net

