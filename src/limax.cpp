#include "stdio.h"
#include "stdlib.h"
#include "float.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#pragma warning(push, 0)
#include "stb_image_write.h"
#pragma warning(pop)

#include "nom.h"

vec3 ScreenFromView(vec3 pos, f32 znear)
{
    vec3 result;
    result.x = (pos.x * znear) / (-pos.z);
    result.y = (pos.y * znear) / (-pos.z);
    result.z = -pos.z;
    return(result);
}

vec2 NDCFromScreen(vec3 pos)
{
    vec2 result;
    result.x = (pos.x + 1) / 2;
    result.y = (pos.y + 1) / 2;
    return(result);
}

vec2i RasterFromNDC(vec2 pos, s32 width, s32 height)
{
    vec2i result;
    result.x = (s32)(pos.x * ((f32)width - 1.0f));
    result.y = (s32)(pos.y * ((f32)height - 1.0f));
    return(result);
}

struct image
{
    u8 *framebuffer;
    f32 *zbuffer;
    s32 width;
    s32 height;
    s32 bytes_per_pixel;
};

image CreateImage(s32 width, s32 height)
{
    image result;
    result.width = width;
    result.height = height;
    result.bytes_per_pixel = 3;

    result.framebuffer = (u8*)malloc(sizeof(u8) * width * height * result.bytes_per_pixel);
    for(s32 i = 0; i < width * height * result.bytes_per_pixel; i++)
    {
        result.framebuffer[i] = 0;
    }

    result.zbuffer = (f32*)malloc(sizeof(f32) * width * height);
    for(s32 i = 0; i < width * height; i++)
    {
        result.zbuffer[i] = FLT_MAX;
    }

    return(result);
}

void WritePixel(image *img, s32 x, s32 y, u8 r, u8 g, u8 b)
{
    if(x < 0 || y < 0 || x >= img->width || y >= img->height)
    {
        return;
    }
    s32 pixel_idx = (y * img->width + x) * 3;
    img->framebuffer[pixel_idx] = r;
    img->framebuffer[pixel_idx + 1] = g;
    img->framebuffer[pixel_idx + 2] = b;
}

void ClearScreen(image *img, u8 r, u8 g, u8 b)
{
    for(s32 x = 0; x < img->width; x++)
    {
        for(s32 y = 0; y < img->height; y++)
        {
            WritePixel(img, x, y, r, g, b);
        }
    }
}

void WriteImageToFile(image *img, const char *filename)
{
    s32 ok = stbi_write_png(filename, img->width, img->height, img->bytes_per_pixel,
                            img->framebuffer, img->bytes_per_pixel * img->width);
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
    s32 image_width = 204;
    s32 image_height = 155;

    f32 aspect_ratio = (f32)image_width / (f32)image_height;
    f32 znear = 0.5f;
    f32 zfar = 100.0f;

    f32 vertical_fov_degrees = 90.0f;
    f32 vertical_canvas_size = 2.0f * tanf(DEG2RAD(vertical_fov_degrees) / 2.0f) * znear;
    f32 horizontal_canvas_size = vertical_canvas_size * aspect_ratio;

    image img = CreateImage(image_width, image_height);

    ClearScreen(&img, 80, 15, 45);

    vec3 A = { 0.0f, 0.5f, 2.0f };
    vec3 B = { -0.5f, -0.5f, 2.0f };
    vec3 C = { 0.5f, -0.5f, 2.0f };
    vec2i Aprime = RasterFromNDC(NDCFromScreen(ScreenFromView(A, znear)), image_width, image_height);
    vec2i Bprime = RasterFromNDC(NDCFromScreen(ScreenFromView(B, znear)), image_width, image_height);
    vec2i Cprime = RasterFromNDC(NDCFromScreen(ScreenFromView(C, znear)), image_width, image_height);

    WritePixel(&img, Aprime.x, Aprime.y, 255, 255, 255);
    WritePixel(&img, Bprime.x, Bprime.y, 255, 255, 255);
    WritePixel(&img, Cprime.x, Cprime.y, 255, 255, 255);

    WriteImageToFile(&img, "out.png");

    exit(0);
}