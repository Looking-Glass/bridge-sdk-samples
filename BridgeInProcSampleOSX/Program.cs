using System;
using System.Collections.Generic;
using System.Diagnostics;
using OpenTK.Graphics.OpenGL;
using OpenTK.Mathematics;
using OpenTK.Windowing.Desktop;
using OpenTK.Windowing.Common;
using System.Runtime.InteropServices;

namespace RotatingCube
{
    public struct Vertex
    {
        public const int Size = (4 + 4) * 4; // size of struct in bytes

        private readonly Vector4 _position;
        private readonly Color4 _color;

        public Vertex(Vector4 position, Color4 color)
        {
            _position = position;
            _color = color;
        }
    }

    public class MainWindow : GameWindow
    {
        private readonly string _title;
        private int _width;
        private int _height;

        private int _program;
        private double _time;
        private bool _initialized;
        private int _vertexArray;
        private int _buffer;
        private int _verticeCount;

        private Matrix4 _model;
        private Matrix4 _view;
        private Matrix4 _projection;
        private float _FOV = 45.0f;

        private float _lastTimestamp = Stopwatch.GetTimestamp();
        private float _freq = Stopwatch.Frequency;

        private float _angle;

        BridgeInProc.Window _bridge_window;
        uint _bridge_window_width = 0;
        uint _bridge_window_height = 0;
        uint _bridge_max_texture_size = 0;
        int _bridge_render_texture = 0;
        int _bridge_render_fbo = 0;
        int _bridge_render_depth_buffer = 0;
        uint _bridge_render_texture_width = 0;
        uint _bridge_render_texture_height = 0;
        uint _bridge_quilt_vx = 5;
        uint _bridge_quilt_vy = 9;
        uint _bridge_quilt_view_width = 0;
        uint _bridge_quilt_view_height = 0;
        float _scaling = 1.0f;

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

        public MainWindow()
            : base(GameWindowSettings.Default, new NativeWindowSettings()
            {
                Size = (750, 500),
                Title = "Bridge InProc C# Sample",
                RedBits = 8,
                GreenBits = 8,
                BlueBits = 8,
                AlphaBits = 8,
                DepthBits = 32,
                StencilBits = 0,
                APIVersion = new Version(4, 1),
                Profile = ContextProfile.Core,
                Flags = ContextFlags.ForwardCompatible
            })
        {
            _width = 750;
            _height = 500;
            _title = "Bridge InProc C# Sample";
        }

        protected override void OnLoad()
        {
            _model = Matrix4.Identity;
            Vertex[] vertices =
            {
                new Vertex(new Vector4(-0.5f, -0.5f, -0.5f,  1.0f), Color4.Blue),
                new Vertex(new Vector4( 0.5f, -0.5f, -0.5f,  1.0f), Color4.Blue),
                new Vertex(new Vector4( 0.5f,  0.5f, -0.5f,  1.0f), Color4.Blue),
                new Vertex(new Vector4( 0.5f,  0.5f, -0.5f,  1.0f), Color4.Blue),
                new Vertex(new Vector4(-0.5f,  0.5f, -0.5f,  1.0f), Color4.Blue),
                new Vertex(new Vector4(-0.5f, -0.5f, -0.5f,  1.0f), Color4.Blue),

                new Vertex(new Vector4(-0.5f, -0.5f,  0.5f,  1.0f), Color4.Blue),
                new Vertex(new Vector4( 0.5f, -0.5f,  0.5f,  1.0f), Color4.Blue),
                new Vertex(new Vector4( 0.5f,  0.5f,  0.5f,  1.0f), Color4.Blue),
                new Vertex(new Vector4( 0.5f,  0.5f,  0.5f,  1.0f), Color4.Blue),
                new Vertex(new Vector4(-0.5f,  0.5f,  0.5f,  1.0f), Color4.Blue),
                new Vertex(new Vector4(-0.5f, -0.5f,  0.5f,  1.0f), Color4.Blue),

                new Vertex(new Vector4(-0.5f,  0.5f,  0.5f,  1.0f), Color4.Red),
                new Vertex(new Vector4(-0.5f,  0.5f, -0.5f,  1.0f), Color4.Red),
                new Vertex(new Vector4(-0.5f, -0.5f, -0.5f,  1.0f), Color4.Red),
                new Vertex(new Vector4(-0.5f, -0.5f, -0.5f,  1.0f), Color4.Red),
                new Vertex(new Vector4(-0.5f, -0.5f,  0.5f,  1.0f), Color4.Red),
                new Vertex(new Vector4(-0.5f,  0.5f,  0.5f,  1.0f), Color4.Red),

                new Vertex(new Vector4( 0.5f,  0.5f,  0.5f,  1.0f), Color4.Red),
                new Vertex(new Vector4( 0.5f,  0.5f, -0.5f,  1.0f), Color4.Red),
                new Vertex(new Vector4( 0.5f, -0.5f, -0.5f,  1.0f), Color4.Red),
                new Vertex(new Vector4( 0.5f, -0.5f, -0.5f,  1.0f), Color4.Red),
                new Vertex(new Vector4( 0.5f, -0.5f,  0.5f,  1.0f), Color4.Red),
                new Vertex(new Vector4( 0.5f,  0.5f,  0.5f,  1.0f), Color4.Red),

                new Vertex(new Vector4(-0.5f, -0.5f, -0.5f,  1.0f), Color4.Green),
                new Vertex(new Vector4( 0.5f, -0.5f, -0.5f,  1.0f), Color4.Green),
                new Vertex(new Vector4( 0.5f, -0.5f,  0.5f,  1.0f), Color4.Green),
                new Vertex(new Vector4( 0.5f, -0.5f,  0.5f,  1.0f), Color4.Green),
                new Vertex(new Vector4(-0.5f, -0.5f,  0.5f,  1.0f), Color4.Green),
                new Vertex(new Vector4(-0.5f, -0.5f, -0.5f,  1.0f), Color4.Green),

                new Vertex(new Vector4(-0.5f,  0.5f, -0.5f,  1.0f), Color4.Green),
                new Vertex(new Vector4( 0.5f,  0.5f, -0.5f,  1.0f), Color4.Green),
                new Vertex(new Vector4( 0.5f,  0.5f,  0.5f,  1.0f), Color4.Green),
                new Vertex(new Vector4( 0.5f,  0.5f,  0.5f,  1.0f), Color4.Green),
                new Vertex(new Vector4(-0.5f,  0.5f,  0.5f,  1.0f), Color4.Green),
                new Vertex(new Vector4(-0.5f,  0.5f, -0.5f,  1.0f), Color4.Green),
            };

            _verticeCount = vertices.Length;
            _vertexArray = GL.GenVertexArray();
            _buffer = GL.GenBuffer();

            GL.BindVertexArray(_vertexArray);
            GL.BindBuffer(BufferTarget.ArrayBuffer, _buffer);

            // create first buffer: vertex
            GL.BufferData(BufferTarget.ArrayBuffer, Vertex.Size * vertices.Length, vertices, BufferUsageHint.StaticDraw);

            // Set up the vertex attributes
            GL.EnableVertexAttribArray(0); // Enable attribute 0
            GL.VertexAttribPointer(
                0,                      // attribute index, from the shader location = 0
                4,                      // size of attribute, vec4
                VertexAttribPointerType.Float, // contains floats
                false,                  // does not need to be normalized as it is already, floats ignore this flag anyway
                Vertex.Size,            // stride (size of a Vertex)
                0);                     // relative offset, first item

            GL.EnableVertexAttribArray(1); // Enable attribute 1
            GL.VertexAttribPointer(
                1,                      // attribute index, from the shader location = 1
                4,                      // size of attribute, vec4
                VertexAttribPointerType.Float, // contains floats
                false,                  // does not need to be normalized as it is already, floats ignore this flag anyway
                Vertex.Size,            // stride (size of a Vertex)
                16);                    // relative offset after a vec4

            GL.Enable(EnableCap.Blend);
            GL.BlendFunc(BlendingFactor.SrcAlpha, BlendingFactor.OneMinusSrcAlpha);
            GL.ColorMask(true, true, true, true);
            GL.ClearDepth(1.0f);
            GL.DepthRange(0.0f, 1.0f);
            GL.DepthFunc(DepthFunction.Lequal);
            GL.Enable(EnableCap.DepthTest);
            GL.PolygonMode(MaterialFace.Front, PolygonMode.Fill);
            GL.PatchParameter(PatchParameterInt.PatchVertices, 3);

            _initialized = true;

            try
            {
                _program = GL.CreateProgram();
                var shaders = new List<int>();
                ShaderType type = ShaderType.VertexShader;
                var shader = GL.CreateShader(type);
                string src = @"#version 330 core
                                layout (location = 0) in vec4 position;
                                layout(location = 1) in vec4 color;
                                out vec4 vs_color;

                                out vec3 original_normal;
                                out vec3 transformed_normal;

                                uniform mat4 model;
                                uniform mat4 view;
                                uniform mat4 projection;

                                void main(void)
                                {
                                    gl_Position = projection * view * model * position;
                                    vs_color = color;
                                    original_normal = vec3(color);
                                    mat3 normal_matrix = transpose(inverse(mat3(view * model)));
                                    transformed_normal = normal_matrix * original_normal;
                                }";

                GL.ShaderSource(shader, src);
                GL.CompileShader(shader);
                var info = GL.GetShaderInfoLog(shader);
                if (!string.IsNullOrWhiteSpace(info))
                    throw new Exception($"CompileShader {type} had errors: {info}");

                shaders.Add(shader);

                type = ShaderType.FragmentShader;
                shader = GL.CreateShader(type);
                src = @"#version 330 core
                        in vec4 vs_color;
                        in vec3 original_normal;
                        in vec3 transformed_normal;
                        out vec4 color;

                        void main(void)
                        {
                            vec3 local_original_normal = original_normal;
                            float lighting = abs(dot(transformed_normal, vec3(0,0,-1)));
                            color = vec4((vs_color * lighting).rgb, 1.0);
                        }";

                GL.ShaderSource(shader, src);
                GL.CompileShader(shader);
                info = GL.GetShaderInfoLog(shader);
                if (!string.IsNullOrWhiteSpace(info))
                    throw new Exception($"CompileShader {type} had errors: {info}");

                shaders.Add(shader);

                foreach (var shader_ in shaders)
                    GL.AttachShader(_program, shader_);

                GL.LinkProgram(_program);
                var info_ = GL.GetProgramInfoLog(_program);
                if (!string.IsNullOrWhiteSpace(info_))
                    throw new Exception($"CompileShaders ProgramLinking had errors: {info}");

                foreach (var shader_ in shaders)
                {
                    GL.DetachShader(_program, shader_);
                    GL.DeleteShader(shader_);
                }
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.ToString());
                throw;
            }

            if (!BridgeInProc.Controller.Initialize(@"BridgeInProcSampleOSX"))
            {
                Environment.Exit(-1);
            }

            // mlc: instance the window
            bool window_status = BridgeInProc.Controller.InstanceWindowGL(ref _bridge_window);

            if (!window_status)
            {
                throw new Exception("Tried to create an InProc Bridge Window without a client GL context active!");
            }

            // mlc: cache the size of the bridge output so we can decide how large to make
            // the quilt views
            BridgeInProc.Controller.GetWindowDimensions(_bridge_window, ref _bridge_window_width, ref _bridge_window_height);

            // mlc: see how large we can make out render texture
            BridgeInProc.Controller.GetMaxTextureSize(_bridge_window, ref _bridge_max_texture_size);

            // mlc: now we need to figure out how large our views and quilt will be
            uint desired_view_width  = _bridge_window_width;
            uint desired_view_height = _bridge_window_height;

            uint desired_render_texture_width  = desired_view_width  * _bridge_quilt_vx;
            uint desired_render_texture_height = desired_view_height * _bridge_quilt_vy;

            if (desired_render_texture_width  <= _bridge_max_texture_size &&
                desired_render_texture_height <= _bridge_max_texture_size)
            {
                // mlc: under the max size -- good to go!
                _bridge_quilt_view_width      = desired_view_width;
                _bridge_quilt_view_height     = desired_view_height;
                _bridge_render_texture_width  = desired_render_texture_width;
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

                _bridge_quilt_view_width      = (uint)((float)desired_view_width * scalar);
                _bridge_quilt_view_height     = (uint)((float)desired_view_height * scalar);
                _bridge_render_texture_width  = (uint)((float)desired_render_texture_width * scalar);
                _bridge_render_texture_height = (uint)((float)desired_render_texture_height * scalar);
            }

            // mlc: generate the texture and fbo so we can interop!
            GL.GenTextures(1, out _bridge_render_texture);
            GL.BindTexture(TextureTarget.Texture2D, _bridge_render_texture);

            GL.TexImage2D(TextureTarget.Texture2D, 0,
                PixelInternalFormat.Rgba,
                (int)_bridge_render_texture_width,
                (int)_bridge_render_texture_height,
                0, PixelFormat.Rgba, PixelType.UnsignedByte,
                IntPtr.Zero);

            // Depth renderbuffer
            GL.GenRenderbuffers(1, out _bridge_render_depth_buffer);
            GL.BindRenderbuffer(RenderbufferTarget.Renderbuffer, _bridge_render_depth_buffer);

            GL.RenderbufferStorage(RenderbufferTarget.Renderbuffer,
                RenderbufferStorage.DepthComponent24,
                (int)_bridge_render_texture_width,
                (int)_bridge_render_texture_height);

            // Framebuffer
            GL.GenFramebuffers(1, out _bridge_render_fbo);
            GL.BindFramebuffer(FramebufferTarget.Framebuffer, _bridge_render_fbo);

            GL.FramebufferTexture2D(FramebufferTarget.Framebuffer,
                FramebufferAttachment.ColorAttachment0,
                TextureTarget.Texture2D,
                _bridge_render_texture,
                0);

            GL.FramebufferRenderbuffer(FramebufferTarget.Framebuffer,
                FramebufferAttachment.DepthAttachment,
                RenderbufferTarget.Renderbuffer,
                _bridge_render_depth_buffer);

            GL.BindFramebuffer(FramebufferTarget.Framebuffer, 0);
            GL.BindRenderbuffer(RenderbufferTarget.Renderbuffer, 0);

            // mlc: set our size to match the display
            Size = new Vector2i((int)_bridge_window_width/2, (int)_bridge_window_height/2);
            _width  = (int)_bridge_window_width/2;
            _height = (int)_bridge_window_height/2;

            // mlc: deal with retina
            _scaling = GetScaleFactor();
        }

        protected override void OnUnload()
        {
            Debug.WriteLine("OnUnload called");

            BridgeInProc.Controller.Uninitialize();

            GL.BindFramebuffer(FramebufferTarget.Framebuffer, 0);
            GL.DeleteFramebuffers(1, ref _bridge_render_fbo);

            GL.BindTexture(TextureTarget.Texture2D, 0);
            GL.DeleteTextures(1, ref _bridge_render_texture);

            GL.DeleteVertexArray(_vertexArray);
            GL.DeleteBuffer(_buffer);
            GL.DeleteProgram(_program);

            base.OnUnload();
        }

        private float[] Matrix4ToArray(Matrix4 matrix)
        {
            float[] data = new float[16];
            for (int i = 0; i < 4; i++)
            {
                for (int j = 0; j < 4; j++)
                {
                    data[i * 4 + j] = matrix[i, j];
                }
            }
            return data;
        }

        protected void DrawScene(float tx = 0.0f, bool invert = false)
        {
            // Bind the VBO
            GL.BindBuffer(BufferTarget.ArrayBuffer, _buffer);

            // Bind the VAO 
            GL.BindVertexArray(_vertexArray);

            // Use/Bind the program
            GL.UseProgram(_program);

            _model = Matrix4.CreateFromAxisAngle(new Vector3(1.0f, 0.0f, 1.0f), _angle);

            Vector3 eye = new Vector3(0, 0, 5);

            Matrix4 view = Matrix4.CreateTranslation(-tx, 0, 0) *
                           Matrix4.LookAt(eye, Vector3.Zero, Vector3.UnitY);

            // mlc: quilts start at the bottom left of the texture with the left most frame oriented top up
            // since we are using gl to render: he will flip the axis -- rotate to counteract this.
            if (invert)
            {

                float x = view.M41;
                float y = view.M42;
                float z = view.M43;
                Vector3 eyePos = new Vector3(x, y, z);

                Matrix4 translateToOrigin = Matrix4.CreateTranslation(-eyePos);
                view = translateToOrigin * view;

                Matrix4 invertView = Matrix4.CreateScale(1, -1, 1);
                view = invertView * view;

                Matrix4 translateBack = Matrix4.CreateTranslation(eyePos);
                view = translateBack * view;

            }

            _view = view;

            _projection = Matrix4.CreatePerspectiveFieldOfView((float)Math.PI * (_FOV / 180f), _width / (float)_height, 0.2f, 256.0f);

            int location = GL.GetUniformLocation(_program, "model");
            GL.UniformMatrix4(location, 1, false, Matrix4ToArray(_model));

            location = GL.GetUniformLocation(_program, "view");
            GL.UniformMatrix4(location, 1, false, Matrix4ToArray(_view));

            location = GL.GetUniformLocation(_program, "projection");
            GL.UniformMatrix4(location, 1, false, Matrix4ToArray(_projection));

            // Draw 
            GL.DrawArrays(PrimitiveType.Triangles, 0, _verticeCount);
        }

        protected override void OnUpdateFrame(FrameEventArgs args)
        {
            var timeStamp = Stopwatch.GetTimestamp();
            _angle += (float)((timeStamp - _lastTimestamp) / (double)_freq);
            _lastTimestamp = timeStamp;

            //mlc: draw individual quilt views to the interop texture
            GL.BindFramebuffer(FramebufferTarget.Framebuffer, _bridge_render_fbo);

            GL.ClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            GL.Clear(ClearBufferMask.ColorBufferBit | ClearBufferMask.DepthBufferBit);

            float tx_offset = 0.005f;
            float tx = -(float)(_bridge_quilt_vx * _bridge_quilt_vy - 1) / 2.0f * tx_offset;

            for (uint y = 0; y < _bridge_quilt_vy; y++)
            {
                for (uint x = 0; x < _bridge_quilt_vx; x++)
                {
                    GL.Viewport((int)(x * _bridge_quilt_view_width),
                        (int)(y * _bridge_quilt_view_height),
                        (int)(_bridge_quilt_view_width),
                        (int)(_bridge_quilt_view_height));

                    DrawScene(tx, true);

                    tx += tx_offset;
                }
            }

            // mlc: debug -- see what the quilt looks like
            //BridgeInProc.Controller.SaveTextureToFileGL(_bridge_window, @"/Users/matty/quilt.png", (ulong)_bridge_render_texture, BridgeInProc.PixelFormats.RGBA, _bridge_render_texture_width, _bridge_render_texture_height);

            // mlc: draw desktop head prevaiew
            GL.BindFramebuffer(FramebufferTarget.Framebuffer, 0);

            GL.Viewport(0, 0, (int)((float)_width*_scaling), (int)((float)_height));

            GL.ClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            GL.Clear(ClearBufferMask.ColorBufferBit | ClearBufferMask.DepthBufferBit);

            DrawScene();

            BridgeInProc.Controller.DrawInteropQuiltTextureGL(_bridge_window, (ulong)_bridge_render_texture, BridgeInProc.PixelFormats.RGBA, _bridge_render_texture_width, _bridge_render_texture_height, _bridge_quilt_vx, _bridge_quilt_vy, 1.0f, 1.0f);
            Context.SwapBuffers();
        }

        protected override void OnResize(ResizeEventArgs e)
        {
            base.OnResize(e);
        }
    }
}

namespace BridgeInProcSample
{
    class Program
    {
        [STAThread]
        static void Main(string[] args)
        {
            new RotatingCube.MainWindow().Run();
        }
    }
}

