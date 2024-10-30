cbuffer cbPerObject {
    float4x4 gWorldViewProj;
}

struct VertexIn {
    float3 PosL : POSITION;
    float4 Color : COLOR;
};

struct VertexOut {
    float4 PosH : SV_POSITION;
    float4 Color : COLOR;
};

VertexOut VS(VertexIn vin){
    VertexOut vout;
    vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
    vout.Color = vin.Color;

    return vout;
}

// Standard pixel shader for normalized color output
float4 PS(VertexOut pin) : SV_Target {
    return pin.Color;
}

// New pixel shader for uint output
uint4 PS_uint(VertexOut pin) : SV_Target {
    return uint4(pin.Color * 255.0f); // Scale and convert to uint
}

// Technique for standard drawing
technique11 ColorTech {
    pass P0 {
        SetVertexShader(CompileShader(vs_4_0, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0, PS()));
    }
}

// New technique for drawing with uint output
technique11 ColorTechUInt {
    pass P0 {
        SetVertexShader(CompileShader(vs_4_0, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0, PS_uint()));
    }
}
