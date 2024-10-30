# Looking Glass Bridge SDK

The Looking Glass Bridge SDK unlocks the ability to integrate your existing 3D software with Looking Glass displays, making the Looking Glass a holographic second monitor.

The SDK comes in two different flavors:

- Native dynamic libraries for Windows, Mac, and Linux (contained in this repository)
- JavaScript for web or Node development using webxr, which you can [access here](https://github.com/Looking-Glass/looking-glass-webxr)

## Prerequisites 

Developers can use the following rendering backends:

- OpenGL
- DirectX 11
- DirectX 12
- Metal

Beyond that, in order to render to the Looking Glass display, your application must be able to:

- Distort the camera's projection matrix
- Render multiple views to a texture
- Pass that texture to the SDK

If you would like to have a single window interactive application you will also must be able to:

- Generate a borderless fullscreen window at a specific position

Also, your end user must have [Looking Glass Bridge](https://lookingglassfactory.com/software/looking-glass-bridge) installed on their machine, this provides the requisite libraries.

## How It Works 

The Looking Glass Bridge SDK provides an interface into Looking Glass Bridge. This allows you to simply render the scene from different views to generate a quilt and then let us to the heavy lift of rendering it to the display.

## Guides

These reference documents cover some of the underlying logic inside the SDK:

- [Camera](https://docs.lookingglassfactory.com/keyconcepts/camera)
- [Quilt](https://docs.lookingglassfactory.com/keyconcepts/quilts)

A few example projects are distributed as part of the Looking Glass Bridge SDK to demonstrate how to move the camera and render to a texture in the appropriate format.

To learn more about the Looking Glass and how it works, click [here](https://docs.lookingglassfactory.com/keyconcepts/how-it-works). 

## Examples

C++ samples are labeled "Native" all other samples are using C#

Any sample labeled "Interactive" uses a window created in the sample to render to the Looking Glass. This is intended for applications where there will not be a 2D window showing on the users 2D display.

OpenGL Samples:
- BridgeInProcSample
- BridgeInProcSampleOSX
- BridgeInProcSampleNative
- BridgeInProcSampleNativeInteractive

DirectX Samples:
- BridgeInProcSampleDX11
- BridgeInProcSampleDX11Interactive
- BridgeInProcSampleDX12
- BridgeInProcSampleDX12Interactive

Metal Samples:
- BridgeInProcSampleOSXMetal_arm64
- BridgeInProcSampleOSXMetal_x86_64
- BridgeInProcSampleMetalNativeInteractive

## Building Samples:

Native samples are built using cmake:

```bash
cd BridgeInProcSampleNative
mkdir build
cd build
cmake ..
cd ..
cmake --build ./build
```

All other sample directories contain a dotnet solution:

```bash
cd BridgeInProcSample
dotnet workload restore
dotnet restore
dotnet build
dotnet run
```

## Questions

Email us at [support@lookingglassfactory.com](mailto:support@lookingglassfactory.com) if you have any further questions about how you can integrate Looking Glass Bridge SDK into your software.
