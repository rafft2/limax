#include "stdio.h"
#include "stdlib.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#pragma warning(push, 0)
#include "stb_image_write.h"
#pragma warning(pop)

#include "nom.h"

vec2 ScreenFromWorld(vec3 pos)
{
    vec2 result;
    result.x = pos.x / pos.z;
    result.y = pos.y / pos.z;
    return(result);
}

vec2 NDCFromScreen(vec2 pos)
{
    vec2 result;
    result.x = (pos.x + 1) / 2;
    result.y = (-pos.y + 1) / 2;
    return(result);
}

vec2i RasterFromNDC(vec2 pos, s32 width, s32 height)
{
    vec2i result;
    result.x = (s32)(pos.x * ((f32)width - 1.0f));
    result.y = (s32)(pos.y * ((f32)height - 1.0f));
    return(result);
}

void WritePixel(u8* framebuffer, vec2i framebuffer_dim, s32 x, s32 y)
{
    if(x < 0 || y < 0 || x >= framebuffer_dim.x || y >= framebuffer_dim.y)
    {
        return;
    }
    framebuffer[y * framebuffer_dim.x + x] = 255;
}

void WriteFramebufferToFile(u8 *framebuffer, vec2i framebuffer_dim, const char *filename)
{
    s32 ok = stbi_write_png(filename, framebuffer_dim.x, framebuffer_dim.y, 1, framebuffer, 1 * framebuffer_dim.x);
    if(!ok)
    {
        printf("error with stbi_write_png.\n");
    }
    else
    {
        printf("wrote image: %s.\n", filename);
    }
}

int main(void)
{
    s32 image_width = 64;
    s32 image_height = 64;

    u8* framebuffer = (u8*)malloc(image_width * image_height * sizeof(u8));
    for(s32 y = 0; y < image_width; y++)
    {
        for(s32 x = 0; x < image_height; x++)
        {
            framebuffer[x * image_width + y] = 127;
        }
    }

    vec3 A = { 0.0f, 0.5f, 2.0f };
    vec3 B = { -0.5f, -0.5f, 2.0f };
    vec3 C = { 0.5f, -0.5f, 2.0f };
    vec2i Aprime = RasterFromNDC(NDCFromScreen(ScreenFromWorld(A)), image_width, image_height);
    vec2i Bprime = RasterFromNDC(NDCFromScreen(ScreenFromWorld(B)), image_width, image_height);
    vec2i Cprime = RasterFromNDC(NDCFromScreen(ScreenFromWorld(C)), image_width, image_height);

    WritePixel(framebuffer, {image_width, image_height}, Aprime.x, Aprime.y);
    WritePixel(framebuffer, {image_width, image_height}, Bprime.x, Bprime.y);
    WritePixel(framebuffer, {image_width, image_height}, Cprime.x, Cprime.y);

    WriteFramebufferToFile(framebuffer, {image_width, image_height}, "out.png");

    exit(0);
}