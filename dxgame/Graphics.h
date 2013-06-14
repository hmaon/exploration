#pragma once

#include "sse2aligned.h"
#include <windef.h>
#include <DirectXMath.h>
#include <assimp\Importer.hpp>
#include "d3dclass.h"
#include "FirstPerson.h"
#include "vanillashaderclass.h"
#include "Chronometer.h"
#include "Scene.h"
#include "SimpleText.h"

class Graphics :
    public SSE2Aligned
{
public:

    bool RenderScene( Chronometer &timer, Scene *scene);

    Graphics(int width, int height, HWND window, HMODULE progInstance);
    ~Graphics(void);

    Graphics(Graphics &); // anti-copy constructor
    Graphics &operator=(Graphics &); // anti-assignment operator

    D3DClass &getD3D() { return d3d; }
    FirstPerson &getCamera() { return FPCamera; }

private:
    DirectX::XMMATRIX projection;

    DirectX::XMMATRIX ortho;

    // abstraction over some DirectX initialization stuff:
    D3DClass d3d;

    // text renderer
    SimpleText text;

    // FirstPerson handles input for an FPS view and returns a view matrix or other related data
    FirstPerson FPCamera;

    // Assimp loads almost any 3D format though it's a bit slow sometimes; still useful:
    Assimp::Importer modelImporter; // this object will own the memory allocated for the models it loads; when it's destroyed, memory is automatically deallocated

    // shaders0 holds the shaders for the main scene rendering step
    VanillaShaderClass shaders0;

    // this is the effects shader for rendering from off-screen buffer to swap chain
    VanillaShaderClass postProcess;

    // this holds a buffer to act as a render target from which the image can be copied to the swap chain via a postprocess filter of some sort
    IntermediateRenderTarget offScreen;

    // two triangles to draw offScreen -> swap chain
    SimpleMesh square;
};
