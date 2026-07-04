#include "stdio.h"
#include "stdlib.h"
#include "float.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#pragma warning(push, 0)
#include "stb_image_write.h"
#pragma warning(pop)

#include "nom.h"

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

vec3 RasterFromView(vec3 p, f32 left, f32 right, f32 bottom, f32 top, f32 znear, s32 image_width, s32 image_height)
{
    vec3 raster;

    // screen from view
    raster.x = (p.x * znear) / -p.z;
    raster.y = (p.y * znear) / -p.z;
    raster.z = -p.z;

    // NDC from screen (here NDC is [-1, +1])
    raster.x = ((2.0f * raster.x) / (right - left)) - ((right + left) / (right - left));
    raster.y = ((2.0f * raster.y) / (top - bottom)) - ((top + bottom) / (top - bottom));

    // raster from NDC
    raster.x = ((raster.x + 1.0f) / 2.0f) * ((f32)image_width - 1.0f);
    raster.y = ((raster.y + 1.0f) / 2.0f) * ((f32)image_height - 1.0f);;

    return(raster);
}

int main(void)
{
    s32 image_width = 204;
    s32 image_height = 155;

    f32 aspect_ratio = (f32)image_width / (f32)image_height;
    f32 znear = 0.1f;
    f32 zfar = 100.0f;

    f32 vertical_fov_degrees = 90.0f;
    f32 vertical_canvas_size = 2.0f * tanf(DEG2RAD(vertical_fov_degrees) / 2.0f) * znear;
    f32 horizontal_canvas_size = vertical_canvas_size * aspect_ratio;
    f32 top = vertical_canvas_size / 2.0f;
    f32 bottom = -top;
    f32 right = horizontal_canvas_size / 2.0f;
    f32 left = -right;

    image img = CreateImage(image_width, image_height);

    ClearScreen(&img, 80, 15, 45);

    vec3 A = { 0.0f, 0.5f, 2.0f };
    vec3 B = { -0.5f, -0.5f, 2.0f };
    vec3 C = { 0.5f, -0.5f, 2.0f };

    vec3 Aprime = RasterFromView(A, left, right, bottom, top, znear, image_width, image_height);
    vec3 Bprime = RasterFromView(B, left, right, bottom, top, znear, image_width, image_height);
    vec3 Cprime = RasterFromView(C, left, right, bottom, top, znear, image_width, image_height);

    WritePixel(&img, (s32)Aprime.x, (s32)Aprime.y, 255, 255, 255);
    WritePixel(&img, (s32)Bprime.x, (s32)Bprime.y, 255, 255, 255);
    WritePixel(&img, (s32)Cprime.x, (s32)Cprime.y, 255, 255, 255);

    WriteImageToFile(&img, "out.png");

    exit(0);
}