using System;
using OpenTK.Windowing.Desktop;
using OpenTK.Windowing.Common;
using OpenTK.Graphics.OpenGL;
using OpenTK.Mathematics;
using BridgeSDK;
using System.Collections.Generic;
using BridgeSDKSample;
using System.Linq;

namespace RefactoredProgram
{
    public class MainWindow : GameWindow
    {
        // Shader sources
        private const string vertexShaderSource = @"
            #version 330 core
            layout (location = 0) in vec3 position;
            layout (location = 1) in vec3 color;
            out vec3 vertexColor;
            uniform mat4 model;
            uniform mat4 view;
            uniform mat4 projection;
            void main() {
                gl_Position = projection * view * model * vec4(position, 1.0);
                vertexColor = color;
            }
        ";

        private const string fragmentShaderSource = @"
            #version 330 core
            in vec3 vertexColor;
            out vec4 FragColor;
            void main() {
                FragColor = vec4(vertexColor, 1.0);
            }
        ";

        private readonly float[] vertices = new float[]
        {
            // Positions           // Colors
            -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  // Front face
             0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
             0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
            -0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,

            -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  // Back face
             0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
             0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,

            -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f,  // Left face
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,

             0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,  // Right face
             0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
             0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
             0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f,

            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f,  // Bottom face
             0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
             0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 1.0f,

            -0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 1.0f,  // Top face
             0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
             0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 1.0f
        };

        private readonly uint[] indices = new uint[]
        {
            0, 1, 2, 2, 3, 0,        // Front face
            4, 5, 6, 6, 7, 4,        // Back face
            8, 9, 10, 10, 11, 8,     // Left face
            12, 13, 14, 14, 15, 12,  // Right face
            16, 17, 18, 18, 19, 16,  // Bottom face
            20, 21, 22, 22, 23, 20   // Top face
        };

        // Bridge Controller
        private Window wnd = 0;
        private BridgeWindowData bridgeData;
        private LKGCamera camera;
        private bool isBridgeDataInitialized = false;

        // Other variables for shader, VAO, VBO, etc.
        private int shaderProgram;
        private int vao, vbo, ebo;

        // Variables for render texture and framebuffers
        private int render_texture = 0;
        private int render_fbo = 0;
        private int depth_buffer = 0;


        // Variables for mouse control
        private bool mousePressed = false;
        private Vector2 lastMousePos;
        private float angleX = 0.0f, angleY = 0.0f;

        private float focus = -0.5f;
        private float offset_mult = 1.0f;

        // Constructor
        public MainWindow()
            : base(GameWindowSettings.Default, new NativeWindowSettings()
            {
                Size = new OpenTK.Mathematics.Vector2i(800, 800),
                Title = "Refactored C# Program",
            })
        {
        }

        protected override void OnLoad()
        {
            if (!Controller.Initialize("BridgeSDKSampleNative"))
            {
                Console.WriteLine("Failed to initialize bridge. Bridge may be missing, or the version may be too old");
            }

            List<DisplayInfo> displays = Controller.GetDisplayInfoList();

            if (displays.Count > 0 && Controller.InstanceWindowGL(ref wnd, displays[0].DisplayId))
            {
                // Successfully created the window handle
            }
            else
            {
                wnd = 0;
                Console.WriteLine("Failed to initialize bridge window. Do you have any displays connected?");
            }

            bridgeData = Controller.GetWindowData(wnd);
            isBridgeDataInitialized = (bridgeData.Wnd != 0);

            if (isBridgeDataInitialized)
            {
                // Set focus based on aspect ratio
                focus = bridgeData.DisplayAspect > 1.0f ? -0.5f : -2.0f;
                offset_mult = 1.0f;

                // Update window title
                DisplayInfo displayInfo = Controller.GetDisplayInfoList().FirstOrDefault(info => info.DisplayId == bridgeData.DisplayIndex);
                Title = $"Bridge SDK C# Sample -- {displayInfo.Name} : {displayInfo.Serial}";

                // Set window size
                int window_width = (int)bridgeData.OutputWidth / 2;
                int window_height = (int)bridgeData.OutputHeight / 2;
                Size = new Vector2i(window_width, window_height);

                // Initialize OpenGL textures and framebuffers using bridgeData's quilt dimensions
                GL.GenTextures(1, out render_texture);
                GL.BindTexture(TextureTarget.Texture2D, render_texture);
                GL.TexImage2D(TextureTarget.Texture2D,
                    0,
                    PixelInternalFormat.Rgba,
                    (int)bridgeData.QuiltWidth,
                    (int)bridgeData.QuiltHeight,
                    0,
                    PixelFormat.Rgba,
                    PixelType.UnsignedByte,
                    IntPtr.Zero);

                // Generate and bind the renderbuffer for depth
                GL.GenRenderbuffers(1, out depth_buffer);
                GL.BindRenderbuffer(RenderbufferTarget.Renderbuffer, depth_buffer);

                // Create a depth buffer
                GL.RenderbufferStorage(RenderbufferTarget.Renderbuffer, RenderbufferStorage.DepthComponent32, (int)bridgeData.QuiltWidth, (int)bridgeData.QuiltHeight);

                // Generate the framebuffer
                GL.GenFramebuffers(1, out render_fbo);
                GL.BindFramebuffer(FramebufferTarget.Framebuffer, render_fbo);

                // Attach the texture to the framebuffer as a color attachment
                GL.FramebufferTexture2D(FramebufferTarget.Framebuffer, FramebufferAttachment.ColorAttachment0, TextureTarget.Texture2D, render_texture, 0);

                // Attach the renderbuffer as a depth attachment
                GL.FramebufferRenderbuffer(FramebufferTarget.Framebuffer, FramebufferAttachment.DepthAttachment, RenderbufferTarget.Renderbuffer, depth_buffer);

                GL.BindFramebuffer(FramebufferTarget.Framebuffer, 0);
                GL.BindRenderbuffer(RenderbufferTarget.Renderbuffer, 0);
            }

            float size = 10.0f;
            Vector3 target = new Vector3(0.0f, 0.0f, 0.0f);
            Vector3 up = new Vector3(0.0f, 1.0f, 0.0f);

            float fov = 14.0f;
            float viewcone = isBridgeDataInitialized ? bridgeData.Viewcone : 40.0f;
            float aspect = isBridgeDataInitialized ? bridgeData.DisplayAspect : 1.0f;
            float nearPlane = 0.1f;
            float farPlane = 100.0f;

            camera = new LKGCamera(size, target, up, fov, viewcone, aspect, nearPlane, farPlane);

            // Initialize shaders
            shaderProgram = GL.CreateProgram();

            int vertexShader = GL.CreateShader(ShaderType.VertexShader);
            GL.ShaderSource(vertexShader, vertexShaderSource);
            GL.CompileShader(vertexShader);
            GL.AttachShader(shaderProgram, vertexShader);

            int fragmentShader = GL.CreateShader(ShaderType.FragmentShader);
            GL.ShaderSource(fragmentShader, fragmentShaderSource);
            GL.CompileShader(fragmentShader);
            GL.AttachShader(shaderProgram, fragmentShader);

            GL.LinkProgram(shaderProgram);

            GL.DeleteShader(vertexShader);
            GL.DeleteShader(fragmentShader);

            // Initialize VAO, VBO, EBO
            vao = GL.GenVertexArray();
            vbo = GL.GenBuffer();
            ebo = GL.GenBuffer();

            GL.BindVertexArray(vao);

            GL.BindBuffer(BufferTarget.ArrayBuffer, vbo);
            GL.BufferData(BufferTarget.ArrayBuffer, vertices.Length * sizeof(float), vertices, BufferUsageHint.StaticDraw);

            GL.BindBuffer(BufferTarget.ElementArrayBuffer, ebo);
            GL.BufferData(BufferTarget.ElementArrayBuffer, indices.Length * sizeof(uint), indices, BufferUsageHint.StaticDraw);

            // Set vertex attribute pointers
            GL.VertexAttribPointer(0, 3, VertexAttribPointerType.Float, false, 6 * sizeof(float), 0);
            GL.EnableVertexAttribArray(0);

            GL.VertexAttribPointer(1, 3, VertexAttribPointerType.Float, false, 6 * sizeof(float), 3 * sizeof(float));
            GL.EnableVertexAttribArray(1);

            GL.BindVertexArray(0);

            GL.Enable(EnableCap.Blend);
            GL.BlendFunc(BlendingFactor.SrcAlpha, BlendingFactor.OneMinusSrcAlpha);
            GL.ColorMask(true, true, true, true);
            GL.ClearDepth(1.0f);
            GL.DepthRange(0.0f, 1.0f);
            GL.DepthFunc(DepthFunction.Lequal);
            GL.Enable(EnableCap.DepthTest);
            GL.PolygonMode(MaterialFace.Front, PolygonMode.Fill);
            GL.ClearColor(0.0f, 0.0f, 0.0f, 1.0f);

            base.OnLoad();
        }

        protected override void OnUpdateFrame(FrameEventArgs args)
        {
            // Handle mouse input
            if (MouseState.IsButtonDown(OpenTK.Windowing.GraphicsLibraryFramework.MouseButton.Left))
            {
                if (!mousePressed)
                {
                    mousePressed = true;
                    lastMousePos = MouseState.Position;
                }
                else
                {
                    Vector2 delta = MouseState.Position - lastMousePos;
                    lastMousePos = MouseState.Position;

                    angleX += delta.Y * 0.005f; // Sensitivity
                    angleY += delta.X * 0.005f; // Sensitivity
                }
            }
            else
            {
                mousePressed = false;
            }

            // Handle scroll input for focus and offset_mult
            Vector2 scrollDelta = MouseState.ScrollDelta;
            if (scrollDelta != Vector2.Zero)
            {
                focus += scrollDelta.Y * 0.075f; // Sensitivity
                offset_mult += scrollDelta.X * 0.075f; // Sensitivity
            }

            base.OnUpdateFrame(args);
        }


        private void DrawScene(int shaderProgram, int vao, LKGCamera camera, float normalizedView = 0.5f, bool invert = false, float offset_mult = 0.0f, float focus = 0.0f)
        {
            GL.BindVertexArray(vao);
            GL.UseProgram(shaderProgram);

            // Compute view and projection matrices using LKGCamera
            Matrix4 viewMatrix;
            Matrix4 projectionMatrix;
            camera.ComputeViewProjectionMatrices(normalizedView, invert, offset_mult, focus, out viewMatrix, out projectionMatrix);

            // Compute the model matrix (e.g., rotating cube)
            Matrix4 modelMatrix = camera.ComputeModelMatrix(angleX, angleY);

            // Set uniforms
            int modelLoc = GL.GetUniformLocation(shaderProgram, "model");
            int viewLoc = GL.GetUniformLocation(shaderProgram, "view");
            int projLoc = GL.GetUniformLocation(shaderProgram, "projection");

            GL.UniformMatrix4(modelLoc, false, ref modelMatrix);
            GL.UniformMatrix4(viewLoc, false, ref viewMatrix);
            GL.UniformMatrix4(projLoc, false, ref projectionMatrix);

            // Draw the object
            GL.DrawElements(PrimitiveType.Triangles, indices.Length, DrawElementsType.UnsignedInt, 0);
        }
        
        protected override void OnRenderFrame(FrameEventArgs args)
        {
            // Draw to primary window
            GL.BindFramebuffer(FramebufferTarget.Framebuffer, 0);
            GL.Viewport(0, 0, Size.X, Size.Y);
            GL.Clear(ClearBufferMask.ColorBufferBit | ClearBufferMask.DepthBufferBit);

            DrawScene(shaderProgram, vao, camera);

            Context.SwapBuffers();

            if (isBridgeDataInitialized)
            {
                // Draw the quilt views for the hologram
                GL.BindFramebuffer(FramebufferTarget.Framebuffer, render_fbo);
                GL.Clear(ClearBufferMask.ColorBufferBit | ClearBufferMask.DepthBufferBit);

                int totalViews = bridgeData.Vx * bridgeData.Vy;

                for (int y = 0; y < bridgeData.Vy; y++)
                {
                    for (int x = 0; x < bridgeData.Vx; x++)
                    {
                        int invertedY = (int)bridgeData.Vy - 1 - y;
                        GL.Viewport(
                            x * (int)bridgeData.ViewWidth,
                            invertedY * (int)bridgeData.ViewHeight,
                            (int)bridgeData.ViewWidth,
                            (int)bridgeData.ViewHeight
                        );

                        int viewIndex = y * (int)bridgeData.Vx + x;
                        float normalizedView = (float)viewIndex / (float)(totalViews - 1);

                        DrawScene(shaderProgram, vao, camera, normalizedView, true, offset_mult, focus);
                    }
                }

                // Update the holographic display
                Controller.DrawInteropQuiltTextureGL(
                    bridgeData.Wnd,
                    (ulong)render_texture,
                    PixelFormats.RGBA,
                    (uint)bridgeData.QuiltWidth,
                    (uint)bridgeData.QuiltHeight,
                    (uint)bridgeData.Vx,
                    (uint)bridgeData.Vy,
                    bridgeData.DisplayAspect,
                    1.0f
                );
            }

            base.OnRenderFrame(args);
        }

        protected override void OnUnload()
        {
            // Cleanup controller
            Controller.Uninitialize();

            // Delete OpenGL resources
            GL.DeleteVertexArray(vao);
            GL.DeleteBuffer(vbo);
            GL.DeleteBuffer(ebo);
            GL.DeleteTexture(render_texture);
            GL.DeleteRenderbuffer(depth_buffer);
            GL.DeleteFramebuffer(render_fbo);
            GL.DeleteProgram(shaderProgram);

            base.OnUnload();
        }
    }

    class Program
    {
        static void Main(string[] args)
        {
            using (var window = new MainWindow())
            {
                window.Run();
            }
        }
    }
}
