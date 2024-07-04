#pragma pack_matrix(column_major)
#include "rootsig.hlsl"

RWTexture2D<float4> framebuffer : register(u0);

[numthreads(8, 8, 1)]
[RootSignature(ROOTSIG)]
void main(uint3 dt_id : SV_DispatchThreadID)
{
    float4 circle_color = { 1.0f, 0.0f, 1.0f, 0.0f }; // alpha channel does not change anything, due to swapchain settings
    
    uint2 screen_coord = uint2(dt_id.x, dt_id.y);
    uint width, height;
    framebuffer.GetDimensions(width, height);
    
    float2 center = { width / 2.0f, height / 2.0f };

    float radius = 200.0f;
    
    float distance = length(float2(screen_coord) - center);
    
    if (distance <= radius)
    {
        framebuffer[screen_coord.xy] = circle_color;
    }
}