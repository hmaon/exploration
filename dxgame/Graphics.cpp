#include "stdafx.h"
#include "Graphics.h"
#include <DirectXMath.h>

using namespace std;
using namespace DirectX;


Graphics::Graphics(int width, int height, HWND window, HMODULE progInstance)
{
    // D3DClass wraps some of the more tedious aspects of DirectX, we need that:
    d3d.Initialize(width, height, true, window, false, 32, 1.0f);

    // initialize the text renderer with our favorite stupid-looking font
    text.init(d3d.GetDevice(), L"Special Elite");

    // this is the off-screen buffer for drawing
    offScreen = IntermediateRenderTarget(d3d.GetDevice(), d3d.GetDeviceContext(), width, height);
    if (!offScreen.getResourceViewAndResolveMSAA(d3d.GetDeviceContext()))
    {
        Errors::Cry("Couldn't initialize off-screen buffer; something must be really wrong. Do you have a graphics card installed?");
    }

    shaders0.InitializeShader(d3d.GetDevice(), window, L"light_vs.cso",  L"light_ps.cso");

    postProcess.InitializeShader(d3d.GetDevice(), window, L"postprocess_vs.cso", L"postprocess_ps.cso");

    skyBoxShaders.InitializeShader(d3d.GetDevice(), window, L"skybox_vs.cso", L"skybox_ps.cso");

    projection = XMMatrixPerspectiveFovLH((float)((60.0/360.0) * M_PI * 2), (float)width / (float)height, 0.1f, 1000.0f);

    ortho = XMMatrixOrthographicOffCenterLH(0.0f, 1.0f, 0.0f, 1.0f, 0.1f, 1.1f);

    if (!square.load(L"square.obj", d3d.GetDevice()))
    {
        return;
    }

    if (!cube.load(L"sphere.obj", d3d.GetDevice()))
    {
        return;
    }

    FPCamera = make_shared<ScriptedCamera>(); // the camera object is cheap to initialize, redundancy won't hurt 
}

Graphics::Graphics( Graphics & )
{
    Errors::Cry("You may not copy the Graphics object.");
}

Graphics & Graphics::operator=( Graphics & )
{
    Errors::Cry("You may not assign the Graphics object with operator= ! Try using a shared_ptr<>");
    return *this;
}


namespace FPS
{
    const int frameTotal = 16;
    int frame = 0;
    float timeAt[frameTotal];
}

Graphics::~Graphics(void)
{
    // need some kind of container to take care of all this cleanup...
    // perhaps a garbage bag
    shaders0.Shutdown();
    postProcess.Shutdown();
    skyBoxShaders.Shutdown();

    offScreen.Shutdown();

    text.Release();
    text.ReleaseFactory();

    d3d.Shutdown();
}

bool Graphics::RenderScene( Chronometer &timer, Scene *scene)
{
    shared_ptr<LightsAndShadows> lighting = scene->getLights();

    //
    // Rendering
    // 


#ifndef DISABLE_OFFSCREEN_BUFFER
    d3d.BeginScene(false); // don't clear back buffer; we're just going to overwrite it completely from the off-screen buffer
#else
    d3d.BeginScene(true);
#endif

#if 0
    sunlight.x *=2;
    sunlight.y *=2;
    sunlight.z *=2;
#endif


    lighting->updateGPU(d3d.GetDeviceContext(), shaders0, (float)timer.sinceInit(), FPCamera->getEyePosition());

    // pass a render function to the LightsAndShadows object; it will be called once per shadowmap
    if (!lighting->renderShadowMaps(d3d, [&](VanillaShaderClass &shader, CXMMATRIX view, CXMMATRIX projection, bool orthoProjection, std::vector<Light> &lights)
    {
        // body of function called from within lighting

        // pass a render function to scene with the combined data from Graphics and LightsAndShadows
        Scene::renderFunc_t renderFuncForScene = [&] (CXMMATRIX world, shared_ptr<ModelManager> models, int modelRefNum, shared_ptr<LightsAndShadows> lighting, float animationTick) 
        {
            return (*models)[modelRefNum]->render(d3d.GetDeviceContext(), &shaders0, world,  view, projection, lights, orthoProjection, animationTick);
        };

        return scene->render(renderFuncForScene); // this will call the above lambda for all the actor objects
    }
    )) return false;


    //
    // render the scene to off-screen buffer
    // 
    D3D11_VIEWPORT viewportMain = { 0.0f, 0.0f,  (float)Options::intOptions["Width"], (float)Options::intOptions["Height"], 0.0f, 1.0f};
    d3d.GetDeviceContext()->RSSetViewports(1, &viewportMain);

    XMMATRIX view = FPCamera->getViewMatrix();

    shaders0.setVSCameraBuffer(d3d.GetDeviceContext(), FPCamera->getEyePosition(), (float)timer.sinceInit(), 0);


#ifndef DISABLE_OFFSCREEN_BUFFER
    offScreen.setAsRenderTarget(d3d.GetDeviceContext(), d3d.GetDepthStencilView()); // set the off-screen texture as the render target
    offScreen.clear(d3d.GetDeviceContext());
#else
    d3d.setAsRenderTarget(true);
    d3d.depthOn();
#endif

    d3d.setDepthBias(false);
    lighting->setShadowsAsViewResources(d3d);

    Scene::renderFunc_t renderFuncForScene = [&] (CXMMATRIX world, shared_ptr<ModelManager> models, int modelRefNum, shared_ptr<LightsAndShadows> lighting, float animationTick) 
    {
        return (*models)[modelRefNum]->render(d3d.GetDeviceContext(), &shaders0, world,  view, projection, lighting->getLights(), false, animationTick);
    };

    if (!scene->render(renderFuncForScene)) return false; // XXX style: use exceptions instead?

    //
    // Done rendering scene
    //


    //
    // draw sky box if applicable
    //

    shared_ptr<LoadedTexture> skyBox = scene->getSkyBoxTexture();
    if (skyBox)
    {
        d3d.noCullNorBias();
        cube.setBuffers(d3d.GetDeviceContext());
        skyBoxShaders.Render(d3d.GetDeviceContext(), cube.getIndexCount(), XMMatrixIdentity(), view, projection, nullptr, nullptr, nullptr, skyBox->getTexture());
    }


#ifndef DISABLE_OFFSCREEN_BUFFER

    //
    // Copy off-screen buffer to swap chain buffer

    d3d.depthOff(); // disable depth test

    d3d.setAsRenderTarget(false); // set a swap chain buffer as render target again

    square.setBuffers(d3d.GetDeviceContext()); // use the two triangles to render off-screen texture to swap chain, through a shader
    if (!postProcess.Render(d3d.GetDeviceContext(), square.getIndexCount(), XMMatrixIdentity(), XMMatrixIdentity(), ortho, nullptr, nullptr, nullptr, offScreen.getResourceViewAndResolveMSAA(d3d.GetDeviceContext()), 1, false))
    //if (!postProcess.Render(d3d.GetDeviceContext(), square.getIndexCount(), XMMatrixIdentity(), XMMatrixIdentity(), ortho, nullptr, nullptr, nullptr, lighting->getShadows()[NUM_SPOTLIGHTS].getResourceView(d3d.GetDeviceContext()), 1, false)) // comment out previous line and uncomment this one to visually inspect a shadow map
    {
        Errors::Cry(L"Error rendering off-screen texture to display. :/");
        return false;
    }
#endif

    //text.write(d3d.GetDeviceContext(), L"Hello?", 0, 0);
    wchar_t fps[256];

    float tenFramesAgo = FPS::timeAt[FPS::frame % FPS::frameTotal];
    FPS::timeAt[FPS::frame % FPS::frameTotal] = (float)timer.sinceInit();

    if (FPS::frame > FPS::frameTotal - 1)
    {
        StringCbPrintfW(fps, sizeof(fps), L"%.02f fps", ((float)FPS::frameTotal) / (timer.sinceInit() - tenFramesAgo));
        text.write(d3d.GetDeviceContext(), fps, 25, 25);
    }
    ++FPS::frame;

    d3d.depthOn(); // enable depth test again for normal drawing

    d3d.EndScene();

    return true;
}

