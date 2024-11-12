//***************************************************************************************
// Directly uses NDC coordinates and samples a texture, modulated by color.
//***************************************************************************************

cbuffer cbPerObject : register(b0)
{
    // No transformation matrix needed since we are in NDC
};

// Texture and Sampler
Texture2D gTexture : register(t0);    // Texture bound to slot t0
SamplerState gSampler : register(s0); // Sampler bound to slot s0

// Input structure for the vertex shader
struct VertexIn
{
    float3 PosL  : POSITION; // NDC position (already in [-1, 1] range)
    float4 Color : COLOR;    // Vertex color
};

// Output structure for the vertex shader, passed to the pixel shader
struct VertexOut
{
    float4 PosH    : SV_POSITION;  // Homogeneous clip space position (same as NDC)
    float2 TexCoord : TEXCOORD;    // Texture coordinates generated from NDC position
    float4 Color   : COLOR;        // Passed-through color for modulation
};

// Vertex Shader: Generates UVs and passes color
VertexOut VS(VertexIn vin)
{
    VertexOut vout;

    // Directly pass NDC position as homogeneous position
    vout.PosH = float4(vin.PosL, 1.0f);

    // Invert Y component of the texture coordinates to flip the texture vertically
    vout.TexCoord = float2(vin.PosL.x * 0.5f + 0.5f, -vin.PosL.y * 0.5f + 0.5f);

    // Pass through the vertex color
    vout.Color = vin.Color;

    return vout;
}

// Pixel Shader: Samples the texture and modulates by the vertex color
float4 PS(VertexOut pin) : SV_Target
{
    // Sample the texture using the generated texture coordinates
    float4 texColor = gTexture.Sample(gSampler, pin.TexCoord);

    // Modulate the texture color by the input vertex color
    return texColor * pin.Color;
}
