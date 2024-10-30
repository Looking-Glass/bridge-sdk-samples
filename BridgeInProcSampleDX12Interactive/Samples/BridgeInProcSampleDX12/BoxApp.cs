using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using SharpDX;
using SharpDX.Direct3D;
using SharpDX.Direct3D12;
using SharpDX.DXGI;
using Resource = SharpDX.Direct3D12.Resource;

// adapted from: https://github.com/discosultan/dx12-game-programming
namespace DX12GameProgramming
{
    [StructLayout(LayoutKind.Sequential, Pack = 4)]
    internal struct Vertex
    {
        public Vector3 Pos;
        public Vector4 Color;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 4)]
    internal struct ObjectConstants
    {
        public Matrix WorldViewProj;
    }

    internal class BoxApp : D3DApp
    {
        private RootSignature _rootSignature;
        private DescriptorHeap _cbvHeap;
        private DescriptorHeap[] _descriptorHeaps;

        private UploadBuffer<ObjectConstants> _objectCB;

        private MeshGeometry _boxGeo;
        private MeshGeometry _quadGeo;

        private ShaderBytecode _mvsByteCode;
        private ShaderBytecode _mpsByteCode;

        private ShaderBytecode _mvsColorTextureByteCode;
        private ShaderBytecode _mpsColorTextureByteCode;

        private InputLayoutDescription _inputLayout;

        private PipelineState _pso;
        private PipelineState _psoColorTexture;

        private Matrix _proj = Matrix.Identity;
        private Matrix _view = Matrix.Identity;
        private Matrix _model = Matrix.Identity;

        private float _lastTimestamp = Stopwatch.GetTimestamp();
        private float _freq = Stopwatch.Frequency;

        private float _angle = 0.0f;
        private float _rotationSpeed = 0.01f;  // Rotation speed for auto-rotation
        private bool _userControlling = false;  // Flag to detect if user is controlling rotation
        private float _userInputCooldown = 3.0f;  // Time to wait before resuming auto-rotation
        private float _timeSinceLastInput = 0.0f;  // Time elapsed since last user input
        private float _lastMouseX;
        private float _lastMouseY;
        private bool _mouseDragging = false;

        bool _lkg_display = false;

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
        SharpDX.Direct3D12.Resource _quiltRenderTarget;
        SharpDX.Direct3D12.Resource _quiltDepthStencilBuffer;
        SharpDX.Direct3D12.DescriptorHeap _quiltRtvHeap;
        SharpDX.Direct3D12.DescriptorHeap _quiltDsvHeap;
        SharpDX.Direct3D12.DescriptorHeap _srvHeap;
        SharpDX.Direct3D12.DescriptorHeap _samplerHeap;
        SharpDX.Direct3D12.Resource _stagingResource;
        SharpDX.Direct3D12.Resource _bridge_offscreen_texture;

        public BoxApp()
        {
            MainWindowCaption = "Bridge InProc C# DX12 Sample";
        }

        public override void Initialize()
        {
            base.Initialize();

            // Reset the command list to prep for initialization commands.
            CommandList.Reset(DirectCmdListAlloc, null);

            BuildDescriptorHeaps();
            BuildConstantBuffers();
            BuildRootSignature();
            BuildShadersAndInputLayout();
            BuildBoxGeometry();
            BuildQuadGeometry();
            BuildPSOs();

            // Execute the initialization commands.
            CommandList.Close();
            CommandQueue.ExecuteCommandList(CommandList);

            // Wait until initialization is complete.
            FlushCommandQueue();

            // mlc: init bridge
            if (!BridgeInProc.Controller.Initialize(@"BridgeInProcSampleDX12Interactive"))
            {
                Environment.Exit(-1);
            }

            // mlc: instance the window
            bool window_status = BridgeInProc.Controller.InstanceOffscreenWindowDX(Device.NativePointer, ref _bridge_window);

            if (window_status)
            {
                // mlc: cache the size of the bridge output so we can decide how large to make
                // the quilt views
                BridgeInProc.Controller.GetWindowDimensions(_bridge_window, ref _bridge_window_width, ref _bridge_window_height);
                BridgeInProc.Controller.GetWindowPosition(_bridge_window, ref _bridge_window_x, ref _bridge_window_y);

                // mlc: see how large we can make out render texture
                BridgeInProc.Controller.GetMaxTextureSize(_bridge_window, ref _bridge_max_texture_size);

                // mlc: dx12 puts an artifical cap on this on some cards:
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

                // mlc: create descriptor heap for offscreen
                var srvHeapDesc = new DescriptorHeapDescription
                {
                    DescriptorCount = 1,
                    Type = DescriptorHeapType.ConstantBufferViewShaderResourceViewUnorderedAccessView,
                    Flags = DescriptorHeapFlags.ShaderVisible
                };

                _srvHeap = Device.CreateDescriptorHeap(srvHeapDesc);


                // mlc: create descriptor heap for quilt RTV
                var rtvHeapDesc = new DescriptorHeapDescription
                {
                    DescriptorCount = 1,
                    Type = DescriptorHeapType.RenderTargetView,
                    Flags = DescriptorHeapFlags.None
                };
                _quiltRtvHeap = Device.CreateDescriptorHeap(rtvHeapDesc);

                // mlc: create descriptor heap for quilt DSV
                var dsvHeapDesc = new DescriptorHeapDescription
                {
                    DescriptorCount = 1,
                    Type = DescriptorHeapType.DepthStencilView,
                    Flags = DescriptorHeapFlags.None
                };

                _quiltDsvHeap = Device.CreateDescriptorHeap(dsvHeapDesc);

                // mlc: describe the render target
                var rtDesc = new ResourceDescription
                {
                    Dimension = ResourceDimension.Texture2D,
                    Alignment = 0,
                    Width = _bridge_render_texture_width,
                    Height = (int)_bridge_render_texture_height,
                    DepthOrArraySize = 1,
                    MipLevels = 1,
                    Format = BackBufferFormat,
                    SampleDescription = new SampleDescription(1, 0),
                    Layout = TextureLayout.Unknown,
                    Flags = ResourceFlags.AllowRenderTarget | ResourceFlags.AllowSimultaneousAccess
                };

                // mlc: specify the clear color for the render target
                var clearColor = new ClearValue
                {
                    Format = BackBufferFormat,
                    Color = new SharpDX.Mathematics.Interop.RawVector4(0.0f, 0.0f, 0.0f, 1.0f) // Clear to black
                };

                // mlc: create the Render Target
                _quiltRenderTarget = Device.CreateCommittedResource(
                    new HeapProperties(HeapType.Default),
                    HeapFlags.Shared,
                    rtDesc,
                    ResourceStates.RenderTarget,
                    clearColor);

                // mlc: describe the Depth/Stencil Buffer
                var dsDesc = new ResourceDescription
                {
                    Dimension = ResourceDimension.Texture2D,
                    Alignment = 0,
                    Width = _bridge_render_texture_width,
                    Height = (int)_bridge_render_texture_height,
                    DepthOrArraySize = 1,
                    MipLevels = 1,
                    Format = Format.D24_UNorm_S8_UInt, // Common depth format
                    SampleDescription = new SampleDescription(1, 0),
                    Layout = TextureLayout.Unknown,
                    Flags = ResourceFlags.AllowDepthStencil
                };

                // mlc: specify the clear value for the depth stencil buffer
                var depthClearValue = new ClearValue
                {
                    Format = Format.D24_UNorm_S8_UInt,
                    DepthStencil = new DepthStencilValue
                    {
                        Depth = 1.0f,
                        Stencil = 0
                    }
                };

                // mlc: create the Depth/Stencil Buffer
                _quiltDepthStencilBuffer = Device.CreateCommittedResource(
                    new HeapProperties(HeapType.Default),
                    HeapFlags.None,
                    dsDesc,
                    ResourceStates.DepthWrite,
                    depthClearValue);

                // mlc: create the RTV for the quilt render target
                var rtvHandle = _quiltRtvHeap.CPUDescriptorHandleForHeapStart;
                Device.CreateRenderTargetView(_quiltRenderTarget, null, rtvHandle);

                // mlc: create the DSV for the quilt depth stencil buffer
                var dsvHandle = _quiltDsvHeap.CPUDescriptorHandleForHeapStart;
                Device.CreateDepthStencilView(_quiltDepthStencilBuffer, null, dsvHandle);

                // mlc: create a readback resource to preview the render target
                var stagingDesc = ResourceDescription.Buffer(new ResourceAllocationInformation()
                {
                    SizeInBytes = _bridge_render_texture_width * _bridge_render_texture_height * 4,
                    Alignment = 0
                });

                _stagingResource = Device.CreateCommittedResource(
                        new HeapProperties(HeapType.Readback),
                        HeapFlags.None,
                        stagingDesc,
                        ResourceStates.CopyDestination);

                ClientWidth  = (int)_bridge_window_width;
                ClientHeight = (int)_bridge_window_height;

                BridgeInProc.Controller.RegisterTextureDX(_bridge_window, _quiltRenderTarget.NativePointer);

                _lkg_display = true;

                SetWindowProperties();
            }
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

        protected override void OnResize()
        {
            base.OnResize();

            // The window resized, so update the aspect ratio and recompute the projection matrix.
            _proj = Matrix.PerspectiveFovLH(MathUtil.PiOverTwo, AspectRatio, 1.0f, 1000.0f);
        }

        protected override void Update(GameTimer gt)
        {
            if (!_userControlling)
            {
                _angle += _rotationSpeed * (float)gt.DeltaTime;
            }

            _timeSinceLastInput += (float)gt.DeltaTime;
            if (_timeSinceLastInput > _userInputCooldown)
            {
                _userControlling = false;
            }

            // Apply the rotation to the model matrix
            _model = Matrix.RotationYawPitchRoll(_angle, 0.0f, _angle);
        }

        protected void Draw(float tx = 0.0f)
        {
            _model = Matrix.RotationYawPitchRoll(_angle, 0.0f, _angle);

            // Build the view matrix.
            _view = Matrix.LookAtLH(new Vector3(0, 0, 5), Vector3.Zero, Vector3.Up) * Matrix.Translation(-tx, 0.0f, 0.0f);

            var cb = new ObjectConstants
            {
                WorldViewProj = Matrix.Transpose(_model * _view * _proj)
            };

            // Update the constant buffer with the latest worldViewProj matrix.
            _objectCB.CopyData(0, ref cb);

            // Reuse the memory associated with command recording.
            // We can only reset when the associated command lists have finished execution on the GPU.
            CommandList.SetGraphicsRootSignature(_rootSignature);

            CommandList.SetVertexBuffer(0, _boxGeo.VertexBufferView);
            CommandList.SetIndexBuffer(_boxGeo.IndexBufferView);
            CommandList.PrimitiveTopology = PrimitiveTopology.TriangleList;

            CommandList.SetGraphicsRootDescriptorTable(0, _cbvHeap.GPUDescriptorHandleForHeapStart);

            CommandList.DrawIndexedInstanced(_boxGeo.IndexCount, 1, 0, 0, 0);
        }

        protected override void Draw(GameTimer gt)
        {
            var timeStamp = Stopwatch.GetTimestamp();
            _angle += (float)((timeStamp - _lastTimestamp) / (double)_freq);
            _lastTimestamp = timeStamp;

            DirectCmdListAlloc.Reset();
            CommandList.Reset(DirectCmdListAlloc, _pso);
            CommandList.SetDescriptorHeaps(_descriptorHeaps.Length, _descriptorHeaps);

            if (_lkg_display)
            {
                // mlc: draw quilt views
                float tx_offset = 0.5f;
                float tx = -(float)(_bridge_quilt_vx * _bridge_quilt_vy - 1) / 2.0f * tx_offset;

                // Get the handles from the heaps for quilt views
                var quiltRtvHandle = _quiltRtvHeap.CPUDescriptorHandleForHeapStart;
                var quiltDsvHandle = _quiltDsvHeap.CPUDescriptorHandleForHeapStart;

                CommandList.SetRenderTargets(quiltRtvHandle, quiltDsvHandle);

                CommandList.SetViewport(new ViewportF(
                    0,
                    0,
                    _bridge_render_texture_width,
                    _bridge_render_texture_height,
                    0.0f, 1.0f));

                CommandList.SetScissorRectangles(new Rectangle(
                    0,
                    0,
                    (int)_bridge_render_texture_width,
                    (int)_bridge_render_texture_height));

                // Clear the entire quilt render target and depth stencil buffer at once
                CommandList.ClearRenderTargetView(quiltRtvHandle, Color.Black, 0, null);
                CommandList.ClearDepthStencilView(quiltDsvHandle, ClearFlags.FlagsDepth, 1.0f, 0);

                for (uint y = 0; y < _bridge_quilt_vy; y++)
                {
                    for (uint x = 0; x < _bridge_quilt_vx; x++)
                    {
                        uint invertedY = _bridge_quilt_vy - 1 - y;

                        // Set the viewport and scissor rectangles for the current quilt view
                        CommandList.SetViewport(new ViewportF(
                            x * _bridge_quilt_view_width,
                            invertedY * _bridge_quilt_view_height,
                            _bridge_quilt_view_width,
                            _bridge_quilt_view_height,
                            0.0f, 1.0f));

                        // Draw the scene
                        Draw(tx);

                        tx += tx_offset;
                    }
                }

                BridgeInProc.Controller.DrawInteropQuiltTextureDX(_bridge_window, _quiltRenderTarget.NativePointer, _bridge_quilt_vx, _bridge_quilt_vy, 1.0f, 1.0f);

                if (_bridge_offscreen_texture == null)
                {
                    IntPtr nativeTexturePtr = IntPtr.Zero;

                    if (BridgeInProc.Controller.GetOffscreenWindowTextureDX(_bridge_window, out nativeTexturePtr))
                    {
                        // Cast the IntPtr to a SharpDX.Direct3D12.Resource (DX12 texture)
                        _bridge_offscreen_texture = new SharpDX.Direct3D12.Resource(nativeTexturePtr);

                        // Create a Shader Resource View (SRV) for this texture using the newly created heap
                        var srvDesc = new ShaderResourceViewDescription
                        {
                            Shader4ComponentMapping = 5768, // D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING
                            Format = _bridge_offscreen_texture.Description.Format,
                            Dimension = SharpDX.Direct3D12.ShaderResourceViewDimension.Texture2D,
                            Texture2D = new ShaderResourceViewDescription.Texture2DResource
                            {
                                MipLevels = _bridge_offscreen_texture.Description.MipLevels,
                                MostDetailedMip = 0,
                                ResourceMinLODClamp = 0.0f
                            }
                        };

                        // Use the dedicated SRV heap for this view
                        Device.CreateShaderResourceView(_bridge_offscreen_texture, srvDesc, _srvHeap.CPUDescriptorHandleForHeapStart);

                        var samplerHeapDesc = new DescriptorHeapDescription
                        {
                            DescriptorCount = 1,
                            Type = DescriptorHeapType.Sampler,
                            Flags = DescriptorHeapFlags.ShaderVisible
                        };

                        _samplerHeap = Device.CreateDescriptorHeap(samplerHeapDesc);

                        // Define the sampler description
                        var samplerDesc = new SamplerStateDescription
                        {
                            Filter = Filter.MinMagMipPoint,
                            AddressU = TextureAddressMode.Wrap,
                            AddressV = TextureAddressMode.Wrap,
                            AddressW = TextureAddressMode.Wrap,
                            ComparisonFunction = Comparison.Always,
                            BorderColor = Color.Black
                        };

                        // Create the sampler and bind it to the sampler descriptor heap
                        Device.CreateSampler(samplerDesc, _samplerHeap.CPUDescriptorHandleForHeapStart);
                    }
                }
            }

            ///////////////////////////////////////////
            CommandList.ResourceBarrierTransition(CurrentBackBuffer, ResourceStates.Common, ResourceStates.RenderTarget);

            CommandList.SetRenderTargets(CurrentBackBufferView, DepthStencilView);
            CommandList.SetViewport(Viewport);
            CommandList.SetScissorRectangles(ScissorRectangle);

            // Clear the back buffer and depth buffer.
            CommandList.ClearRenderTargetView(CurrentBackBufferView, Color.Black);
            CommandList.ClearDepthStencilView(DepthStencilView, ClearFlags.FlagsDepth | ClearFlags.FlagsStencil, 1.0f, 0);

            Draw();

            CommandList.ResourceBarrierTransition(CurrentBackBuffer, ResourceStates.RenderTarget, ResourceStates.Present);

            if (_lkg_display)
            {
                CommandList.ResourceBarrierTransition(_quiltRenderTarget, ResourceStates.RenderTarget, ResourceStates.Common);
            }

            CommandList.Close();
            CommandQueue.ExecuteCommandList(CommandList);

            FlushCommandQueue();
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

            if (_lkg_display)
            {
                // Start a new command list recording for drawing the full-screen quad
                DirectCmdListAlloc.Reset();
                CommandList.Reset(DirectCmdListAlloc, _psoColorTexture);

                CommandList.SetRenderTargets(CurrentBackBufferView, DepthStencilView);

                // Set the descriptor heaps for SRV and Sampler
                CommandList.SetDescriptorHeaps(2, new[] { _srvHeap, _samplerHeap });

                // Set the root signature that matches the quad rendering
                CommandList.SetGraphicsRootSignature(_rootSignature);

                // Set the SRV (Shader Resource View) for the quad's texture (slot 1 in the shader)
                CommandList.SetGraphicsRootDescriptorTable(1, _srvHeap.GPUDescriptorHandleForHeapStart);

                // Set the Sampler for the quad's texture (slot 0 in the shader)
                CommandList.SetGraphicsRootDescriptorTable(2, _samplerHeap.GPUDescriptorHandleForHeapStart); // Assuming your root signature is using slot 2 for the sampler

                // Set the vertex and index buffers for the full-screen quad
                CommandList.SetVertexBuffer(0, _quadGeo.VertexBufferView);
                CommandList.SetIndexBuffer(_quadGeo.IndexBufferView);
                CommandList.PrimitiveTopology = PrimitiveTopology.TriangleList;

                // Set the full-screen viewport and scissor rectangles
                CommandList.SetViewport(new ViewportF(0, 0, Viewport.Width, Viewport.Height, 0.0f, 1.0f));
                CommandList.SetScissorRectangles(new Rectangle(0, 0, (int)Viewport.Width, (int)Viewport.Height));

                // Transition the current back buffer to render target state before drawing
                CommandList.ResourceBarrierTransition(CurrentBackBuffer, ResourceStates.Present, ResourceStates.RenderTarget);

                // Clear the render target again (if necessary)
                CommandList.ClearRenderTargetView(CurrentBackBufferView, Color.Black);

                // Draw the full-screen quad
                CommandList.DrawIndexedInstanced(_quadGeo.IndexCount, 1, 0, 0, 0);

                // Transition the back buffer to present state for final output
                CommandList.ResourceBarrierTransition(CurrentBackBuffer, ResourceStates.RenderTarget, ResourceStates.Present);

                // Close the command list for execution
                CommandList.Close();

                // Execute the command list
                CommandQueue.ExecuteCommandList(CommandList);

                // Final flush of the command queue
                FlushCommandQueue();
            }

            SwapChain.Present(0, PresentFlags.None);

            if (_lkg_display)
            {
                // mlc: kick the render target back to the proper state
                DirectCmdListAlloc.Reset();
                CommandList.Reset(DirectCmdListAlloc, _pso);
                CommandList.SetDescriptorHeaps(_descriptorHeaps.Length, _descriptorHeaps);

                CommandList.ResourceBarrierTransition(_quiltRenderTarget, ResourceStates.Common, ResourceStates.RenderTarget);

                CommandList.Close();
                CommandQueue.ExecuteCommandList(CommandList);

                FlushCommandQueue();
            }
        }

        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                // mlc: make sure to tell bridge we are done with the texture
                BridgeInProc.Controller.UnregisterTextureDX(_bridge_window, _quiltRenderTarget.NativePointer);
                BridgeInProc.Controller.Uninitialize();

                _stagingResource?.Dispose();
                _quiltRtvHeap?.Dispose();
                _quiltDsvHeap?.Dispose();
                _quiltRenderTarget?.Dispose();
                _quiltDepthStencilBuffer?.Dispose();
                _rootSignature?.Dispose();
                _cbvHeap?.Dispose();
                _objectCB?.Dispose();
                _boxGeo?.Dispose();
                _pso?.Dispose();
                _quiltRenderTarget?.Dispose();
                _quiltDepthStencilBuffer?.Dispose();
            }

            base.Dispose(disposing);
        }

        private void BuildDescriptorHeaps()
        {
            var cbvHeapDesc = new DescriptorHeapDescription
            {
                DescriptorCount = 1,
                Type = DescriptorHeapType.ConstantBufferViewShaderResourceViewUnorderedAccessView,
                Flags = DescriptorHeapFlags.ShaderVisible,
                NodeMask = 0
            };

            _cbvHeap = Device.CreateDescriptorHeap(cbvHeapDesc);

            _descriptorHeaps = new[] { _cbvHeap };
        }

        private void BuildConstantBuffers()
        {
            int sizeInBytes = D3DUtil.CalcConstantBufferByteSize<ObjectConstants>();

            _objectCB = new UploadBuffer<ObjectConstants>(Device, 1, true);

            var cbvDesc = new ConstantBufferViewDescription
            {
                BufferLocation = _objectCB.Resource.GPUVirtualAddress,
                SizeInBytes = sizeInBytes
            };
            CpuDescriptorHandle cbvHeapHandle = _cbvHeap.CPUDescriptorHandleForHeapStart;
            Device.CreateConstantBufferView(cbvDesc, cbvHeapHandle);
        }

        private void BuildRootSignature()
        {
            // Descriptor table for the texture (SRV at register t0)
            var srvTable = new DescriptorRange(DescriptorRangeType.ShaderResourceView, 1, 0);

            // Descriptor table for the sampler (sampler at register s0)
            var samplerTable = new DescriptorRange(DescriptorRangeType.Sampler, 1, 0);

            // Create the root signature with two descriptor tables: one for SRV and one for sampler
            var rootSigDesc = new RootSignatureDescription(RootSignatureFlags.AllowInputAssemblerInputLayout,
                new[]
                {
                    // Constant buffer for the per-object data (e.g., transformation matrix)
                    new RootParameter(ShaderVisibility.Vertex, new DescriptorRange(DescriptorRangeType.ConstantBufferView, 1, 0)),
                    // Texture SRV table for pixel shader
                    new RootParameter(ShaderVisibility.Pixel, srvTable),
                    // Sampler table for pixel shader
                    new RootParameter(ShaderVisibility.Pixel, samplerTable)
                });

            _rootSignature = Device.CreateRootSignature(rootSigDesc.Serialize());
        }

        private void BuildShadersAndInputLayout()
        {
            _mvsByteCode = D3DUtil.CompileShader("Shaders\\Color.hlsl", "VS", "vs_5_0");
            _mpsByteCode = D3DUtil.CompileShader("Shaders\\Color.hlsl", "PS", "ps_5_0");

            _mvsColorTextureByteCode = D3DUtil.CompileShader("Shaders\\Color_Texture.hlsl", "VS", "vs_5_0");
            _mpsColorTextureByteCode = D3DUtil.CompileShader("Shaders\\Color_Texture.hlsl", "PS", "ps_5_0");

            _inputLayout = new InputLayoutDescription(new[]
            {
                new InputElement("POSITION", 0, Format.R32G32B32_Float, 0, 0),
                new InputElement("COLOR", 0, Format.R32G32B32A32_Float, 12, 0)
            });
        }

        private void BuildBoxGeometry()
        {
            Vertex[] vertices =
            {
                // Red front face
                new Vertex { Pos = new Vector3(-1.0f, -1.0f, -1.0f), Color = Color.Red.ToVector4() },
                new Vertex { Pos = new Vector3(-1.0f,  1.0f, -1.0f), Color = Color.Red.ToVector4() },
                new Vertex { Pos = new Vector3( 1.0f,  1.0f, -1.0f), Color = Color.Red.ToVector4() },
                new Vertex { Pos = new Vector3( 1.0f, -1.0f, -1.0f), Color = Color.Red.ToVector4() },

                // Red back face
                new Vertex { Pos = new Vector3(-1.0f, -1.0f,  1.0f), Color = Color.Red.ToVector4() },
                new Vertex { Pos = new Vector3(-1.0f,  1.0f,  1.0f), Color = Color.Red.ToVector4() },
                new Vertex { Pos = new Vector3( 1.0f,  1.0f,  1.0f), Color = Color.Red.ToVector4() },
                new Vertex { Pos = new Vector3( 1.0f, -1.0f,  1.0f), Color = Color.Red.ToVector4() },

                // Green left face
                new Vertex { Pos = new Vector3(-1.0f, -1.0f, -1.0f), Color = Color.Green.ToVector4() },
                new Vertex { Pos = new Vector3(-1.0f,  1.0f, -1.0f), Color = Color.Green.ToVector4() },
                new Vertex { Pos = new Vector3(-1.0f,  1.0f,  1.0f), Color = Color.Green.ToVector4() },
                new Vertex { Pos = new Vector3(-1.0f, -1.0f,  1.0f), Color = Color.Green.ToVector4() },

                // Green right face
                new Vertex { Pos = new Vector3( 1.0f, -1.0f, -1.0f), Color = Color.Green.ToVector4() },
                new Vertex { Pos = new Vector3( 1.0f,  1.0f, -1.0f), Color = Color.Green.ToVector4() },
                new Vertex { Pos = new Vector3( 1.0f,  1.0f,  1.0f), Color = Color.Green.ToVector4() },
                new Vertex { Pos = new Vector3( 1.0f, -1.0f,  1.0f), Color = Color.Green.ToVector4() },

                // Blue top face
                new Vertex { Pos = new Vector3(-1.0f,  1.0f, -1.0f), Color = Color.Blue.ToVector4() },
                new Vertex { Pos = new Vector3(-1.0f,  1.0f,  1.0f), Color = Color.Blue.ToVector4() },
                new Vertex { Pos = new Vector3( 1.0f,  1.0f,  1.0f), Color = Color.Blue.ToVector4() },
                new Vertex { Pos = new Vector3( 1.0f,  1.0f, -1.0f), Color = Color.Blue.ToVector4() },

                // Blue bottom face
                new Vertex { Pos = new Vector3(-1.0f, -1.0f, -1.0f), Color = Color.Blue.ToVector4() },
                new Vertex { Pos = new Vector3(-1.0f, -1.0f,  1.0f), Color = Color.Blue.ToVector4() },
                new Vertex { Pos = new Vector3( 1.0f, -1.0f,  1.0f), Color = Color.Blue.ToVector4() },
                new Vertex { Pos = new Vector3( 1.0f, -1.0f, -1.0f), Color = Color.Blue.ToVector4() }
            };

            short[] indices =
            {
                // Red front face
                0, 1, 2,
                0, 2, 3,

                // Red back face
                4, 6, 5,
                4, 7, 6,

                // Green left face
                10, 9, 8,
                11, 10, 8,

                // Green right face
                13, 14, 12,
                14, 15, 12,

                // Blue top face
                17, 18, 16,
                18, 19, 16,

                // Blue bottom face
                22, 21, 20,
                23, 22, 20
            };

            _boxGeo = MeshGeometry.New(Device, CommandList, vertices, indices);
        }

        private void BuildQuadGeometry()
        {
            // Fullscreen quad in NDC with positions in the [-1, 1] range and texture coordinates
            Vertex[] vertices =
            {
                // Top-left
                new Vertex { Pos = new Vector3(-1.0f,  1.0f, 0.0f), Color = Color.White.ToVector4() },
        
                // Bottom-left
                new Vertex { Pos = new Vector3(-1.0f, -1.0f, 0.0f), Color = Color.White.ToVector4()},
        
                // Bottom-right
                new Vertex { Pos = new Vector3( 1.0f, -1.0f, 0.0f), Color = Color.White.ToVector4() },
        
                // Top-right
                new Vertex { Pos = new Vector3( 1.0f,  1.0f, 0.0f), Color = Color.White.ToVector4() }
            };

            // Indices for two triangles to form the quad
            short[] indices =
            {
                0, 2, 1,  // First triangle
                0, 3, 2   // Second triangle
            };

            // Create the quad geometry
            _quadGeo = MeshGeometry.New(Device, CommandList, vertices, indices);
        }

        private void BuildPSOs()
        {
            {
                var psoDesc = new GraphicsPipelineStateDescription
                {
                    InputLayout = _inputLayout,
                    RootSignature = _rootSignature,
                    VertexShader = _mvsByteCode,
                    PixelShader = _mpsByteCode,
                    RasterizerState = RasterizerStateDescription.Default(),
                    BlendState = BlendStateDescription.Default(),
                    DepthStencilState = DepthStencilStateDescription.Default(),
                    SampleMask = int.MaxValue,
                    PrimitiveTopologyType = PrimitiveTopologyType.Triangle,
                    RenderTargetCount = 1,
                    SampleDescription = new SampleDescription(MsaaCount, MsaaQuality),
                    DepthStencilFormat = DepthStencilFormat
                };
                psoDesc.RenderTargetFormats[0] = BackBufferFormat;

                _pso = Device.CreateGraphicsPipelineState(psoDesc);
            }

            {
                var psoDesc = new GraphicsPipelineStateDescription
                {
                    InputLayout = _inputLayout,
                    RootSignature = _rootSignature,
                    VertexShader = _mvsColorTextureByteCode,
                    PixelShader = _mpsColorTextureByteCode,
                    RasterizerState = RasterizerStateDescription.Default(),
                    BlendState = BlendStateDescription.Default(),
                    DepthStencilState = DepthStencilStateDescription.Default(),
                    SampleMask = int.MaxValue,
                    PrimitiveTopologyType = PrimitiveTopologyType.Triangle,
                    RenderTargetCount = 1,
                    SampleDescription = new SampleDescription(MsaaCount, MsaaQuality),
                    DepthStencilFormat = DepthStencilFormat
                };
                psoDesc.RenderTargetFormats[0] = BackBufferFormat;

                _psoColorTexture = Device.CreateGraphicsPipelineState(psoDesc);
            }
        }

        protected override void OnMouseDown(MouseButtons button, Point location)
        {
            if (button == MouseButtons.Left)
            {
                _mouseDragging = true;
                _lastMouseX = location.X;
                _lastMouseY = location.Y;
            }
        }

        protected override void OnMouseUp(MouseButtons button, Point location)
        {
            if (button == MouseButtons.Left)
            {
                _mouseDragging = false;
                _userControlling = false;
            }
        }

        protected override void OnMouseMove(MouseButtons button, Point location)
        {
            if (_mouseDragging)
            {
                float deltaX = location.X - _lastMouseX;

                // Apply user rotation based on mouse movement
                _angle += deltaX * 0.01f;

                // Update the last mouse position
                _lastMouseX = location.X;
                _lastMouseY = location.Y;

                // User is controlling the rotation
                _userControlling = true;
                _timeSinceLastInput = 0.0f;  // Reset input cooldown timer
            }
        }
    }
}
