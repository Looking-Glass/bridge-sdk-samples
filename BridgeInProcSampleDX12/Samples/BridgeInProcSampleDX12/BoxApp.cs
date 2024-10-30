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

        private ShaderBytecode _mvsByteCode;
        private ShaderBytecode _mpsByteCode;

        private InputLayoutDescription _inputLayout;

        private PipelineState _pso;

        private Matrix _proj = Matrix.Identity;
        private Matrix _view = Matrix.Identity;
        private Matrix _model = Matrix.Identity;

        private float _lastTimestamp = Stopwatch.GetTimestamp();
        private float _freq = Stopwatch.Frequency;

        private float _angle = 0.0f;

        BridgeInProc.Window _bridge_window;
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
        private SharpDX.Direct3D12.DescriptorHeap _quiltRtvHeap;
        private SharpDX.Direct3D12.DescriptorHeap _quiltDsvHeap;
        SharpDX.Direct3D12.Resource _stagingResource;

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
            BuildPSO();

            // Execute the initialization commands.
            CommandList.Close();
            CommandQueue.ExecuteCommandList(CommandList);

            // Wait until initialization is complete.
            FlushCommandQueue();

            // mlc: init bridge
            if (!BridgeInProc.Controller.Initialize(@"BridgeInProcSampleDX12"))
            {
                Environment.Exit(-1);
            }

            // mlc: instance the window
            bool window_status = BridgeInProc.Controller.InstanceWindowDX(Device.NativePointer, ref _bridge_window);

            if (!window_status)
            {
                throw new Exception("");
            }

            // mlc: cache the size of the bridge output so we can decide how large to make
            // the quilt views
            BridgeInProc.Controller.GetWindowDimensions(_bridge_window, ref _bridge_window_width, ref _bridge_window_height);

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
        }

        protected override void OnResize()
        {
            base.OnResize();

            // The window resized, so update the aspect ratio and recompute the projection matrix.
            _proj = Matrix.PerspectiveFovLH(MathUtil.PiOverFour, AspectRatio, 1.0f, 1000.0f);
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

        bool first = true;

        protected override void Draw(GameTimer gt)
        {
            var timeStamp = Stopwatch.GetTimestamp();
            _angle += (float)((timeStamp - _lastTimestamp) / (double)_freq);
            _lastTimestamp = timeStamp;

            DirectCmdListAlloc.Reset();
            CommandList.Reset(DirectCmdListAlloc, _pso);
            CommandList.SetDescriptorHeaps(_descriptorHeaps.Length, _descriptorHeaps);

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

            CommandList.ResourceBarrierTransition(CurrentBackBuffer, ResourceStates.Common, ResourceStates.RenderTarget);

            CommandList.SetRenderTargets(CurrentBackBufferView, DepthStencilView);
            CommandList.SetViewport(Viewport);
            CommandList.SetScissorRectangles(ScissorRectangle);

            // Clear the back buffer and depth buffer.
            CommandList.ClearRenderTargetView(CurrentBackBufferView, Color.Black);
            CommandList.ClearDepthStencilView(DepthStencilView, ClearFlags.FlagsDepth | ClearFlags.FlagsStencil, 1.0f, 0);

            Draw();

            CommandList.ResourceBarrierTransition(CurrentBackBuffer, ResourceStates.RenderTarget, ResourceStates.Present);

            // mlc: kick the render target to common so we can access it via interop
            CommandList.ResourceBarrierTransition(_quiltRenderTarget, ResourceStates.RenderTarget, ResourceStates.Common);

            CommandList.Close();
            CommandQueue.ExecuteCommandList(CommandList);

            SwapChain.Present(0, PresentFlags.None);

            FlushCommandQueue();

            if (first)
            {
                // mlc: register the render target for use with bridge (doing it when in the common state)
                BridgeInProc.Controller.RegisterTextureDX(_bridge_window, _quiltRenderTarget.NativePointer);
                first = false;
            }

            BridgeInProc.Controller.DrawInteropQuiltTextureDX(_bridge_window, _quiltRenderTarget.NativePointer, _bridge_quilt_vx, _bridge_quilt_vy, 1.0f, 1.0f);
            //BridgeInProc.Controller.SaveTextureToFileDX(_bridge_window, @"C:\\temp\\quilt.png", _quiltRenderTarget.NativePointer);
            
            // mlc: kick the render target back to the proper start
            DirectCmdListAlloc.Reset();
            CommandList.Reset(DirectCmdListAlloc, _pso);
            CommandList.SetDescriptorHeaps(_descriptorHeaps.Length, _descriptorHeaps);

            CommandList.ResourceBarrierTransition(_quiltRenderTarget, ResourceStates.Common, ResourceStates.RenderTarget);

            CommandList.Close();
            CommandQueue.ExecuteCommandList(CommandList);

            FlushCommandQueue();
        }

        bool SaveRenderTargetToFile(SharpDX.Direct3D12.Resource resource, string filename)
        {
            if (resource == null)
            {
                return false;
            }

            if (string.IsNullOrEmpty(filename))
            {
                return false;
            }

            if (resource.Description.Format != Format.R8G8B8A8_UNorm)
            {
                return false;
            }

            DirectCmdListAlloc.Reset();
            CommandList.Reset(DirectCmdListAlloc, _pso);

            CommandList.ResourceBarrierTransition(resource,
                                      ResourceStates.Common,
                                      ResourceStates.CopySource);

            var resourceDesc = _quiltRenderTarget.Description;
            PlacedSubResourceFootprint[] footprints = new PlacedSubResourceFootprint[1];
            int[] numRows = new int[1];
            long[] rowSizes = new long[1];
            long totalBytes;

            Device.GetCopyableFootprints(ref resourceDesc, 0, 1, 0, footprints, numRows, rowSizes, out totalBytes);

            // Set up the texture copy locations
            var textureCopyLocation = new TextureCopyLocation(_quiltRenderTarget, 0);
            var bufferCopyLocation = new TextureCopyLocation(_stagingResource, footprints[0]);

            // Copy from the render target to the readback buffer
            CommandList.CopyTextureRegion(bufferCopyLocation, 0, 0, 0, textureCopyLocation, null);

            CommandList.ResourceBarrierTransition(resource,
                                      ResourceStates.CopySource,
                                      ResourceStates.Common);

            CommandList.Close();
            CommandQueue.ExecuteCommandList(CommandList);

            IntPtr mappedDataPointer = _stagingResource.Map(0, null);

            BridgeInProc.Controller.SaveImageToFile(_bridge_window, filename, mappedDataPointer, BridgeInProc.PixelFormats.RGBA, (ulong)resource.Description.Width, (ulong)resource.Description.Height);

            _stagingResource.Unmap(0);

            return true;
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
            // Shader programs typically require resources as input (constant buffers,
            // textures, samplers). The root signature defines the resources the shader
            // programs expect. If we think of the shader programs as a function, and
            // the input resources as function parameters, then the root signature can be
            // thought of as defining the function signature.

            // Root parameter can be a table, root descriptor or root constants.

            // Create a single descriptor table of CBVs.
            var cbvTable = new DescriptorRange(DescriptorRangeType.ConstantBufferView, 1, 0);

            // A root signature is an array of root parameters.
            var rootSigDesc = new RootSignatureDescription(RootSignatureFlags.AllowInputAssemblerInputLayout, new[]
            {
                new RootParameter(ShaderVisibility.Vertex, cbvTable)
            });

            _rootSignature = Device.CreateRootSignature(rootSigDesc.Serialize());
        }

        private void BuildShadersAndInputLayout()
        {
            _mvsByteCode = D3DUtil.CompileShader("Shaders\\Color.hlsl", "VS", "vs_5_0");
            _mpsByteCode = D3DUtil.CompileShader("Shaders\\Color.hlsl", "PS", "ps_5_0");

            _inputLayout = new InputLayoutDescription(new[] // TODO: API suggestion: Add params overload
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

        private void BuildPSO()
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
    }
}
