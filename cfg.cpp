/***************************************************************************
                           cfg.c  -  description
                             -------------------
    begin                : Sun Mar 08 2009
    copyright            : (C) 1999-2009 by Pete Bernert
    web                  : www.pbernert.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

//*************************************************************************//
// History of changes:
//
// 2009/03/08 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//

#include "stdafx.h"
#include "externals.h"

void ReadConfig(void) // read config (linux file)
{
	iResX = 640;
	iResY = 480;
	bFullVRam = false;
	iFrameTexType = 1;
	iFrameReadType = 0;
	iVRamSize = 0;
	iTexGarbageCollection = 1;

}
