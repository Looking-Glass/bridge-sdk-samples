using System;
using System.Runtime.InteropServices;
using AppKit;
using Foundation;
using Metal;
using MetalKit;
using System.Numerics;
using System.Diagnostics;
using BridgeSDK;
using CoreMedia;
using ObjCRuntime;
using CoreGraphics;
using ImageIO;
using ModelIO;

namespace DrawCube
{
    public partial class GameViewController : NSViewController, IMTKViewDelegate
    {
        Vector4[] vertexData = new Vector4[]
        {
                  new Vector4(-1.0f, -1.0f, -1.0f, 1.0f), new Vector4(1.0f, 0.0f, 0.0f, 1.0f), // Front
                  new Vector4( 1.0f,  1.0f, -1.0f, 1.0f), new Vector4(1.0f, 0.0f, 0.0f, 1.0f),
                  new Vector4(-1.0f,  1.0f, -1.0f, 1.0f), new Vector4(1.0f, 0.0f, 0.0f, 1.0f),
                  new Vector4(-1.0f, -1.0f, -1.0f, 1.0f), new Vector4(1.0f, 0.0f, 0.0f, 1.0f),
                  new Vector4( 1.0f, -1.0f, -1.0f, 1.0f), new Vector4(1.0f, 0.0f, 0.0f, 1.0f),
                  new Vector4( 1.0f,  1.0f, -1.0f, 1.0f), new Vector4(1.0f, 0.0f, 0.0f, 1.0f),

                  new Vector4(-1.0f, -1.0f,  1.0f, 1.0f), new Vector4(0.0f, 1.0f, 0.0f, 1.0f), // BACK
                  new Vector4(-1.0f,  1.0f,  1.0f, 1.0f), new Vector4(0.0f, 1.0f, 0.0f, 1.0f),
                  new Vector4( 1.0f,  1.0f,  1.0f, 1.0f), new Vector4(0.0f, 1.0f, 0.0f, 1.0f),
                  new Vector4(-1.0f, -1.0f,  1.0f, 1.0f), new Vector4(0.0f, 1.0f, 0.0f, 1.0f),
                  new Vector4( 1.0f,  1.0f,  1.0f, 1.0f), new Vector4(0.0f, 1.0f, 0.0f, 1.0f),
                  new Vector4( 1.0f, -1.0f,  1.0f, 1.0f), new Vector4(0.0f, 1.0f, 0.0f, 1.0f),

                  new Vector4(-1.0f, 1.0f, -1.0f,  1.0f), new Vector4(0.0f, 0.0f, 1.0f, 1.0f), // Top
                  new Vector4( 1.0f, 1.0f,  1.0f,  1.0f), new Vector4(0.0f, 0.0f, 1.0f, 1.0f),
                  new Vector4(-1.0f, 1.0f,  1.0f,  1.0f), new Vector4(0.0f, 0.0f, 1.0f, 1.0f),
                  new Vector4(-1.0f, 1.0f, -1.0f,  1.0f), new Vector4(0.0f, 0.0f, 1.0f, 1.0f),
                  new Vector4( 1.0f, 1.0f, -1.0f,  1.0f), new Vector4(0.0f, 0.0f, 1.0f, 1.0f),
                  new Vector4( 1.0f, 1.0f,  1.0f,  1.0f), new Vector4(0.0f, 0.0f, 1.0f, 1.0f),

                  new Vector4(-1.0f,-1.0f, -1.0f,  1.0f), new Vector4(1.0f, 1.0f, 0.0f, 1.0f), // Bottom
                  new Vector4(-1.0f,-1.0f,  1.0f,  1.0f), new Vector4(1.0f, 1.0f, 0.0f, 1.0f),
                  new Vector4( 1.0f,-1.0f,  1.0f,  1.0f), new Vector4(1.0f, 1.0f, 0.0f, 1.0f),
                  new Vector4(-1.0f,-1.0f, -1.0f,  1.0f), new Vector4(1.0f, 1.0f, 0.0f, 1.0f),
                  new Vector4( 1.0f,-1.0f,  1.0f,  1.0f), new Vector4(1.0f, 1.0f, 0.0f, 1.0f),
                  new Vector4( 1.0f,-1.0f, -1.0f,  1.0f), new Vector4(1.0f, 1.0f, 0.0f, 1.0f),

                  new Vector4(-1.0f, -1.0f, -1.0f, 1.0f), new Vector4(1.0f, 0.0f, 1.0f, 1.0f), // Left
                  new Vector4(-1.0f,  1.0f,  1.0f, 1.0f), new Vector4(1.0f, 0.0f, 1.0f, 1.0f),
                  new Vector4(-1.0f, -1.0f,  1.0f, 1.0f), new Vector4(1.0f, 0.0f, 1.0f, 1.0f),
                  new Vector4(-1.0f, -1.0f, -1.0f, 1.0f), new Vector4(1.0f, 0.0f, 1.0f, 1.0f),
                  new Vector4(-1.0f,  1.0f, -1.0f, 1.0f), new Vector4(1.0f, 0.0f, 1.0f, 1.0f),
                  new Vector4(-1.0f,  1.0f,  1.0f, 1.0f), new Vector4(1.0f, 0.0f, 1.0f, 1.0f),

                  new Vector4( 1.0f, -1.0f, -1.0f, 1.0f), new Vector4(0.0f, 1.0f, 1.0f, 1.0f), // Right
                  new Vector4( 1.0f, -1.0f,  1.0f, 1.0f), new Vector4(0.0f, 1.0f, 1.0f, 1.0f),
                  new Vector4( 1.0f,  1.0f,  1.0f, 1.0f), new Vector4(0.0f, 1.0f, 1.0f, 1.0f),
                  new Vector4( 1.0f, -1.0f, -1.0f, 1.0f), new Vector4(0.0f, 1.0f, 1.0f, 1.0f),
                  new Vector4( 1.0f,  1.0f,  1.0f, 1.0f), new Vector4(0.0f, 1.0f, 1.0f, 1.0f),
                  new Vector4( 1.0f,  1.0f, -1.0f, 1.0f), new Vector4(0.0f, 1.0f, 1.0f, 1.0f),
        };

        // view
        MTKView mtkView;

        // renderer
        IMTLDevice device;
        IMTLCommandQueue commandQueue;
        IMTLLibrary defaultLibrary;
        IMTLRenderPipelineState pipelineState;
        IMTLDepthStencilState depthState;
        IMTLBuffer vertexBuffer;
        IMTLBuffer constantBuffer;

        System.Diagnostics.Stopwatch clock;
        Matrix4x4 proj, view;

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
        float _scaling = GetScaleFactor();

        IMTLBuffer[] _constantBuffers;
        int _constantBufferIndex = 0;
        int _constantBufferCount = 0;

        IMTLTexture _renderTexture;
        IntPtr      _renderTextureIOSurface;
        IMTLTexture _depthTexture;

        public static class MacInterop
        {
            private const string ObjectiveCLib = "/usr/lib/libobjc.dylib";

            [DllImport(ObjectiveCLib, EntryPoint = "objc_getClass")]
            public static extern IntPtr GetClass(string className);

            [DllImport(ObjectiveCLib, EntryPoint = "sel_registerName")]
            public static extern IntPtr GetSelector(string selectorName);

            [DllImport(ObjectiveCLib, EntryPoint = "objc_msgSend")]
            public static extern IntPtr SendMsg(IntPtr receiver, IntPtr selector);

            [DllImport(ObjectiveCLib, EntryPoint = "objc_msgSend")]
            public static extern double SendMsgDouble(IntPtr receiver, IntPtr selector);
        }

        public static float GetScaleFactor()
        {
            var classNSScreen = MacInterop.GetClass("NSScreen");
            var selMainScreen = MacInterop.GetSelector("mainScreen");
            var mainScreen = MacInterop.SendMsg(classNSScreen, selMainScreen);

            if (mainScreen == IntPtr.Zero)
                return 1.0f; // Default to 1.0 if the main screen is not detected

            var selBackingScaleFactor = MacInterop.GetSelector("backingScaleFactor");
            var scaleFactor = MacInterop.SendMsgDouble(mainScreen, selBackingScaleFactor);

            return (float)scaleFactor;
        }

        public GameViewController(IntPtr handle)
            : base(handle)
        {
        }

        public override void ViewDidLoad()
        {
            base.ViewDidLoad();

            // Set the view to use the default device
            device = MTLDevice.SystemDefault;

            if (device == null)
            {
                Console.WriteLine("Metal is not supported on this device");
                View = new NSView(View.Frame);
            }

            // Create a new command queue
            commandQueue = device.CreateCommandQueue();

            // Load all the shader files with a metal file extension in the project
            defaultLibrary = device.CreateDefaultLibrary();

            // Setup view
            mtkView = (MTKView)View;
            mtkView.Delegate = this;
            mtkView.Device = device;

            mtkView.SampleCount = 1;
            mtkView.DepthStencilPixelFormat = MTLPixelFormat.Depth32Float_Stencil8;
            mtkView.ColorPixelFormat = MTLPixelFormat.BGRA8Unorm;
            mtkView.PreferredFramesPerSecond = 60;
            mtkView.ClearColor = new MTLClearColor(0.0f, 0.0f, 0.0f, 1.0f);

            // Load the vertex program into the library
            IMTLFunction vertexProgram = defaultLibrary.CreateFunction("cube_vertex");

            // Load the fragment program into the library
            IMTLFunction fragmentProgram = defaultLibrary.CreateFunction("cube_fragment");

            // Create a vertex descriptor from the MTKMesh       
            MTLVertexDescriptor vertexDescriptor = new MTLVertexDescriptor();
            vertexDescriptor.Attributes[0].Format = MTLVertexFormat.Float4;
            vertexDescriptor.Attributes[0].BufferIndex = 0;
            vertexDescriptor.Attributes[0].Offset = 0;
            vertexDescriptor.Attributes[1].Format = MTLVertexFormat.Float4;
            vertexDescriptor.Attributes[1].BufferIndex = 0;
            vertexDescriptor.Attributes[1].Offset = 4 * sizeof(float);                      

            vertexDescriptor.Layouts[0].Stride = 8 * sizeof(float);
            vertexDescriptor.Layouts[0].StepRate = 1;
            vertexDescriptor.Layouts[0].StepFunction = MTLVertexStepFunction.PerVertex;

            vertexBuffer = device.CreateBuffer(vertexData, MTLResourceOptions.CpuCacheModeDefault);// (MTLResourceOptions)0);

            this.clock = new System.Diagnostics.Stopwatch();
            clock.Start();

            this.view = CreateLookAt(new Vector3(0, 0, 5), new Vector3(0, 0, 0), Vector3.UnitY);
            var aspect = (float)(View.Bounds.Size.Width.Value / View.Bounds.Size.Height.Value);
            proj = Matrix4x4.CreatePerspectiveFieldOfView((float)Math.PI / 4, aspect, 0.01f, 100.0f);

            constantBuffer = device.CreateBuffer(64, MTLResourceOptions.CpuCacheModeDefault);

            // Create a reusable pipeline state
            var pipelineStateDescriptor = new MTLRenderPipelineDescriptor
            {
                SampleCount = mtkView.SampleCount,
                VertexFunction = vertexProgram,
                FragmentFunction = fragmentProgram,
                VertexDescriptor = vertexDescriptor,
                DepthAttachmentPixelFormat = mtkView.DepthStencilPixelFormat,
                StencilAttachmentPixelFormat = mtkView.DepthStencilPixelFormat
            };

            pipelineStateDescriptor.ColorAttachments[0].PixelFormat = mtkView.ColorPixelFormat;

            NSError error;
            pipelineState = device.CreateRenderPipelineState(pipelineStateDescriptor, out error);
            if (pipelineState == null)
                Console.WriteLine("Failed to created pipeline state, error {0}", error);

            var depthStateDesc = new MTLDepthStencilDescriptor
            {
                DepthCompareFunction = MTLCompareFunction.Less,
                DepthWriteEnabled = true
            };

            depthState = device.CreateDepthStencilState(depthStateDesc);

            if (!BridgeSDK.Controller.InitializeWithPath(@"BridgeSDKSampleOSX", "/Users/matty/git/LookingGlassBridge/build/LookingGlassBridge.app/Contents/MacOS"))
            {
                Environment.Exit(-1);
            }

            // mlc: instance the window
            bool window_status = BridgeSDK.Controller.InstanceWindowMetal(device.Handle, ref _bridge_window);

            if (!window_status)
            {
                throw new Exception("Tried to create an SDK Bridge Window without a client GL context active!");
            }

            // mlc: cache the size of the bridge output so we can decide how large to make
            // the quilt views
            BridgeSDK.Controller.GetWindowDimensions(_bridge_window, ref _bridge_window_width, ref _bridge_window_height);

            // mlc: see how large we can make out render texture
            BridgeSDK.Controller.GetMaxTextureSize(_bridge_window, ref _bridge_max_texture_size);

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

            InitializeBuffers(_bridge_quilt_vx * _bridge_quilt_vy + 1);

            var textureDescriptor = MTLTextureDescriptor.CreateTexture2DDescriptor(MTLPixelFormat.RGBA8Unorm, _bridge_render_texture_width, _bridge_render_texture_height, false);
            textureDescriptor.StorageMode = MTLStorageMode.Private;
            textureDescriptor.Usage = MTLTextureUsage.RenderTarget | MTLTextureUsage.ShaderRead;

            // mlc: must use the controller method to create the texture so it has a backing IO surface
            IntPtr metalTexturePtr = IntPtr.Zero;
            BridgeSDK.Controller.CreateMetalTextureWithIOSurface(_bridge_window, textureDescriptor.Handle, out metalTexturePtr);
            _renderTexture = Runtime.GetINativeObject<IMTLTexture>(metalTexturePtr, owns: false);

            var depthTextureDescriptor = MTLTextureDescriptor.CreateTexture2DDescriptor(MTLPixelFormat.Depth32Float, _bridge_render_texture_width, _bridge_render_texture_height, false);
            depthTextureDescriptor.StorageMode = MTLStorageMode.Private;
            depthTextureDescriptor.Usage = MTLTextureUsage.RenderTarget;

            _depthTexture = device.CreateTexture(depthTextureDescriptor);
        }

        public void InitializeBuffers(uint count)
        {
            _constantBufferCount = (int)count;

            // Assuming you have a 'device' variable of type IMTLDevice
            nuint rawsize = (nuint)Marshal.SizeOf<Matrix4x4>();
            _constantBuffers = new IMTLBuffer[_constantBufferCount];
            for (int i = 0; i < _constantBufferCount; i++)
            {
                _constantBuffers[i] = device.CreateBuffer(rawsize, MTLResourceOptions.StorageModeShared);
            }
        }

        public void DrawableSizeWillChange(MTKView view, CoreGraphics.CGSize size)
        {

        }

        public void Draw(MTKView view, IMTLCommandBuffer commandBuffer, IMTLRenderCommandEncoder renderEncoder, MTLViewport viewport, float tx)
        {
            // Update transformation matrices
            var time = clock.ElapsedMilliseconds / 1000.0f;
            var viewProj = Matrix4x4.Multiply(Matrix4x4.Multiply(this.view, Matrix4x4.CreateTranslation(tx, 0.0f, 0.0f)), this.proj);
            var worldViewProj = Matrix4x4.CreateRotationX(time) * Matrix4x4.CreateRotationY(time * 2) * Matrix4x4.CreateRotationZ(time * .7f) * viewProj;
            worldViewProj = Matrix4x4.Transpose(worldViewProj);

            // Prepare uniform data using the current buffer
            int rawsize = Marshal.SizeOf<Matrix4x4>();
            var rawdata = new byte[rawsize];
            GCHandle pinnedUniforms = GCHandle.Alloc(worldViewProj, GCHandleType.Pinned);
            IntPtr ptr = pinnedUniforms.AddrOfPinnedObject();
            Marshal.Copy(ptr, rawdata, 0, rawsize);
            pinnedUniforms.Free();

            var currentBuffer = _constantBuffers[_constantBufferIndex];
            Marshal.Copy(rawdata, 0, currentBuffer.Contents, rawsize);

            // Set context state
            renderEncoder.SetDepthStencilState(depthState);
            renderEncoder.SetRenderPipelineState(pipelineState);
            renderEncoder.SetVertexBuffer(vertexBuffer, 0, 0);
            renderEncoder.SetVertexBuffer(currentBuffer, 0, 1);
            renderEncoder.SetViewport(viewport);

            // Issue the draw call
            renderEncoder.DrawPrimitives(MTLPrimitiveType.Triangle, 0, (nuint)vertexData.Length / 2);
        }

        MTLRenderPassDescriptor CreateQuiltRenderPassDescriptor()
        {
            // Create and configure the color attachment
            var colorAttachment = new MTLRenderPassColorAttachmentDescriptor
            {
                Texture = _renderTexture,
                LoadAction = MTLLoadAction.Clear,
                StoreAction = MTLStoreAction.Store,
                ClearColor = new MTLClearColor(0, 0, 0, 1) // Clear to black
            };

            // Create and configure the depth attachment
            var depthAttachment = new MTLRenderPassDepthAttachmentDescriptor
            {
                Texture = _depthTexture,
                LoadAction = MTLLoadAction.Clear,
                StoreAction = MTLStoreAction.DontCare,
                ClearDepth = 1.0
            };

            // Create the render pass descriptor
            var renderPassDescriptor = new MTLRenderPassDescriptor
            {
                ColorAttachments = { [0] = colorAttachment },
                DepthAttachment = depthAttachment
            };

            return renderPassDescriptor;
        }

        public void Draw(MTKView view)
        {
            var commandBuffer = commandQueue.CommandBuffer();

            // Render to primary view
            var primaryRenderPassDescriptor = view.CurrentRenderPassDescriptor;
            var primaryRenderEncoder = commandBuffer.CreateRenderCommandEncoder(primaryRenderPassDescriptor);
            MTLViewport primaryViewport = new MTLViewport(0, 0, _scaling * View.Bounds.Size.Width.Value, _scaling * View.Bounds.Size.Height.Value, 0, 1);
            _constantBufferIndex = 0;
            Draw(view, commandBuffer, primaryRenderEncoder, primaryViewport, 0.0f);
            primaryRenderEncoder.EndEncoding(); // End encoding for primary head

            // Draw quilt views to the render texture
            // Create and configure the render pass descriptor for quilt views
            var quiltRenderPassDescriptor = CreateQuiltRenderPassDescriptor();
            var quiltRenderEncoder = commandBuffer.CreateRenderCommandEncoder(quiltRenderPassDescriptor);

            float tx_offset = 0.01f;
            float tx = -(float)(_bridge_quilt_vx * _bridge_quilt_vy - 1) / 2.0f * tx_offset;

            for (uint y = 0; y < _bridge_quilt_vy; y++)
            {
                for (uint x = 0; x < _bridge_quilt_vx; x++)
                {
                    // Calculate the Y-coordinate so the first view is at the bottom left
                    uint yPos = _bridge_render_texture_height - (y + 1) * _bridge_quilt_view_height;

                    MTLViewport quiltViewport = new MTLViewport(
                        x * _bridge_quilt_view_width, // X-coordinate
                        yPos,                        // Y-coordinate
                        _bridge_quilt_view_width,    // Width
                        _bridge_quilt_view_height,   // Height
                        0, 1);                       // Z-range

                    // Increment and wrap the buffer index
                    _constantBufferIndex = (_constantBufferIndex + 1) % _constantBufferCount;

                    Draw(view, commandBuffer, quiltRenderEncoder, quiltViewport, tx);
                    tx += tx_offset;
                }
            }

            quiltRenderEncoder.EndEncoding();

            // Present and commit the frame
            commandBuffer.PresentDrawable(view.CurrentDrawable);
            commandBuffer.Commit();

            commandBuffer.WaitUntilCompleted();

            BridgeSDK.Controller.CopyMetalTexture(_bridge_window, _renderTexture.Handle, _renderTexture.Handle);

            //BridgeSDK.Controller.SaveMetalTextureToFile(_bridge_window, @"/Users/matty/rendertarget.png", _renderTexture.Handle, PixelFormats.RGBA, _bridge_render_texture_width, _bridge_render_texture_height);

            BridgeSDK.Controller.DrawInteropQuiltTextureMetal(_bridge_window, _renderTexture.Handle, _bridge_quilt_vx, _bridge_quilt_vy, 1.0f, 1.0f);
        }

        public void SaveRenderTargetToPNG(IMTLTexture renderTexture, string filePath)
        {
            var width = (nint)renderTexture.Width;
            var height = (nint)renderTexture.Height;
            var pixelData = new byte[width * height * 4]; // Assuming 4 bytes per pixel (RGBA)

            var textureDescriptor = MTLTextureDescriptor.CreateTexture2DDescriptor(MTLPixelFormat.RGBA8Unorm, (nuint)width, (nuint)height, false);
            textureDescriptor.StorageMode = MTLStorageMode.Shared;
            textureDescriptor.Usage = MTLTextureUsage.ShaderRead | MTLTextureUsage.ShaderWrite;
            var tempTexture = device.CreateTexture(textureDescriptor);

            var commandBuffer = commandQueue.CommandBuffer();
            var blitPassDescriptor = MTLBlitPassDescriptor.Create(); // Create default blit pass descriptor
            var blitEncoder = commandBuffer.CreateBlitCommandEncoder(blitPassDescriptor); // Use the descriptor
            blitEncoder.CopyFromTexture(renderTexture, 0, 0, new MTLOrigin(0, 0, 0), new MTLSize(width, height, 1), tempTexture, 0, 0, new MTLOrigin(0, 0, 0));
            blitEncoder.Synchronize(tempTexture);
            blitEncoder.EndEncoding();

            commandBuffer.Commit();
            commandBuffer.WaitUntilCompleted();

            // Get bytes from the texture
            unsafe
            {
                fixed (byte* pixelDataPtr = pixelData)
                {
                    var region = new MTLRegion(new MTLOrigin(0, 0, 0), new MTLSize(width, height, 1));
                    tempTexture.GetBytes((IntPtr)pixelDataPtr, (nuint)width * 4, region, 0);
                }
            }

            // Create CGImage from the buffer
            var colorSpace = CGColorSpace.CreateDeviceRGB();
            var bitmapInfo = CGBitmapFlags.ByteOrder32Big | CGBitmapFlags.PremultipliedLast;
            var context = new CGBitmapContext(pixelData, width, height, 8, width * 4, colorSpace, bitmapInfo);
            var cgImage = context.ToImage();

            // Save the CGImage as a PNG file
            var url = new NSUrl(filePath, false);
            var destination = CGImageDestination.Create(url, MobileCoreServices.UTType.PNG, 1);
            destination.AddImage(cgImage);
            destination.Close();
        }

        #region Helpers

        public static Matrix4x4 CreateLookAt(Vector3 position, Vector3 target, Vector3 upVector)
        {
            Matrix4x4 matrix;
            CreateLookAt(ref position, ref target, ref upVector, out matrix);

            return matrix;
        }

        public static void CreateLookAt(ref Vector3 position, ref Vector3 target, ref Vector3 upVector, out Matrix4x4 result)
        {
            Vector3 vector1 = Vector3.Normalize(position - target);
            Vector3 vector2 = Vector3.Normalize(Vector3.Cross(upVector, vector1));
            Vector3 vector3 = Vector3.Cross(vector1, vector2);

            result = Matrix4x4.Identity;
            result.M11 = vector2.X;
            result.M12 = vector3.X;
            result.M13 = vector1.X;
            result.M14 = 0f;
            result.M21 = vector2.Y;
            result.M22 = vector3.Y;
            result.M23 = vector1.Y;
            result.M24 = 0f;
            result.M31 = vector2.Z;
            result.M32 = vector3.Z;
            result.M33 = vector1.Z;
            result.M34 = 0f;
            result.M41 = -Vector3.Dot(vector2, position);
            result.M42 = -Vector3.Dot(vector3, position);
            result.M43 = -Vector3.Dot(vector1, position);
            result.M44 = 1f;
        }

        #endregion
    }
}
