Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

struct VertexIn {
    float3 PosL : POSITION;
    float4 Color : COLOR;
};

struct VertexOut {
    float4 PosH : SV_POSITION;
    float2 TexCoord : TEXCOORD;
    float4 Color : COLOR;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    
    // Convert position to homogenous clip space
    vout.PosH = float4(vin.PosL, 1.0f); // Directly in NDC coordinates
    
    // Convert from NDC (-1,1) to UV (0,1) and flip the Y-coordinate
    vout.TexCoord = float2(vin.PosL.x * 0.5f + 0.5f, 1.0f - (vin.PosL.y * 0.5f + 0.5f));
    
    // Pass through color
    vout.Color = vin.Color; 

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    // Sample the texture using the calculated UV coordinates
    float4 sampledColor = gTexture.Sample(gSampler, pin.TexCoord);
    return sampledColor * pin.Color;
}

technique11 RenderTexture {
    pass P0 {
        SetVertexShader(CompileShader(vs_4_0, VS()));
        SetPixelShader(CompileShader(ps_4_0, PS()));
    }
}
