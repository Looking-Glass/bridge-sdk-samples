using System;
using System.Diagnostics;
using System.Drawing;
using System.Windows.Forms;
using Core;
using SlimDX;
using SlimDX.D3DCompiler;
using SlimDX.DXGI;
using SlimDX.Direct3D11;
using Buffer = SlimDX.Direct3D11.Buffer;
using Debug = System.Diagnostics.Debug;

namespace BoxDemo {
    using Core.Vertex;
    using System.Runtime.InteropServices;
    using Effect = SlimDX.Direct3D11.Effect;

    public class BoxApp : D3DApp {
        private Buffer _boxVB;
        private Buffer _boxIB;

        private Effect _fx;
        private EffectTechnique _tech;
        private EffectTechnique _techUint;
        private EffectMatrixVariable _fxWVP;

        private InputLayout _inputLayout;
        private InputLayout _inputLayoutUint;

        // Matrices
        private Matrix _world;
        private Matrix _view;
        private Matrix _proj;

        // Camera variables
        private float _theta;
        private float _phi;
        private float _radius;

        private Point _lastMousePos;

        private bool _disposed;

        private float _lastTimestamp = Stopwatch.GetTimestamp();
        private float _freq = Stopwatch.Frequency;
        private float _angle = 0.0f;

        BridgeSDK.Window _bridge_window;
        uint _bridge_window_width = 0;
        uint _bridge_window_height = 0;
        uint _bridge_max_texture_size = 0;
        uint _bridge_render_texture_width = 0;
        uint _bridge_render_texture_height = 0;
        uint _bridge_quilt_vx = 5;
        uint _bridge_quilt_vy = 9;
        uint _bridge_quilt_view_width = 0;
        uint _bridge_quilt_view_height = 0;
        private Texture2D _quiltRenderTarget;
        private RenderTargetView _quiltRtv;
        private Texture2D _quiltDepthStencilBuffer;
        private DepthStencilView _quiltDsv;

        bool window_showing = true;
        bool inited = true;
        bool show_state_toggle_requested = false;

        public BoxApp(IntPtr hInstance) : base(hInstance)
        {
            _boxIB = null;
            _boxVB = null;
            _fx = null;
            _tech = null;
            _fxWVP = null;
            _inputLayout = null;
            _theta = 1.5f * MathF.PI;
            _phi = 0.25f * MathF.PI;
            _radius = 5.0f;

            MainWindowCaption = "Box Demo";
            _lastMousePos = new Point(0, 0);
            _world = Matrix.Identity;
            _view = Matrix.Identity;
            _proj = Matrix.Identity;
        }

        protected override void Dispose(bool disposing)
        {
            if (!_disposed)
            {
                if (disposing)
                {
                    UninitBridge();

                    Util.ReleaseCom(ref _boxVB);
                    Util.ReleaseCom(ref _boxIB);
                    Util.ReleaseCom(ref _fx);
                    Util.ReleaseCom(ref _inputLayout);
                    Util.ReleaseCom(ref _inputLayoutUint);
                }

                _disposed = true;
            }

            base.Dispose(disposing);
        }

    void UninitBridge()
    {
       BridgeSDK.Controller.UnregisterTextureDX(_bridge_window, _quiltRenderTarget.ComPointer);

        Util.ReleaseCom(ref _quiltRenderTarget);
        Util.ReleaseCom(ref _quiltRtv);
        Util.ReleaseCom(ref _quiltDepthStencilBuffer);
        Util.ReleaseCom(ref _quiltDsv);

        if (window_showing)
        {
            BridgeSDK.Controller.ShowWindow(_bridge_window, false);
        }

        BridgeSDK.Controller.Uninitialize();
    }

    void InitBridge()
    {
        // mlc: init bridge
        if (!BridgeSDK.Controller.Initialize(@"BridgeSDKSampleDX11"))
        {
            Environment.Exit(-1);
        }

        // mlc: instance the window
        bool window_status = BridgeSDK.Controller.InstanceWindowDX(Device.ComPointer, ref _bridge_window);

        if (!window_status)
        {
            throw new Exception("");
        }

       // mlc: cache the size of the bridge output so we can decide how large to make
       // the quilt views
       BridgeSDK.Controller.GetWindowDimensions(_bridge_window, ref _bridge_window_width, ref _bridge_window_height);

        // mlc: see how large we can make out render texture
        BridgeSDK.Controller.GetMaxTextureSize(_bridge_window, ref _bridge_max_texture_size);

        // mlc: dx puts an artifical cap on this on some cards:
        if (_bridge_max_texture_size > 16384)
            _bridge_max_texture_size = 16384;

        // mlc: now we need to figure out how large our views and quilt will be
        uint desired_view_width = _bridge_window_width;
        uint desired_view_height = _bridge_window_height;

        uint desired_render_texture_width = desired_view_width * _bridge_quilt_vx;
        uint desired_render_texture_height = desired_view_height * _bridge_quilt_vy;

        if (desired_render_texture_width <= _bridge_max_texture_size &&
            desired_render_texture_height <= _bridge_max_texture_size)
        {
            // mlc: under the max size -- good to go!
            _bridge_quilt_view_width = desired_view_width;
            _bridge_quilt_view_height = desired_view_height;
            _bridge_render_texture_width = desired_render_texture_width;
            _bridge_render_texture_height = desired_render_texture_height;
        }
        else
        {
            // mlc: the desired sizes are larger than we can support, find the dominant
            // and scale down to fit.
            float scalar = 0.0f;

            if (desired_render_texture_width > desired_render_texture_height)
            {
                scalar = (float)_bridge_max_texture_size / (float)desired_render_texture_width;
            }
            else
            {
                scalar = (float)_bridge_max_texture_size / (float)desired_render_texture_height;
            }

            _bridge_quilt_view_width = (uint)((float)desired_view_width * scalar);
            _bridge_quilt_view_height = (uint)((float)desired_view_height * scalar);
            _bridge_render_texture_width = (uint)((float)desired_render_texture_width * scalar);
            _bridge_render_texture_height = (uint)((float)desired_render_texture_height * scalar);
        }

        var renderTargetDesc = new SlimDX.Direct3D11.Texture2DDescription
        {
            Width = (int)_bridge_render_texture_width,
            Height = (int)_bridge_render_texture_height,
            MipLevels = 1,
            ArraySize = 1,
            Format = SlimDX.DXGI.Format.R8G8B8A8_UNorm, // R10G10B10A2_UNorm 
            SampleDescription = new SlimDX.DXGI.SampleDescription(1, 0),
            Usage = SlimDX.Direct3D11.ResourceUsage.Default,
            BindFlags = SlimDX.Direct3D11.BindFlags.RenderTarget,
            CpuAccessFlags = SlimDX.Direct3D11.CpuAccessFlags.None,
            OptionFlags = SlimDX.Direct3D11.ResourceOptionFlags.None
        };

        _quiltRenderTarget = new SlimDX.Direct3D11.Texture2D(Device, renderTargetDesc);
        _quiltRtv = new SlimDX.Direct3D11.RenderTargetView(Device, _quiltRenderTarget);

        var depthBufferDesc = new SlimDX.Direct3D11.Texture2DDescription
        {
            Width = (int)_bridge_render_texture_width,
            Height = (int)_bridge_render_texture_height,
            MipLevels = 1,
            ArraySize = 1,
            Format = SlimDX.DXGI.Format.D24_UNorm_S8_UInt, // Common depth-stencil format
            SampleDescription = new SlimDX.DXGI.SampleDescription(1, 0),
            Usage = SlimDX.Direct3D11.ResourceUsage.Default,
            BindFlags = SlimDX.Direct3D11.BindFlags.DepthStencil,
            CpuAccessFlags = SlimDX.Direct3D11.CpuAccessFlags.None,
            OptionFlags = SlimDX.Direct3D11.ResourceOptionFlags.None
        };

        _quiltDepthStencilBuffer = new SlimDX.Direct3D11.Texture2D(Device, depthBufferDesc);

        // Create the Depth Stencil View (DSV)
        _quiltDsv = new SlimDX.Direct3D11.DepthStencilView(Device, _quiltDepthStencilBuffer);

        BridgeSDK.Controller.RegisterTextureDX(_bridge_window, _quiltRenderTarget.ComPointer);
        
        window_showing = true;
    }

    public override bool Init() 
    {
        if (!base.Init()) 
        {
            return false;
        }

        InitBridge();

        BuildGeometryBuffers();
        BuildFX();
        BuildVertexLayout();

        return true;
    }

    public override void OnResize() {
        base.OnResize();
        // Recalculate perspective matrix
        _proj = Matrix.PerspectiveFovLH(0.25f * MathF.PI, AspectRatio, 1.0f, 1000.0f);
    }

    public override void UpdateScene(float dt) {
        base.UpdateScene(dt);

        // Get camera position from polar coords
        var x = _radius * MathF.Sin(_phi) * MathF.Cos(_theta);
        var z = _radius * MathF.Sin(_phi) * MathF.Sin(_theta);
        var y = _radius * MathF.Cos(_phi);

        // Build the view matrix
        var pos = new Vector3(x, y, z);
        var target = new Vector3(0);
        var up = new Vector3(0, 1, 0);
        _view = Matrix.LookAtLH(pos, target, up);

    }

    protected void Draw(float tx = 0.0f)
    {    
        // Update transformation matrices
        _world = Matrix.RotationYawPitchRoll(_angle, 0.0f, _angle);
        Matrix wvp = _world * _view * _proj;

        _fxWVP.SetMatrix(wvp);

        // Setup for drawing
        ImmediateContext.InputAssembler.InputLayout = _inputLayout;

        ImmediateContext.InputAssembler.PrimitiveTopology = PrimitiveTopology.TriangleList;
        ImmediateContext.InputAssembler.SetVertexBuffers(0, new VertexBufferBinding(_boxVB, VertexPC.Stride, 0));
        ImmediateContext.InputAssembler.SetIndexBuffer(_boxIB, Format.R32_UInt, 0);

        EffectTechnique tech = _tech;

        for (int p = 0; p < _tech.Description.PassCount; p++)
        {
            tech.GetPassByIndex(p).Apply(ImmediateContext);
            ImmediateContext.DrawIndexed(36, 0, 0); // Using 36 as the fixed index count, adjust as necessary
        }
    }

        public override void DrawScene()
        {
            if (show_state_toggle_requested)
            {
                BridgeSDK.Controller.ShowWindow(_bridge_window, window_showing);
                show_state_toggle_requested = false;
            }

            base.DrawScene();

            // Update for animation or any dynamic elements in the scene
            var timeStamp = Stopwatch.GetTimestamp();
            _angle += (float)((timeStamp - _lastTimestamp) / (double)_freq);
            _lastTimestamp = timeStamp;

            if (inited)
            {
                // Set render target to the quilt texture and its corresponding depth stencil
                ImmediateContext.OutputMerger.SetTargets(_quiltDsv, _quiltRtv);

                // Clear the render target and depth stencil for the quilt
                ImmediateContext.ClearRenderTargetView(_quiltRtv, Color.Black);
                ImmediateContext.ClearDepthStencilView(_quiltDsv, DepthStencilClearFlags.Depth, 1.0f, 0);

                float tx_offset = 0.5f;
                float tx = -(float)(_bridge_quilt_vx * _bridge_quilt_vy - 1) / 2.0f * tx_offset;

                for (uint y = 0; y < _bridge_quilt_vy; y++)
                {
                    for (uint x = 0; x < _bridge_quilt_vx; x++)
                    {
                        uint invertedY = _bridge_quilt_vy - 1 - y;

                        // Adjust viewport for the current quilt view
                        Viewport quiltViewport = new Viewport(
                            x * _bridge_quilt_view_width,
                            invertedY * _bridge_quilt_view_height,
                            _bridge_quilt_view_width,
                            _bridge_quilt_view_height,
                            0.0f, 1.0f);

                        ImmediateContext.Rasterizer.SetViewports(quiltViewport);

                        // Call the inner draw function to render the scene for the current view
                        Draw(tx);

                        tx += tx_offset;
                    }
                }
            }

            // Switch back to the primary back buffer
            ImmediateContext.OutputMerger.SetTargets(DepthStencilView, RenderTargetView);

            // Reset viewport to the full screen for the main display
            Viewport fullViewport = new Viewport(
                0.0f, 0.0f,
                SwapChain.Description.ModeDescription.Width,
                SwapChain.Description.ModeDescription.Height,
                0.0f, 1.0f);
            ImmediateContext.Rasterizer.SetViewports(fullViewport);

            // Clear the primary back buffer and depth stencil
            ImmediateContext.ClearRenderTargetView(RenderTargetView, Color.Black);
            ImmediateContext.ClearDepthStencilView(DepthStencilView, DepthStencilClearFlags.Depth | DepthStencilClearFlags.Stencil, 1.0f, 0);

            // Draw the scene again for the main display head
            Draw(); // This Draw call is for the main display, make sure it's set up to render as needed for the primary view

            if (inited)
            {
                BridgeSDK.Controller.DrawInteropQuiltTextureDX(_bridge_window, _quiltRenderTarget.ComPointer, _bridge_quilt_vx, _bridge_quilt_vy, 1.0f, 1.0f);

                bool save = false;
                if (save)
                {
                    BridgeSDK.Controller.SaveTextureToFileDX(_bridge_window, @"C:\temp\quilt.png", _quiltRenderTarget.ComPointer);
                }
            }

            // Present the back buffer to the screen
            SwapChain.Present(0, PresentFlags.None);
        }

    protected override void OnMouseDown(object sender, MouseEventArgs mouseEventArgs) 
    {
        _lastMousePos = mouseEventArgs.Location;
        Window.Capture = true;
    }

    protected override void OnMouseUp(object sender, MouseEventArgs e) 
    {
        Window.Capture = false;
    }

    protected override void OnMouseMove(object sender, MouseEventArgs e) 
    {
        if (e.Button == MouseButtons.Left) 
        {
            var dx = MathF.ToRadians(0.25f * (e.X - _lastMousePos.X));
            var dy = MathF.ToRadians(0.25f * (e.Y - _lastMousePos.Y));

            _theta += dx;
            _phi += dy;

            _phi = MathF.Clamp(_phi, 0.1f, MathF.PI - 0.1f);
        } 
        else if (e.Button == MouseButtons.Right) 
        {
            var dx = 0.005f * (e.X - _lastMousePos.X);
            var dy = 0.005f * (e.Y - _lastMousePos.Y);
            _radius += dx - dy;

            _radius = MathF.Clamp(_radius, 3.0f, 15.0f);
        }

        _lastMousePos = e.Location;
    }

    private void BuildGeometryBuffers() 
    {
        var vertices = new[] {
            new VertexPC(new Vector3(-1.0f, -1.0f, -1.0f), Color.White),
            new VertexPC(new Vector3(-1, 1, -1), Color.Black),
            new VertexPC(new Vector3(1,1,-1), Color.Red ),
            new VertexPC( new Vector3(1,-1,-1), Color.Green ),
            new VertexPC(new Vector3(-1,-1,1),Color.Blue ),
            new VertexPC(new Vector3(-1,1,1), Color.Yellow ),
            new VertexPC(new Vector3(1,1,1), Color.Cyan ),
            new VertexPC(new Vector3(1,-1,1),Color.Magenta )
        };
        var vbd = new BufferDescription(
            VertexPC.Stride*vertices.Length, 
            ResourceUsage.Immutable, 
            BindFlags.VertexBuffer, 
            CpuAccessFlags.None, 
            ResourceOptionFlags.None, 
            0);
        _boxVB = new Buffer(Device, new DataStream(vertices, true, false), vbd);

        var indices = new uint[] {
            // front
            0,1,2,
            0,2,3,
            // back
            4,6,5,
            4,7,6,
            // left
            4,5,1,
            4,1,0,
            // right
            3,2,6,
            3,6,7,
            //top
            1,5,6,
            1,6,2,
            // bottom
            4,0,3,
            4,3,7
        };
        var ibd = new BufferDescription(
            sizeof (uint)*indices.Length, 
            ResourceUsage.Immutable, 
            BindFlags.IndexBuffer, 
            CpuAccessFlags.None, 
            ResourceOptionFlags.None, 
            0);
        _boxIB = new Buffer(Device, new DataStream(indices, false, false), ibd);
    }
    private void BuildFX() {
        var shaderFlags = ShaderFlags.None;
        #if DEBUG
            shaderFlags |= ShaderFlags.Debug;
            shaderFlags |= ShaderFlags.SkipOptimization;
        #endif
        string errors = null;
        ShaderBytecode compiledShader = null;
        try {
            compiledShader = ShaderBytecode.CompileFromFile(
                "FX/color.fx", 
                null, 
                "fx_5_0", 
                shaderFlags, 
                EffectFlags.None, 
                null, 
                null, 
                out errors);
            _fx = new Effect(Device, compiledShader);
        } catch (Exception ex) {
            if (!string.IsNullOrEmpty(errors)) {
                MessageBox.Show(errors);
            }
            MessageBox.Show(ex.Message);
            return;
        } finally {
            Util.ReleaseCom(ref compiledShader);
        }

        _tech = _fx.GetTechniqueByName("ColorTech");
        _techUint = _fx.GetTechniqueByName("ColorTechUInt");
        _fxWVP = _fx.GetVariableByName("gWorldViewProj").AsMatrix();
    }
    private void BuildVertexLayout() {
        var vertexDesc = new[] {
            new InputElement("POSITION", 0, Format.R32G32B32_Float, 
                0, 0, InputClassification.PerVertexData, 0),
            new InputElement("COLOR", 0, Format.R32G32B32A32_Float, 
                12, 0, InputClassification.PerVertexData, 0)
        };
        Debug.Assert(_tech != null);

        _inputLayout = new InputLayout(Device, _tech.GetPassByIndex(0).Description.Signature, vertexDesc);
        _inputLayoutUint = new InputLayout(Device, _techUint.GetPassByIndex(0).Description.Signature, vertexDesc);
    }
    }


    static class Program {
        static void Main(string[] args) {
            Configuration.EnableObjectTracking = true;
            var app = new BoxApp(Process.GetCurrentProcess().Handle);
            if (!app.Init()) {
                return;
            }
            app.Run();
        }
    }
}
