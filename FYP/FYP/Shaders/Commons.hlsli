struct RayPayload
{
    float4 color;
};

struct Viewport
{
    float Left;
    float Top;
    float Right;
    float Bottom;
};

struct RayGenerationCB
{
    Viewport View;
    Viewport Stencil;
};