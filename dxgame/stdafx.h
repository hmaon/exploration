// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <cstring>

#include <stdio.h>
#include <string.h>
#include <tchar.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <dxgi.h>
#include <d3d11.h>

// mm, obsolete D3DX11 library stuff...
#include <d3dx10math.h>
#include <d3dx11async.h>
//#include <d3dx11Effect.h>

#include <xnamath.h>

#include "d3dclass.h"
#include "LoadedTexture.h"
#include "WICTextureLoader.h"
#include "vertex.h"
#include "Chronometer.h"
#include "Sound.h"
#include "inputclass.h"
#include "FirstPerson.h"
#include "IntermediateRenderTarget.h"
#include "Errors.h"


#include <iostream>
#include <string>
#include <locale>
#include <codecvt>
#include <iomanip>
#include <memory>
#include <vector>
#include <fstream>
#include <map>


// external lib deps:
#include <lua.hpp>
#include <fmod.hpp>
