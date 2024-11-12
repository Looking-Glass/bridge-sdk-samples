/*
 *
 * Copyright 2022 Apple Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <bridge.h>
#include <cassert>

#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#include <Metal/Metal.hpp>
#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>
#include <simd/simd.h>
#include <vector>

static constexpr size_t kInstanceRows = 10;
static constexpr size_t kInstanceColumns = 10;
static constexpr size_t kInstanceDepth = 10;
static constexpr size_t kNumInstances = (kInstanceRows * kInstanceColumns * kInstanceDepth);
static constexpr size_t kMaxFramesInFlight = 3;

using namespace std;
using simd::float3;
using simd::float4;
using simd::float4x4;

#include <cstdlib>
void showDialog(string message, string informativeText) 
{
    string command = "osascript -e 'tell app \"System Events\" to display dialog \"" + message +
                          "\" with title \"" + informativeText + "\" buttons {\"OK\"}'";
    system(command.c_str());
}

struct Dim
{
    unsigned long width  = 0;
    unsigned long height = 0;
};

struct BridgeVersionAndPostfix
{
    unsigned long major = 0;
    unsigned long minor = 0;
    unsigned long build = 0; 
    wstring       postfix;
};

struct Calibration
{
    float center = 0;
    float pitch = 0;
    float slope = 0;
    int width = 0;
    int height = 0;
    float dpi = 0;
    float flip_x = 0;
    int invView = 0;
    float viewcone = 0.0f;
    float fringe = 0.0f;
    int cell_pattern_mode = 0;
    vector<CalibrationSubpixelCell> cells;
};

struct DefaultQuitSettings
{
    float aspect        = 0.0f;
    int   quilt_width   = 0;
    int   quilt_height  = 0;
    int   quilt_columns = 0;
    int   quilt_rows    = 0;
};

struct WindowPos
{
    long x = 0;
    long y = 0;
};

unique_ptr<Controller>      g_controller     = nullptr;
WINDOW_HANDLE               g_wnd            = 0;
unsigned long               g_display_index  = 0;
float                       g_viewcone       = 0.0f;
int                         g_device_type    = 0;
float                       g_aspect         = 0.0f;
int                         g_quilt_width    = 0;
int                         g_quilt_height   = 0;
int                         g_vx             = 0;
int                         g_vy             = 0;
unsigned long               g_output_width   = 800;
unsigned long               g_output_height  = 600;
int                         g_window_width   = 800;
int                         g_window_height  = 600;
int                         g_view_width     = 0;
int                         g_view_height    = 0;
int                         g_invview        = 0;
int                         g_ri             = 0;
int                         g_bi             = 0;
float                       g_tilt           = 0.0f;
float                       g_displayaspect  = 0.0f;
float                       g_fringe         = 0.0f;
float                       g_subp           = 0.0f;
float                       g_pitch          = 0.0f;
float                       g_center         = 0.0f;
WindowPos                   g_window_position;
BridgeVersionAndPostfix     g_active_bridge_version;
vector<unsigned long>       g_displays;
vector<wstring>             g_display_serials;
vector<wstring>             g_display_names;
vector<Dim>                 g_display_dimensions;
vector<int>                 g_display_hw_enums;
vector<Calibration>         g_display_calibrations;
vector<int>                 g_display_viewinvs;
vector<int>                 g_display_ris;
vector<int>                 g_display_bis;
vector<float>               g_display_tilts;
vector<float>               g_display_aspects;
vector<float>               g_display_fringes;
vector<float>               g_display_subps;
vector<float>               g_display_viewcones;
vector<float>               g_display_centers;
vector<float>               g_display_pitches;
vector<DefaultQuitSettings> g_display_default_quilt_settings;
vector<WindowPos>           g_display_window_positions;

void populate_displays()
{
    int lkg_display_count = 0;
    g_controller->GetDisplays(&lkg_display_count, nullptr);

    g_displays.resize(lkg_display_count);
    g_controller->GetDisplays(&lkg_display_count, g_displays.data());

    for (auto it : g_displays)
    {
        {
            wstring serial;
            int serial_count = 0;

            g_controller->GetDeviceSerialForDisplay(it, &serial_count, nullptr);

            serial.resize(serial_count);
            g_controller->GetDeviceSerialForDisplay(it, &serial_count, serial.data());

            g_display_serials.push_back(serial);
        }

        {
            wstring name;
            int name_count = 0;

            g_controller->GetDeviceNameForDisplay(it, &name_count, nullptr);

            name.resize(name_count);
            g_controller->GetDeviceNameForDisplay(it, &name_count, name.data());

            g_display_names.push_back(name);
        }

        {
            Dim dim;
            g_controller->GetDimensionsForDisplay(it, &dim.width, &dim.height);

            g_display_dimensions.push_back(dim);
        }

        {
            int hw_enum = -1;
            g_controller->GetDeviceTypeForDisplay(it, &hw_enum);

            g_display_hw_enums.push_back(hw_enum);
        }

        {
            Calibration calibration;
            int number_of_cells = 0;

            g_controller->GetCalibrationForDisplay(it,
                                                   &calibration.center,
                                                   &calibration.pitch,
                                                   &calibration.slope,
                                                   &calibration.width,
                                                   &calibration.height,
                                                   &calibration.dpi,
                                                   &calibration.flip_x,
                                                   &calibration.invView,
                                                   &calibration.viewcone,
                                                   &calibration.fringe,
                                                   &calibration.cell_pattern_mode,
                                                   &number_of_cells,
                                                   nullptr);

            calibration.cells.resize(number_of_cells);

            g_controller->GetCalibrationForDisplay(it,
                                                   &calibration.center,
                                                   &calibration.pitch,
                                                   &calibration.slope,
                                                   &calibration.width,
                                                   &calibration.height,
                                                   &calibration.dpi,
                                                   &calibration.flip_x,
                                                   &calibration.invView,
                                                   &calibration.viewcone,
                                                   &calibration.fringe,
                                                   &calibration.cell_pattern_mode,
                                                   &number_of_cells,
                                                   calibration.cells.data());

            g_display_calibrations.push_back(calibration);
        }

        {
            int invview;
            g_controller->GetInvViewForDisplay(it, &invview);

            g_display_viewinvs.push_back(invview);
        }

        {
            int ri = 0;
            g_controller->GetRiForDisplay(it, &ri);

            g_display_ris.push_back(ri);
        }

        {
            int bi = 0;
            g_controller->GetBiForDisplay(it, &bi);

            g_display_bis.push_back(bi);
        }

        {
            float tilt = 0.0f;
            g_controller->GetTiltForDisplay(it, &tilt);

            g_display_tilts.push_back(tilt);
        }

        {
            float aspect = 0.0f;
            g_controller->GetDisplayAspectForDisplay(it, &aspect);

            g_display_aspects.push_back(aspect);
        }

        {
            float fringe = 0.0f;
            g_controller->GetFringeForDisplay(it, &fringe);

            g_display_fringes.push_back(fringe);
        }

        {
            float subp = 0.0f;
            g_controller->GetSubpForDisplay(it, &subp);

            g_display_subps.push_back(subp);
        }

        {
            float viewcone = 0.0f;
            g_controller->GetViewConeForDisplay(it, &viewcone);

            g_display_viewcones.push_back(viewcone);
        }

        {
            float center = 0.0f;
            g_controller->GetCenterForDisplay(it, &center);

            g_display_centers.push_back(center);
        }

        {
            float pitch = 0.0f;
            g_controller->GetPitchForDisplay(it, &pitch);

            g_display_pitches.push_back(pitch);
        }

        {
            DefaultQuitSettings quilt_settings;
            g_controller->GetDefaultQuiltSettingsForDisplay(it, &quilt_settings.aspect, &quilt_settings.quilt_width, &quilt_settings.quilt_height, &quilt_settings.quilt_columns, &quilt_settings.quilt_rows);

            g_display_default_quilt_settings.push_back(quilt_settings);
        }

        {
            WindowPos pos;
            g_controller->GetWindowPositionForDisplay(it, &pos.x, &pos.y);

            g_display_window_positions.push_back(pos);
        }
    }
}

void populate_window_values()
{
    g_controller->GetDisplayForWindow(g_wnd, &g_display_index);
    g_controller->GetDeviceType(g_wnd, &g_device_type);
    g_controller->GetDefaultQuiltSettings(g_wnd, &g_aspect, &g_quilt_width, &g_quilt_height, &g_vx, &g_vy);
    g_controller->GetWindowDimensions(g_wnd, &g_output_width, &g_output_height);
    g_controller->GetViewCone(g_wnd, &g_viewcone);
    g_controller->GetInvView(g_wnd, &g_invview);
    g_controller->GetRi(g_wnd, &g_ri);
    g_controller->GetBi(g_wnd, &g_bi);
    g_controller->GetTilt(g_wnd, &g_tilt);
    g_controller->GetDisplayAspect(g_wnd, &g_displayaspect);
    g_controller->GetFringe(g_wnd, &g_fringe);
    g_controller->GetSubp(g_wnd, &g_subp);
    g_controller->GetPitch(g_wnd, &g_pitch);
    g_controller->GetCenter(g_wnd, &g_center);
    g_controller->GetWindowPosition(g_wnd, &g_window_position.x, &g_window_position.y);
}

void populate_active_bridge_version()
{
    int number_of_postfix_wchars = 0;

    g_controller->GetBridgeVersion(&g_active_bridge_version.major,
                                   &g_active_bridge_version.minor,
                                   &g_active_bridge_version.build,
                                   &number_of_postfix_wchars,
                                   nullptr);

    g_active_bridge_version.postfix.resize(number_of_postfix_wchars);

    g_controller->GetBridgeVersion(&g_active_bridge_version.major,
                                   &g_active_bridge_version.minor,
                                   &g_active_bridge_version.build,
                                   &number_of_postfix_wchars,
                                   g_active_bridge_version.postfix.data());

}

void initBridge(MTL::Device* pDevice)
{
    g_controller = make_unique<Controller>();

    if (g_controller->Initialize("BridgeSDKSampleNative"))
    {
        populate_active_bridge_version();
        populate_displays();
 
        if (!g_displays.empty())
        if (g_controller->InstanceOffscreenWindowMetal(pDevice, &g_wnd))
        {
            populate_window_values();

            wstring name;
            int     name_size = 0;
            g_controller->GetDeviceName(g_wnd, &name_size, nullptr);

            name.resize(name_size);
            g_controller->GetDeviceName(g_wnd, &name_size, name.data());

            wstring serial;
            int     serial_size = 0;

            g_controller->GetDeviceSerial(g_wnd, &serial_size, nullptr);

            serial.resize(serial_size);
            g_controller->GetDeviceSerial(g_wnd, &serial_size, serial.data());
        }
    }

    if (g_wnd == 0)
    {
        // mlc: couldn't init bridge, run desktop head only
        g_controller = nullptr;
    }
    else
    {
        // mlc: bridge inited, finish setup
        g_view_width  = int(float(g_quilt_width) / float(g_vx));
        g_view_height = int(float(g_quilt_height) / float(g_vy));
    }
}

#pragma region Declarations {

namespace math
{
    constexpr simd::float3 add( const simd::float3& a, const simd::float3& b );
    constexpr simd_float4x4 makeIdentity();
    simd::float4x4 makePerspective();
    simd::float4x4 makeXRotate( float angleRadians );
    simd::float4x4 makeYRotate( float angleRadians );
    simd::float4x4 makeZRotate( float angleRadians );
    simd::float4x4 makeTranslate( const simd::float3& v );
    simd::float4x4 makeScale( const simd::float3& v );
    simd::float3x3 discardTranslation( const simd::float4x4& m );
}

class Renderer
{
    public:
        Renderer(MTL::Device* pDevice);
        ~Renderer();
        void buildShaders();
        void buildDepthStencilStates();
        void buildBuffers();
        void draw(MTK::View* pView);
        void drawInternal(MTL::Buffer* pInstanceDataBuffer, MTL::RenderCommandEncoder* pOffscreenEncoder, float tx = 0.0f);
        void buildOffscreenRenderTarget();

    private:
        MTL::Device* _pDevice;
        MTL::CommandQueue* _pCommandQueue;
        MTL::Library* _pShaderLibrary;
        MTL::RenderPipelineState* _pPSO;
        MTL::DepthStencilState* _pDepthStencilState;
        MTL::Buffer* _pVertexDataBuffer;
        MTL::Buffer* _pInstanceDataBuffer[kMaxFramesInFlight];
        MTL::Buffer* _pCameraDataBuffer[kMaxFramesInFlight];
        MTL::Buffer* _pIndexBuffer;
        MTL::Texture* _pOffscreenRenderTarget;
        MTL::Texture* _pDepthTexture;
        MTL::Buffer* _pQuadVertexBuffer;
        MTL::Buffer* _pQuadIndexBuffer;
        MTL::RenderPipelineState* _pQuadPSO;
        float _angle;
        int _frame;
        dispatch_semaphore_t _semaphore;
        static const int kMaxFramesInFlight;
};

class MyMTKViewDelegate : public MTK::ViewDelegate
{
    public:
        MyMTKViewDelegate( MTL::Device* pDevice );
        virtual ~MyMTKViewDelegate() override;
        virtual void drawInMTKView( MTK::View* pView ) override;

    private:
        Renderer* _pRenderer;
};

class MyAppDelegate : public NS::ApplicationDelegate
{
    public:
        ~MyAppDelegate();

        NS::Menu* createMenuBar();

        virtual void applicationWillFinishLaunching( NS::Notification* pNotification ) override;
        virtual void applicationDidFinishLaunching( NS::Notification* pNotification ) override;
        virtual bool applicationShouldTerminateAfterLastWindowClosed( NS::Application* pSender ) override;

    private:
        NS::Window* _pWindow;
        MTK::View* _pMtkView;
        MTL::Device* _pDevice;
        MyMTKViewDelegate* _pViewDelegate = nullptr;
};

#pragma endregion Declarations }


int main( int argc, char* argv[] )
{
    NS::AutoreleasePool* pAutoreleasePool = NS::AutoreleasePool::alloc()->init();

    MyAppDelegate del;

    NS::Application* pSharedApplication = NS::Application::sharedApplication();
    pSharedApplication->setDelegate( &del );
    pSharedApplication->run();

    pAutoreleasePool->release();

    return 0;
}


#pragma mark - AppDelegate
#pragma region AppDelegate {

MyAppDelegate::~MyAppDelegate()
{
    _pMtkView->release();
    _pWindow->release();
    _pDevice->release();
    delete _pViewDelegate;
}

NS::Menu* MyAppDelegate::createMenuBar()
{
    using NS::StringEncoding::UTF8StringEncoding;

    NS::Menu* pMainMenu = NS::Menu::alloc()->init();
    NS::MenuItem* pAppMenuItem = NS::MenuItem::alloc()->init();
    NS::Menu* pAppMenu = NS::Menu::alloc()->init( NS::String::string( "Appname", UTF8StringEncoding ) );

    NS::String* appName = NS::RunningApplication::currentApplication()->localizedName();
    NS::String* quitItemName = NS::String::string( "Quit ", UTF8StringEncoding )->stringByAppendingString( appName );
    SEL quitCb = NS::MenuItem::registerActionCallback( "appQuit", [](void*,SEL,const NS::Object* pSender){
        auto pApp = NS::Application::sharedApplication();
        pApp->terminate( pSender );
    } );

    NS::MenuItem* pAppQuitItem = pAppMenu->addItem( quitItemName, quitCb, NS::String::string( "q", UTF8StringEncoding ) );
    pAppQuitItem->setKeyEquivalentModifierMask( NS::EventModifierFlagCommand );
    pAppMenuItem->setSubmenu( pAppMenu );

    NS::MenuItem* pWindowMenuItem = NS::MenuItem::alloc()->init();
    NS::Menu* pWindowMenu = NS::Menu::alloc()->init( NS::String::string( "Window", UTF8StringEncoding ) );

    SEL closeWindowCb = NS::MenuItem::registerActionCallback( "windowClose", [](void*, SEL, const NS::Object*){
        auto pApp = NS::Application::sharedApplication();
            pApp->windows()->object< NS::Window >(0)->close();
    } );
    NS::MenuItem* pCloseWindowItem = pWindowMenu->addItem( NS::String::string( "Close Window", UTF8StringEncoding ), closeWindowCb, NS::String::string( "w", UTF8StringEncoding ) );
    pCloseWindowItem->setKeyEquivalentModifierMask( NS::EventModifierFlagCommand );

    pWindowMenuItem->setSubmenu( pWindowMenu );

    pMainMenu->addItem( pAppMenuItem );
    pMainMenu->addItem( pWindowMenuItem );

    pAppMenuItem->release();
    pWindowMenuItem->release();
    pAppMenu->release();
    pWindowMenu->release();

    return pMainMenu->autorelease();
}

void MyAppDelegate::applicationWillFinishLaunching( NS::Notification* pNotification )
{
    NS::Menu* pMenu = createMenuBar();
    NS::Application* pApp = reinterpret_cast< NS::Application* >( pNotification->object() );
    pApp->setMainMenu( pMenu );
    pApp->setActivationPolicy( NS::ActivationPolicy::ActivationPolicyRegular );
}

void MyAppDelegate::applicationDidFinishLaunching( NS::Notification* pNotification )
{
    CGRect frame = (CGRect){ {100.0, 100.0}, {512.0, 512.0} };

    _pWindow = NS::Window::alloc()->init(
        frame,
        NS::WindowStyleMaskClosable|NS::WindowStyleMaskTitled|NS::WindowStyleMaskResizable,
        NS::BackingStoreBuffered,
        false );

    _pDevice = MTL::CreateSystemDefaultDevice();
 
    initBridge(_pDevice);

    _pMtkView = MTK::View::alloc()->init( frame, _pDevice );
    _pMtkView->setColorPixelFormat( MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB );
    _pMtkView->setClearColor( MTL::ClearColor::Make( 1.0, 0.0, 0.0, 1.0 ) );

    _pViewDelegate = new MyMTKViewDelegate( _pDevice );
    _pMtkView->setDelegate( _pViewDelegate ); 

    _pWindow->setContentView( _pMtkView );
    _pWindow->setTitle( NS::String::string( "BridgeSDKSampleMetalNativeInteractive", NS::StringEncoding::UTF8StringEncoding ) );

    if (!g_controller)
    {
        _pWindow->makeKeyAndOrderFront( nullptr );

        NS::Application* pApp = reinterpret_cast< NS::Application* >( pNotification->object() );
        pApp->activateIgnoringOtherApps( true );
    }
    else
    {
        CGRect frame_device = (CGRect){
            {(CGFloat)g_window_position.x, (CGFloat)g_window_position.y}, 
            {(CGFloat)g_output_width, (CGFloat)g_output_height}
        };

        _pWindow->setFrame(frame_device, true);

        _pWindow->makeKeyAndOrderFront( nullptr );

        NS::Application* pApp = reinterpret_cast< NS::Application* >( pNotification->object() );
        pApp->activateIgnoringOtherApps( true );

        _pWindow->toggleFullScreen(nullptr);
    }
}

bool MyAppDelegate::applicationShouldTerminateAfterLastWindowClosed( NS::Application* pSender )
{
    return true;
}

#pragma endregion AppDelegate }


#pragma mark - ViewDelegate
#pragma region ViewDelegate {

MyMTKViewDelegate::MyMTKViewDelegate( MTL::Device* pDevice )
: MTK::ViewDelegate()
, _pRenderer( new Renderer( pDevice ) )
{
}

MyMTKViewDelegate::~MyMTKViewDelegate()
{
    delete _pRenderer;
}

void MyMTKViewDelegate::drawInMTKView( MTK::View* pView )
{
    _pRenderer->draw( pView );
}

#pragma endregion ViewDelegate }


#pragma mark - Math

namespace math
{
    constexpr simd::float3 add( const simd::float3& a, const simd::float3& b )
    {
        return { a.x + b.x, a.y + b.y, a.z + b.z };
    }

    constexpr simd_float4x4 makeIdentity()
    {
        using simd::float4;
        return (simd_float4x4){ (float4){ 1.f, 0.f, 0.f, 0.f },
                                (float4){ 0.f, 1.f, 0.f, 0.f },
                                (float4){ 0.f, 0.f, 1.f, 0.f },
                                (float4){ 0.f, 0.f, 0.f, 1.f } };
    }

    simd::float4x4 makePerspective( float fovRadians, float aspect, float znear, float zfar )
    {
        using simd::float4;
        float ys = 1.f / tanf(fovRadians * 0.5f);
        float xs = ys / aspect;
        float zs = zfar / ( znear - zfar );
        return simd_matrix_from_rows((float4){ xs, 0.0f, 0.0f, 0.0f },
                                     (float4){ 0.0f, ys, 0.0f, 0.0f },
                                     (float4){ 0.0f, 0.0f, zs, znear * zs },
                                     (float4){ 0, 0, -1, 0 });
    }

    simd::float4x4 makeXRotate( float angleRadians )
    {
        using simd::float4;
        const float a = angleRadians;
        return simd_matrix_from_rows((float4){ 1.0f, 0.0f, 0.0f, 0.0f },
                                     (float4){ 0.0f, cosf( a ), sinf( a ), 0.0f },
                                     (float4){ 0.0f, -sinf( a ), cosf( a ), 0.0f },
                                     (float4){ 0.0f, 0.0f, 0.0f, 1.0f });
    }

    simd::float4x4 makeYRotate( float angleRadians )
    {
        using simd::float4;
        const float a = angleRadians;
        return simd_matrix_from_rows((float4){ cosf( a ), 0.0f, sinf( a ), 0.0f },
                                     (float4){ 0.0f, 1.0f, 0.0f, 0.0f },
                                     (float4){ -sinf( a ), 0.0f, cosf( a ), 0.0f },
                                     (float4){ 0.0f, 0.0f, 0.0f, 1.0f });
    }

    simd::float4x4 makeZRotate( float angleRadians )
    {
        using simd::float4;
        const float a = angleRadians;
        return simd_matrix_from_rows((float4){ cosf( a ), sinf( a ), 0.0f, 0.0f },
                                     (float4){ -sinf( a ), cosf( a ), 0.0f, 0.0f },
                                     (float4){ 0.0f, 0.0f, 1.0f, 0.0f },
                                     (float4){ 0.0f, 0.0f, 0.0f, 1.0f });
    }

    simd::float4x4 makeTranslate( const simd::float3& v )
    {
        using simd::float4;
        const float4 col0 = { 1.0f, 0.0f, 0.0f, 0.0f };
        const float4 col1 = { 0.0f, 1.0f, 0.0f, 0.0f };
        const float4 col2 = { 0.0f, 0.0f, 1.0f, 0.0f };
        const float4 col3 = { v.x, v.y, v.z, 1.0f };
        return simd_matrix( col0, col1, col2, col3 );
    }

    simd::float4x4 makeScale( const simd::float3& v )
    {
        using simd::float4;
        return simd_matrix((float4){ v.x, 0, 0, 0 },
                           (float4){ 0, v.y, 0, 0 },
                           (float4){ 0, 0, v.z, 0 },
                           (float4){ 0, 0, 0, 1.0 });
    }

    simd::float3x3 discardTranslation( const simd::float4x4& m )
    {
        return simd_matrix( m.columns[0].xyz, m.columns[1].xyz, m.columns[2].xyz );
    }

}


#pragma mark - Renderer
#pragma region Renderer {

const int Renderer::kMaxFramesInFlight = 3;

Renderer::Renderer( MTL::Device* pDevice )
: _pDevice( pDevice->retain() )
, _angle ( 0.f )
, _frame( 0 )
{
    _pCommandQueue = _pDevice->newCommandQueue();

    buildShaders();
    buildDepthStencilStates();
    buildBuffers();
    buildOffscreenRenderTarget();

    _semaphore = dispatch_semaphore_create( Renderer::kMaxFramesInFlight );
}

Renderer::~Renderer()
{
    _pQuadPSO->release();
    _pQuadVertexBuffer->release();
    _pQuadIndexBuffer->release();
    _pOffscreenRenderTarget->release();
    _pDepthTexture->release();
    _pShaderLibrary->release();
    _pDepthStencilState->release();
    _pVertexDataBuffer->release();
    for ( int i = 0; i < kMaxFramesInFlight; ++i )
    {
        _pInstanceDataBuffer[i]->release();
    }
    for ( int i = 0; i < kMaxFramesInFlight; ++i )
    {
        _pCameraDataBuffer[i]->release();
    }
    _pIndexBuffer->release();
    _pPSO->release();
    _pCommandQueue->release();
    _pDevice->release();
}

namespace shader_types
{
    struct VertexData
    {
        simd::float3 position;
        simd::float3 normal;
    };

    struct InstanceData
    {
        simd::float4x4 instanceTransform;
        simd::float3x3 instanceNormalTransform;
        simd::float4 instanceColor;
    };

    struct CameraData
    {
        simd::float4x4 perspectiveTransform;
        simd::float4x4 worldTransform;
        simd::float3x3 worldNormalTransform;
    };
}

void Renderer::buildOffscreenRenderTarget()
{
    auto width  = g_quilt_width;
    auto height = g_quilt_height;

    if (width == 0)
        width = 4096;

    if (height == 0)
        height = 4096;

    // Create the color render target
    MTL::TextureDescriptor* pColorTextureDesc = MTL::TextureDescriptor::alloc()->init();
    pColorTextureDesc->setPixelFormat(MTL::PixelFormat::PixelFormatRGBA8Unorm);
    pColorTextureDesc->setWidth(width);
    pColorTextureDesc->setHeight(height);
    pColorTextureDesc->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);
    pColorTextureDesc->setStorageMode(MTL::StorageModePrivate);

    if (!g_controller)
    {
        _pOffscreenRenderTarget = _pDevice->newTexture(pColorTextureDesc);
    }
    else
    {
        // mlc: if we are interop'ing with bridge then we must use a rendertarget backed by an IO surface
        g_controller->CreateMetalTextureWithIOSurface(g_wnd, pColorTextureDesc, (void**)&_pOffscreenRenderTarget);
    }

    pColorTextureDesc->release();

    // Create the depth render target
    MTL::TextureDescriptor* pDepthTextureDesc = MTL::TextureDescriptor::alloc()->init();
    pDepthTextureDesc->setPixelFormat(MTL::PixelFormatDepth32Float);
    pDepthTextureDesc->setWidth(4096);
    pDepthTextureDesc->setHeight(4096);
    pDepthTextureDesc->setUsage(MTL::TextureUsageRenderTarget);
    pDepthTextureDesc->setStorageMode(MTL::StorageModePrivate);

    _pDepthTexture = _pDevice->newTexture(pDepthTextureDesc);
    pDepthTextureDesc->release();
}


void Renderer::buildShaders()
{
    using NS::StringEncoding::UTF8StringEncoding;

    const char* sceneShaderSrc = R"(
        #include <metal_stdlib>
        using namespace metal;

        struct v2f
        {
            float4 position [[position]];
            float3 normal;
            float3 color;
        };

        struct VertexData
        {
            float3 position;
            float3 normal;
        };

        struct InstanceData
        {
            float4x4 instanceTransform;
            float3x3 instanceNormalTransform;
            float4 instanceColor;
        };

        struct CameraData
        {
            float4x4 perspectiveTransform;
            float4x4 worldTransform;
            float3x3 worldNormalTransform;
        };

        v2f vertex vertexMain( device const VertexData* vertexData [[buffer(0)]],
                               device const InstanceData* instanceData [[buffer(1)]],
                               device const CameraData& cameraData [[buffer(2)]],
                               uint vertexId [[vertex_id]],
                               uint instanceId [[instance_id]] )
        {
            v2f o;

            const device VertexData& vd = vertexData[vertexId];
            float4 pos = float4(vd.position, 1.0);
            pos = instanceData[instanceId].instanceTransform * pos;
            pos = cameraData.perspectiveTransform * cameraData.worldTransform * pos;
            o.position = pos;

            float3 normal = instanceData[instanceId].instanceNormalTransform * vd.normal;
            normal = cameraData.worldNormalTransform * normal;
            o.normal = normal;

            o.color = instanceData[instanceId].instanceColor.rgb;
            return o;
        }

        fragment float4 fragmentMain( v2f in [[stage_in]] )
        {
            // assume light coming from (front-top-right)
            float3 l = normalize(float3(1.0, 1.0, 0.8));
            float3 n = normalize(in.normal);

            float ndotl = saturate(dot(n, l));
            return float4(in.color * 0.1 + in.color * ndotl, 1.0);
        }
    )";

    const char* quadShaderSrc = R"(
        #include <metal_stdlib>
        using namespace metal;

        struct v2f
        {
            float4 position [[position]];
            float2 uv;
        };

        vertex v2f fullscreenQuadVertex(uint vertexID [[vertex_id]],
                                const device float4* vertexData [[buffer(0)]])
        {
            v2f o;
            float2 ndcPos = vertexData[vertexID].xy;   // NDC coordinates (x, y)
            o.position = float4(ndcPos, 0.0, 1.0);     // Convert to 4D position with z = 0 and w = 1
            o.uv = vertexData[vertexID].zw;            // Read UVs from the vertex data
            return o;
        }

        fragment float4 fullscreenQuadFragment(v2f in [[stage_in]],
                                               texture2d<float> renderTargetTexture [[texture(0)]])
        {
            constexpr sampler quadSampler(filter::linear, address::clamp_to_edge);
            float4 color = renderTargetTexture.sample(quadSampler, in.uv);
            return color;
        }
    )";

    // Compile scene shaders
    NS::Error* pError = nullptr;
    MTL::Library* pLibrary = _pDevice->newLibrary(NS::String::string(sceneShaderSrc, UTF8StringEncoding), nullptr, &pError);
    if (!pLibrary)
    {
        __builtin_printf("%s", pError->localizedDescription()->utf8String());
        assert(false);
    }

    MTL::Function* pVertexFn = pLibrary->newFunction(NS::String::string("vertexMain", UTF8StringEncoding));
    MTL::Function* pFragFn = pLibrary->newFunction(NS::String::string("fragmentMain", UTF8StringEncoding));

    MTL::RenderPipelineDescriptor* pDesc = MTL::RenderPipelineDescriptor::alloc()->init();
    pDesc->setVertexFunction(pVertexFn);
    pDesc->setFragmentFunction(pFragFn);
    pDesc->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm);
    pDesc->setDepthAttachmentPixelFormat(MTL::PixelFormat::PixelFormatDepth16Unorm);

    _pPSO = _pDevice->newRenderPipelineState(pDesc, &pError);
    if (!_pPSO)
    {
        __builtin_printf("%s", pError->localizedDescription()->utf8String());
        assert(false);
    }

    pVertexFn->release();
    pFragFn->release();
    pDesc->release();
    _pShaderLibrary = pLibrary;

    // Compile full-screen quad shaders
    MTL::Library* pQuadLibrary = _pDevice->newLibrary(NS::String::string(quadShaderSrc, UTF8StringEncoding), nullptr, &pError);
    if (!pQuadLibrary)
    {
        __builtin_printf("%s", pError->localizedDescription()->utf8String());
        assert(false);
    }

    MTL::Function* pQuadVertexFn = pQuadLibrary->newFunction(NS::String::string("fullscreenQuadVertex", UTF8StringEncoding));
    MTL::Function* pQuadFragFn = pQuadLibrary->newFunction(NS::String::string("fullscreenQuadFragment", UTF8StringEncoding));

    MTL::RenderPipelineDescriptor* pQuadDesc = MTL::RenderPipelineDescriptor::alloc()->init();
    pQuadDesc->setVertexFunction(pQuadVertexFn);
    pQuadDesc->setFragmentFunction(pQuadFragFn);
    pQuadDesc->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm);

    _pQuadPSO = _pDevice->newRenderPipelineState(pQuadDesc, &pError);
    if (!_pQuadPSO)
    {
        __builtin_printf("%s", pError->localizedDescription()->utf8String());
        assert(false);
    }

    pQuadVertexFn->release();
    pQuadFragFn->release();
    pQuadDesc->release();
    pQuadLibrary->release();
}

void Renderer::buildDepthStencilStates()
{
    // Ensure that the depth comparison function is correct
    MTL::DepthStencilDescriptor* pDsDesc = MTL::DepthStencilDescriptor::alloc()->init();
    pDsDesc->setDepthCompareFunction(MTL::CompareFunctionLess);
    pDsDesc->setDepthWriteEnabled(true);

    _pDepthStencilState = _pDevice->newDepthStencilState(pDsDesc);
    pDsDesc->release();
}

void Renderer::buildBuffers()
{
    using simd::float3;
    const float s = 0.5f;

    shader_types::VertexData verts[] = {
        //   Positions          Normals
        { { -s, -s, +s }, { 0.f,  0.f,  1.f } },
        { { +s, -s, +s }, { 0.f,  0.f,  1.f } },
        { { +s, +s, +s }, { 0.f,  0.f,  1.f } },
        { { -s, +s, +s }, { 0.f,  0.f,  1.f } },

        { { +s, -s, +s }, { 1.f,  0.f,  0.f } },
        { { +s, -s, -s }, { 1.f,  0.f,  0.f } },
        { { +s, +s, -s }, { 1.f,  0.f,  0.f } },
        { { +s, +s, +s }, { 1.f,  0.f,  0.f } },

        { { +s, -s, -s }, { 0.f,  0.f, -1.f } },
        { { -s, -s, -s }, { 0.f,  0.f, -1.f } },
        { { -s, +s, -s }, { 0.f,  0.f, -1.f } },
        { { +s, +s, -s }, { 0.f,  0.f, -1.f } },

        { { -s, -s, -s }, { -1.f, 0.f,  0.f } },
        { { -s, -s, +s }, { -1.f, 0.f,  0.f } },
        { { -s, +s, +s }, { -1.f, 0.f,  0.f } },
        { { -s, +s, -s }, { -1.f, 0.f,  0.f } },

        { { -s, +s, +s }, { 0.f,  1.f,  0.f } },
        { { +s, +s, +s }, { 0.f,  1.f,  0.f } },
        { { +s, +s, -s }, { 0.f,  1.f,  0.f } },
        { { -s, +s, -s }, { 0.f,  1.f,  0.f } },

        { { -s, -s, -s }, { 0.f, -1.f,  0.f } },
        { { +s, -s, -s }, { 0.f, -1.f,  0.f } },
        { { +s, -s, +s }, { 0.f, -1.f,  0.f } },
        { { -s, -s, +s }, { 0.f, -1.f,  0.f } },
    };

    uint16_t indices[] = {
         0,  1,  2,  2,  3,  0, /* front */
         4,  5,  6,  6,  7,  4, /* right */
         8,  9, 10, 10, 11,  8, /* back */
        12, 13, 14, 14, 15, 12, /* left */
        16, 17, 18, 18, 19, 16, /* top */
        20, 21, 22, 22, 23, 20, /* bottom */
    };

    const size_t vertexDataSize = sizeof( verts );
    const size_t indexDataSize = sizeof( indices );

    MTL::Buffer* pVertexBuffer = _pDevice->newBuffer( vertexDataSize, MTL::ResourceStorageModeManaged );
    MTL::Buffer* pIndexBuffer = _pDevice->newBuffer( indexDataSize, MTL::ResourceStorageModeManaged );

    _pVertexDataBuffer = pVertexBuffer;
    _pIndexBuffer = pIndexBuffer;

    memcpy( _pVertexDataBuffer->contents(), verts, vertexDataSize );
    memcpy( _pIndexBuffer->contents(), indices, indexDataSize );

    _pVertexDataBuffer->didModifyRange( NS::Range::Make( 0, _pVertexDataBuffer->length() ) );
    _pIndexBuffer->didModifyRange( NS::Range::Make( 0, _pIndexBuffer->length() ) );

    const size_t instanceDataSize = kMaxFramesInFlight * kNumInstances * sizeof( shader_types::InstanceData );
    for ( size_t i = 0; i < kMaxFramesInFlight; ++i )
    {
        _pInstanceDataBuffer[ i ] = _pDevice->newBuffer( instanceDataSize, MTL::ResourceStorageModeManaged );
    }

    const size_t cameraDataSize = kMaxFramesInFlight * sizeof( shader_types::CameraData );
    for ( size_t i = 0; i < kMaxFramesInFlight; ++i )
    {
        _pCameraDataBuffer[ i ] = _pDevice->newBuffer( cameraDataSize, MTL::ResourceStorageModeManaged );
    }

    const float quadVertices[] = {
        //   NDC positions   |   UV coordinates
        -1.0f, -1.0f,         0.0f, 1.0f,   // Bottom-left
        1.0f, -1.0f,         1.0f, 1.0f,   // Bottom-right
        -1.0f,  1.0f,         0.0f, 0.0f,   // Top-left
        1.0f,  1.0f,         1.0f, 0.0f    // Top-right
    };

    _pQuadVertexBuffer = _pDevice->newBuffer(sizeof(quadVertices), MTL::ResourceStorageModeManaged);
    memcpy(_pQuadVertexBuffer->contents(), quadVertices, sizeof(quadVertices));
    _pQuadVertexBuffer->didModifyRange(NS::Range::Make(0, sizeof(quadVertices)));

    // Define the indices for the full-screen quad (two triangles)
    const uint16_t quadIndices[] = {
        0, 1, 2,  // First triangle (bottom-left)
        1, 3, 2   // Second triangle (top-right)
    };

    // Create the index buffer for the full-screen quad
    _pQuadIndexBuffer = _pDevice->newBuffer(sizeof(quadIndices), MTL::ResourceStorageModeManaged);
    memcpy(_pQuadIndexBuffer->contents(), quadIndices, sizeof(quadIndices));
    _pQuadIndexBuffer->didModifyRange(NS::Range::Make(0, sizeof(quadIndices)));
}

void Renderer::drawInternal(MTL::Buffer* pInstanceDataBuffer, MTL::RenderCommandEncoder* pOffscreenEncoder, float tx)
{
    // Update instance positions
    const float scl = 0.2f;
    shader_types::InstanceData* pInstanceData = reinterpret_cast<shader_types::InstanceData*>(pInstanceDataBuffer->contents());

    float3 objectPosition = {0.f, 0.f, -10.f};
    float3 txOffset = {tx, 0.0f, 0.0f};
    float4x4 rt = math::makeTranslate(objectPosition) * math::makeTranslate(txOffset);
    float4x4 rr1 = math::makeYRotate(-_angle);
    float4x4 rr0 = math::makeXRotate(_angle * 0.5f);
    float4x4 rtInv = math::makeTranslate({-objectPosition.x, -objectPosition.y, -objectPosition.z});
    float4x4 fullObjectRot = rt * rr1 * rr0 * rtInv;

    size_t ix = 0;
    size_t iy = 0;
    size_t iz = 0;
    for (size_t i = 0; i < kNumInstances; ++i) {
        if (ix == kInstanceRows) {
            ix = 0;
            iy += 1;
        }
        if (iy == kInstanceColumns) {
            iy = 0;
            iz += 1;
        }

        float4x4 scale = math::makeScale((float3){scl, scl, scl});
        float4x4 zrot = math::makeZRotate(_angle * sinf((float)ix));
        float4x4 yrot = math::makeYRotate(_angle * cosf((float)iy));

        float x = ((float)ix - (float)kInstanceRows / 2.f) * (2.f * scl) + scl;
        float y = ((float)iy - (float)kInstanceColumns / 2.f) * (2.f * scl) + scl;
        float z = ((float)iz - (float)kInstanceDepth / 2.f) * (2.f * scl);
        float4x4 translate = math::makeTranslate(math::add(objectPosition, {x, y, z}));

        pInstanceData[i].instanceTransform = fullObjectRot * translate * yrot * zrot * scale;
        pInstanceData[i].instanceNormalTransform = math::discardTranslation(pInstanceData[i].instanceTransform);

        float iDivNumInstances = i / (float)kNumInstances;
        float r = iDivNumInstances;
        float g = 1.0f - r;
        float b = sinf(M_PI * 2.0f * iDivNumInstances);
        pInstanceData[i].instanceColor = (float4){r, g, b, 1.0f};

        ix += 1;
    }

    pInstanceDataBuffer->didModifyRange(NS::Range::Make(0, pInstanceDataBuffer->length()));

    pOffscreenEncoder->drawIndexedPrimitives(MTL::PrimitiveType::PrimitiveTypeTriangle,
                                             6 * 6, MTL::IndexType::IndexTypeUInt16,
                                             _pIndexBuffer, 0, kNumInstances);
}

void Renderer::draw(MTK::View* pView)
{
    if (!g_controller)
    {
         NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();

        _frame = (_frame + 1) % Renderer::kMaxFramesInFlight;
        MTL::Buffer* pInstanceDataBuffer = _pInstanceDataBuffer[ _frame ];

        MTL::CommandBuffer* pCmd = _pCommandQueue->commandBuffer();
        dispatch_semaphore_wait( _semaphore, DISPATCH_TIME_FOREVER );
        Renderer* pRenderer = this;
        pCmd->addCompletedHandler( ^void( MTL::CommandBuffer* pCmd ){
            dispatch_semaphore_signal( pRenderer->_semaphore );
        });

        _angle += 0.002f;

        // Update instance positions:

        const float scl = 0.2f;
        shader_types::InstanceData* pInstanceData = reinterpret_cast< shader_types::InstanceData *>( pInstanceDataBuffer->contents() );

        float3 objectPosition = { 0.f, 0.f, -10.f };

        float4x4 rt = math::makeTranslate( objectPosition );
        float4x4 rr1 = math::makeYRotate( -_angle );
        float4x4 rr0 = math::makeXRotate( _angle * 0.5 );
        float4x4 rtInv = math::makeTranslate( { -objectPosition.x, -objectPosition.y, -objectPosition.z } );
        float4x4 fullObjectRot = rt * rr1 * rr0 * rtInv;

        size_t ix = 0;
        size_t iy = 0;
        size_t iz = 0;
        for ( size_t i = 0; i < kNumInstances; ++i )
        {
            if ( ix == kInstanceRows )
            {
                ix = 0;
                iy += 1;
            }
            if ( iy == kInstanceRows )
            {
                iy = 0;
                iz += 1;
            }

            float4x4 scale = math::makeScale( (float3){ scl, scl, scl } );
            float4x4 zrot = math::makeZRotate( _angle * sinf((float)ix) );
            float4x4 yrot = math::makeYRotate( _angle * cosf((float)iy));

            float x = ((float)ix - (float)kInstanceRows/2.f) * (2.f * scl) + scl;
            float y = ((float)iy - (float)kInstanceColumns/2.f) * (2.f * scl) + scl;
            float z = ((float)iz - (float)kInstanceDepth/2.f) * (2.f * scl);
            float4x4 translate = math::makeTranslate( math::add( objectPosition, { x, y, z } ) );

            pInstanceData[ i ].instanceTransform = fullObjectRot * translate * yrot * zrot * scale;
            pInstanceData[ i ].instanceNormalTransform = math::discardTranslation( pInstanceData[ i ].instanceTransform );

            float iDivNumInstances = i / (float)kNumInstances;
            float r = iDivNumInstances;
            float g = 1.0f - r;
            float b = sinf( M_PI * 2.0f * iDivNumInstances );
            pInstanceData[ i ].instanceColor = (float4){ r, g, b, 1.0f };

            ix += 1;
        }

        // Update camera state:

        CGSize drawableSize = pView->drawableSize();
        CGFloat aspectRatio = drawableSize.width / drawableSize.height;

        MTL::Buffer* pCameraDataBuffer = _pCameraDataBuffer[ _frame ];
        shader_types::CameraData* pCameraData = reinterpret_cast< shader_types::CameraData *>( pCameraDataBuffer->contents() );
        pCameraData->perspectiveTransform = math::makePerspective( 45.f * M_PI / 180.f, aspectRatio, 0.03f, 500.0f ) ;
        pCameraData->worldTransform = math::makeIdentity();
        pCameraData->worldNormalTransform = math::discardTranslation( pCameraData->worldTransform );

        // Begin render pass:

        MTL::RenderPassDescriptor* pRpd = pView->currentRenderPassDescriptor();

        pRpd->colorAttachments()->object(0)->setClearColor(MTL::ClearColor::Make(0.0, 0.0, 0.0, 1.0)); // Clear to black
        pRpd->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionClear);   // Ensure it clears
        pRpd->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore); // Store the result

        MTL::RenderCommandEncoder* pEnc = pCmd->renderCommandEncoder( pRpd );

        pEnc->setRenderPipelineState( _pPSO );
        pEnc->setDepthStencilState( _pDepthStencilState );

        pEnc->setVertexBuffer( _pVertexDataBuffer, /* offset */ 0, /* index */ 0 );
        pEnc->setVertexBuffer( pInstanceDataBuffer, /* offset */ 0, /* index */ 1 );
        pEnc->setVertexBuffer( pCameraDataBuffer, /* offset */ 0, /* index */ 2 );

        pEnc->setCullMode( MTL::CullModeBack );
        pEnc->setFrontFacingWinding( MTL::Winding::WindingCounterClockwise );

        pEnc->drawIndexedPrimitives( MTL::PrimitiveType::PrimitiveTypeTriangle,
                                    6 * 6, MTL::IndexType::IndexTypeUInt16,
                                    _pIndexBuffer,
                                    0,
                                    kNumInstances );

        pEnc->endEncoding();
        pCmd->presentDrawable( pView->currentDrawable() );
        pCmd->commit();

        pPool->release();
        
        return;
    }
    NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();

    _frame = (_frame + 1) % Renderer::kMaxFramesInFlight;
    MTL::Buffer* pInstanceDataBuffer = _pInstanceDataBuffer[_frame];

    // Wait for frame to complete
    MTL::CommandBuffer* pCmd = _pCommandQueue->commandBuffer();
    dispatch_semaphore_wait(_semaphore, DISPATCH_TIME_FOREVER);
    Renderer* pRenderer = this;
    pCmd->addCompletedHandler(^void(MTL::CommandBuffer* pCmd) {
        dispatch_semaphore_signal(pRenderer->_semaphore);
    });

    _angle += 0.002f;

    // Update camera state
    MTL::Buffer* pCameraDataBuffer = _pCameraDataBuffer[_frame];
    shader_types::CameraData* pCameraData = reinterpret_cast<shader_types::CameraData*>(pCameraDataBuffer->contents());
    pCameraData->perspectiveTransform = math::makePerspective(45.f * M_PI / 180.f, 1.f, 0.03f, 500.0f);
    pCameraData->worldTransform = math::makeIdentity();
    pCameraData->worldNormalTransform = math::discardTranslation(pCameraData->worldTransform);
    pCameraDataBuffer->didModifyRange(NS::Range::Make(0, sizeof(shader_types::CameraData)));

    // Begin the offscreen render pass
    MTL::RenderPassDescriptor* pOffscreenPassDesc = MTL::RenderPassDescriptor::alloc()->init();
    pOffscreenPassDesc->colorAttachments()->object(0)->setTexture(_pOffscreenRenderTarget);
    pOffscreenPassDesc->colorAttachments()->object(0)->setLoadAction(MTL::LoadAction::LoadActionClear);
    pOffscreenPassDesc->colorAttachments()->object(0)->setClearColor(MTL::ClearColor::Make(0.0, 0.0, 0.0, 1.0));
    pOffscreenPassDesc->colorAttachments()->object(0)->setStoreAction(MTL::StoreAction::StoreActionStore);

    // Attach the depth texture
    pOffscreenPassDesc->depthAttachment()->setTexture(_pDepthTexture);
    pOffscreenPassDesc->depthAttachment()->setLoadAction(MTL::LoadAction::LoadActionClear);
    pOffscreenPassDesc->depthAttachment()->setClearDepth(1.0f);
    pOffscreenPassDesc->depthAttachment()->setStoreAction(MTL::StoreAction::StoreActionDontCare);

    MTL::RenderCommandEncoder* pOffscreenEncoder = pCmd->renderCommandEncoder(pOffscreenPassDesc);
    pOffscreenEncoder->setRenderPipelineState(_pPSO);
    pOffscreenEncoder->setDepthStencilState(_pDepthStencilState);

    pOffscreenEncoder->setVertexBuffer(_pVertexDataBuffer, 0, 0);
    pOffscreenEncoder->setVertexBuffer(pInstanceDataBuffer, 0, 1);
    pOffscreenEncoder->setVertexBuffer(pCameraDataBuffer, 0, 2);

    pOffscreenEncoder->setCullMode(MTL::CullModeBack);
    pOffscreenEncoder->setFrontFacingWinding(MTL::WindingCounterClockwise);

    // Set up viewports and loop through them
    float tx_offset = 0.01f; // horizontal offset per view
    float tx = -(float)(g_vx * g_vy - 1) / 2.0f * tx_offset;

    for (uint32_t y = 0; y < g_vy; y++) {
        for (uint32_t x = 0; x < g_vx; x++) {
            // Calculate the Y-coordinate so the first view is at the bottom left
            uint32_t yPos = g_quilt_height - (y + 1) * g_view_height;

            // Set up the viewport for this quilt tile
            MTL::Viewport quiltViewport = {
                (double)x * g_view_width,      // X-coordinate for this view
                (double)yPos,                  // Y-coordinate (starting from the bottom)
                (double)g_view_width,          // Width of the view
                (double)g_view_height,         // Height of the view
                (double)0,                     // Z-near
                (double)1                      // Z-far
            };

            // Set the viewport for the current render pass
            pOffscreenEncoder->setViewport(quiltViewport);

            // Call drawInternal for the current viewport
            drawInternal(pInstanceDataBuffer, pOffscreenEncoder, tx);

            // Increment tx for the next viewport
            tx += tx_offset;
        }
    }

    pOffscreenEncoder->endEncoding();
    pOffscreenPassDesc->release();

    // mlc: second pass: let bridge make the hologram
    g_controller->DrawInteropQuiltTextureMetal(g_wnd, _pOffscreenRenderTarget, g_vx, g_vy, 1.0f, 1.0f);

    // mlc: third pass - Render the full-screen quad with the offscreen hologram texture
    MTL::Texture* pHologramTexture;
    g_controller->GetOffscreenWindowTextureMetal(g_wnd, (void**)&pHologramTexture);

    MTL::RenderPassDescriptor* pFinalPassDesc = pView->currentRenderPassDescriptor();
    if (pFinalPassDesc)
    {
        MTL::RenderCommandEncoder* pFinalEncoder = pCmd->renderCommandEncoder(pFinalPassDesc);

        pFinalEncoder->setRenderPipelineState(_pQuadPSO);
        pFinalEncoder->setVertexBuffer(_pQuadVertexBuffer, 0, 0);
        pFinalEncoder->setFragmentTexture(pHologramTexture, 0);

        pFinalEncoder->drawIndexedPrimitives(MTL::PrimitiveType::PrimitiveTypeTriangle, 
                                             6, MTL::IndexType::IndexTypeUInt16, 
                                             _pQuadIndexBuffer, 0);

        pFinalEncoder->endEncoding();
    }

    pCmd->presentDrawable(pView->currentDrawable());
    pCmd->commit();

    pPool->release();
}


#pragma endregion Renderer }
