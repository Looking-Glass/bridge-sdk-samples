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
    using System.IO;
    using System.Reflection;
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

        private Texture2D _bridge_offscreen_texture;
        private Buffer _fullscreenQuadVB;
        private Effect _fxNdcTexture;
        private EffectTechnique _techRenderTexture;
        private ShaderResourceView _quadTextureSRV;

        BridgeInProc.Window _bridge_window;
        long _bridge_window_x = 0;
        long _bridge_window_y = 0;
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

        bool inited = true;

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
        if (_quiltRenderTarget != null)
        {
            BridgeInProc.Controller.UnregisterTextureDX(_bridge_window, _quiltRenderTarget.ComPointer);
            Util.ReleaseCom(ref _quiltRenderTarget);
        }

        Util.ReleaseCom(ref _quiltRtv);
        Util.ReleaseCom(ref _quiltDepthStencilBuffer);
        Util.ReleaseCom(ref _quiltDsv);

        BridgeInProc.Controller.Uninitialize();
    }

    void InitBridge()
    {
        // mlc: init bridge
        if (!BridgeInProc.Controller.Initialize(@"BridgeInProcSampleDX11Interactive"))
        {
            Environment.Exit(-1);
        }

        // mlc: instance the window
        bool window_status = BridgeInProc.Controller.InstanceOffscreenWindowDX(Device.ComPointer, ref _bridge_window);

        if (!window_status)
        {
           return;
        }

        BridgeInProc.Controller.GetWindowDimensions(_bridge_window, ref _bridge_window_width, ref _bridge_window_height);
        BridgeInProc.Controller.GetWindowPosition(_bridge_window, ref _bridge_window_x, ref _bridge_window_y);

        // mlc: see how large we can make out render texture
        BridgeInProc.Controller.GetMaxTextureSize(_bridge_window, ref _bridge_max_texture_size);

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

        BridgeInProc.Controller.RegisterTextureDX(_bridge_window, _quiltRenderTarget.ComPointer);

        SetWindowProperties();
    }

    public override bool Init() 
    {
        if (!base.Init()) 
        {
            return false;
        }

        InitBridge();

        BuildGeometryBuffers();
        BuildFullScreenQuad();
        BuildFX();
        BuildVertexLayout();

        return true;
    }

    void SetWindowProperties()
    {
        // Remove the title bar and borders (set as popup)
        var hWnd = Process.GetCurrentProcess().MainWindowHandle;
        uint style = NativeMethods.GetWindowLong(hWnd, NativeMethods.GWL_STYLE);
        style &= ~NativeMethods.WS_OVERLAPPEDWINDOW;  // Remove standard window styles
        style |= NativeMethods.WS_POPUP;              // Set to popup (borderless)
        NativeMethods.SetWindowLong(hWnd, NativeMethods.GWL_STYLE, style);

        // Position the window at _bridge_window_x, _bridge_window_y and set the size
        NativeMethods.SetWindowPos(
            hWnd,
            IntPtr.Zero,  // No special window ordering
            (int)_bridge_window_x,
            (int)_bridge_window_y,
            (int)_bridge_window_width,
            (int)_bridge_window_height,
            NativeMethods.SWP_NOZORDER | NativeMethods.SWP_FRAMECHANGED
        );

        // Trigger OnResize to update the projection matrix
        OnResize();
    }

    public override void OnResize() 
    {
        if (_bridge_window_width != 0)
        {
            ClientWidth = (int)_bridge_window_width;
        }

        if (_bridge_window_height != 0)
        {
            ClientHeight = (int)_bridge_window_height;
        }
        
        base.OnResize();
        
        // Recalculate perspective matrix
        _proj = Matrix.PerspectiveFovLH(0.5f * MathF.PI, AspectRatio, 1.0f, 1000.0f);
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

    public static void SaveTextureAsPng(SlimDX.Direct3D11.Device device, Texture2D texture, string filePath)
    {
        try
        {
            // Create a stream to save the texture
            using (var stream = new MemoryStream())
            {
                // Use Texture2D.ToStream to save the texture to the stream in PNG format
                Texture2D.ToStream(device.ImmediateContext, texture, ImageFileFormat.Png, stream);

                // Write the stream to a PNG file
                File.WriteAllBytes(filePath, stream.ToArray());

                Console.WriteLine($"Texture saved successfully to {filePath}");
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Error saving texture to PNG: {ex.Message}");
        }
    }

    public override void DrawScene()
    {
        base.DrawScene();

        // Update for animation or any dynamic elements in the scene
        var timeStamp = Stopwatch.GetTimestamp();
        _angle += (float)((timeStamp - _lastTimestamp) / (double)_freq);
        _lastTimestamp = timeStamp;

        if (inited && _quiltRtv != null)
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

            // Switch back to the primary back buffer
            ImmediateContext.OutputMerger.SetTargets(DepthStencilView, RenderTargetView);

            // Reset viewport to the full screen for the main display
            Viewport fullViewport = new Viewport(
                0.0f, 0.0f,
                SwapChain.Description.ModeDescription.Width,
                SwapChain.Description.ModeDescription.Height,
                0.0f, 1.0f);
            ImmediateContext.Rasterizer.SetViewports(fullViewport);

            if (inited)
            {
                BridgeInProc.Controller.DrawInteropQuiltTextureDX(_bridge_window, _quiltRenderTarget.ComPointer, _bridge_quilt_vx, _bridge_quilt_vy, 1.0f, 1.0f);

                if (_bridge_offscreen_texture == null)
                {
                    IntPtr nativeTexturePtr = IntPtr.Zero;
                    if (BridgeInProc.Controller.GetOffscreenWindowTextureDX(_bridge_window, out nativeTexturePtr))
                    {
                        var textureType = typeof(SlimDX.Direct3D11.Texture2D);
                        var constructor = textureType.GetConstructor(
                            BindingFlags.NonPublic | BindingFlags.Instance,
                            null,
                            new Type[] { typeof(IntPtr) },
                            null);

                        _bridge_offscreen_texture = (SlimDX.Direct3D11.Texture2D)constructor.Invoke(new object[] { nativeTexturePtr });
                        _quadTextureSRV = new ShaderResourceView(Device, _bridge_offscreen_texture);
                    }
                }

                if (_bridge_offscreen_texture != null)
                {
                    // Clear the primary back buffer and depth stencil
                    ImmediateContext.ClearRenderTargetView(RenderTargetView, Color.Black);
                    ImmediateContext.ClearDepthStencilView(DepthStencilView, DepthStencilClearFlags.Depth | DepthStencilClearFlags.Stencil, 1.0f, 0);

                    // Set the input layout for VertexPC
                    ImmediateContext.InputAssembler.InputLayout = _inputLayout;

                    SamplerDescription samplerDesc = new SamplerDescription
                    {
                        Filter = Filter.MinMagMipLinear, // Use linear filtering for simplicity
                        AddressU = TextureAddressMode.Clamp,
                        AddressV = TextureAddressMode.Clamp,
                        AddressW = TextureAddressMode.Clamp,
                        ComparisonFunction = Comparison.Never,
                        MinimumLod = 0,
                        MaximumLod = float.MaxValue
                    };

                    // Create the sampler state and bind it to the pixel shader
                    SamplerState samplerState = SamplerState.FromDescription(Device, samplerDesc);
                    ImmediateContext.PixelShader.SetSampler(samplerState, 0);

                    // Set up the input assembler for the full-screen quad
                    ImmediateContext.InputAssembler.PrimitiveTopology = PrimitiveTopology.TriangleStrip;
                    ImmediateContext.InputAssembler.SetVertexBuffers(0, new VertexBufferBinding(_fullscreenQuadVB, VertexPC.Stride, 0));

                    // Apply the full-screen quad rendering technique
                    EffectTechnique tech = _techRenderTexture;
                    for (int p = 0; p < tech.Description.PassCount; p++)
                    {
                        tech.GetPassByIndex(p).Apply(ImmediateContext);
                        ImmediateContext.PixelShader.SetShaderResource(_quadTextureSRV, 0);
                        ImmediateContext.Draw(4, 0);  // Draw the full-screen quad
                    }
                }
            }
        }
        else
        {
            // mlc: no looking glass device connected: draw primary display
            ImmediateContext.OutputMerger.SetTargets(DepthStencilView, RenderTargetView);

            // Reset viewport to the full screen for the main display
            Viewport fullViewport = new Viewport(
                0.0f, 0.0f,
                SwapChain.Description.ModeDescription.Width,
                SwapChain.Description.ModeDescription.Height,
                0.0f, 1.0f);
            ImmediateContext.Rasterizer.SetViewports(fullViewport);

            ImmediateContext.ClearRenderTargetView(RenderTargetView, Color.Black);
            ImmediateContext.ClearDepthStencilView(DepthStencilView, DepthStencilClearFlags.Depth | DepthStencilClearFlags.Stencil, 1.0f, 0);

            Draw();
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

    private void BuildFullScreenQuad()
    {
        var quadVertices = new[]
        {
            new VertexPC(new Vector3(-1.0f, -1.0f, 0.0f), new Color4(1.0f, 1.0f, 1.0f, 1.0f)), // Bottom-left 
            new VertexPC(new Vector3(-1.0f,  1.0f, 0.0f), new Color4(1.0f, 1.0f, 1.0f, 1.0f)), // Top-left 
            new VertexPC(new Vector3( 1.0f, -1.0f, 0.0f), new Color4(1.0f, 1.0f, 1.0f, 1.0f)), // Bottom-right 
            new VertexPC(new Vector3( 1.0f,  1.0f, 0.0f), new Color4(1.0f, 1.0f, 1.0f, 1.0f))  // Top-right 
        };

        var quadBufferDesc = new BufferDescription(
            VertexPC.Stride * quadVertices.Length,
            ResourceUsage.Immutable,
            BindFlags.VertexBuffer,
            CpuAccessFlags.None,
            ResourceOptionFlags.None,
            0);

        _fullscreenQuadVB = new Buffer(Device, new DataStream(quadVertices, true, false), quadBufferDesc);
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
        private void BuildFX()
        {
            var shaderFlags = ShaderFlags.None;
#if DEBUG
            shaderFlags |= ShaderFlags.Debug;
            shaderFlags |= ShaderFlags.SkipOptimization;
#endif
            string errors = null;
            ShaderBytecode compiledShader = null;
            try
            {
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
            }
            catch (Exception ex)
            {
                if (!string.IsNullOrEmpty(errors))
                {
                    MessageBox.Show(errors);
                }
                MessageBox.Show(ex.Message);
                return;
            }
            finally
            {
                Util.ReleaseCom(ref compiledShader);
            }

            _tech = _fx.GetTechniqueByName("ColorTech");
            _techUint = _fx.GetTechniqueByName("ColorTechUInt");
            _fxWVP = _fx.GetVariableByName("gWorldViewProj").AsMatrix();

            try 
            {
                compiledShader = ShaderBytecode.CompileFromFile(
                    "FX/ndc_texture.fx",
                    null,
                    "fx_5_0",
                    shaderFlags,
                    EffectFlags.None,
                    null,
                    null,
                    out errors);
                _fxNdcTexture = new Effect(Device, compiledShader);
            }
            catch (Exception ex)
            {
                if (!string.IsNullOrEmpty(errors))
                {
                    MessageBox.Show(errors);
                }
                MessageBox.Show(ex.Message);
                return;
            }
            finally
            {
                Util.ReleaseCom(ref compiledShader);
            }

            // Technique for rendering the full-screen quad
            _techRenderTexture = _fxNdcTexture.GetTechniqueByName("RenderTexture");
        }

        private void BuildVertexLayout() 
        {
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


    static class Program 
    {
        static void Main(string[] args) 
        {
            Configuration.EnableObjectTracking = true;
            var app = new BoxApp(Process.GetCurrentProcess().Handle);
            if (!app.Init())
            {
                return;
            }
            
            app.Run();
        }
    }
}
