cbuffer ShadowDepthCB : register(b0) {
    matrix worldLightViewProj;
};

float4 VSMain(float4 pos : POSITION0) : SV_POSITION {
    return mul(pos, worldLightViewProj);
}
