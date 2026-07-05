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

f32 GetDepth(image *img, s32 x, s32 y)
{
    if(x < 0 || y < 0 || x >= img->width || y >= img->height)
    {
        return FLT_MAX;
    }
    f32 depth = img->zbuffer[y * img->width + x];
    return(depth);
}

void WriteDepth(image *img, s32 x, s32 y, f32 depth)
{
    if(x < 0 || y < 0 || x >= img->width || y >= img->height)
    {
        return;
    }
    img->zbuffer[y * img->width + x] = depth;
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
    vec3 raster = { FLT_MAX, FLT_MAX, FLT_MAX };

    // screen from view
    raster.x = (p.x * znear) / -p.z;
    raster.y = (p.y * znear) / -p.z;
    raster.z = -p.z;

    // NDC from screen (here NDC is [-1, +1])
    raster.x = ((2.0f * raster.x) / (right - left)) - ((right + left) / (right - left));
    raster.y = ((2.0f * raster.y) / (top - bottom)) - ((top + bottom) / (top - bottom));

    // raster from NDC
    raster.x = ((raster.x + 1.0f) / 2.0f) * ((f32)image_width - 1.0f);
    raster.y = ((-raster.y + 1.0f) / 2.0f) * ((f32)image_height - 1.0f);

    return(raster);
}

f32 GetTriangleArea(vec3 P, vec3 A, vec3 B)
{
    f32 result = (A.x - B.x) * (P.y - A.y) - (A.y - B.y) * (P.x - A.x);
    return(result);
}

struct rect2
{
    vec2 min_corner;
    vec2 max_corner;
};

rect2 GetTriangleBounds(vec3 A, vec3 B, vec3 C)
{
    f32 min_x = MIN(A.x, MIN(B.x, C.x));
    f32 max_x = MAX(A.x, MAX(B.x, C.x));
    f32 min_y = MIN(A.y, MIN(B.y, C.y));
    f32 max_y = MAX(A.y, MAX(B.y, C.y));
    rect2 result;
    result.min_corner = { min_x, min_y };
    result.max_corner = { max_x, max_y };
    return(result);
}

vec3 background_color = { 0.31f, 0.058f, 0.176f };
void DrawTriangle(image *img, vec3 V0, vec3 V1, vec3 V2)
{
    f32 total_area = GetTriangleArea(V0, V1, V2);
    if(NearZero(total_area))
    {
        return;
    }
    rect2 bbox = GetTriangleBounds(V0, V1, V2);
    s32 min_x = MAX((s32)(bbox.min_corner.x), 0);
    s32 min_y = MAX((s32)(bbox.min_corner.y), 0);
    s32 max_x = MIN((s32)ceilf(bbox.max_corner.x), img->width - 1);
    s32 max_y = MIN((s32)ceilf(bbox.max_corner.y), img->height - 1);

    for(s32 x = min_x; x <= max_x; x++)
    {
        for(s32 y = min_y; y <= max_y; y++)
        {
            vec3 rasterP = { (f32)x + 0.5f, (f32)y + 0.5f, FLT_MAX };
            vec3 samples[4]
            {
                { rasterP.x - 0.25f, rasterP.y - 0.25f },
                { rasterP.x - 0.25f, rasterP.y + 0.25f },
                { rasterP.x + 0.25f, rasterP.y - 0.25f },
                { rasterP.x + 0.25f, rasterP.y + 0.25f },
            };
            vec3 color = { 0.0f, 0.0f, 0.0f };
            f32 closest_depth = FLT_MAX;
            for(s32 i = 0; i < 4; i++)
            {
                vec3 P = samples[i];
                f32 w0 = GetTriangleArea(P, V1, V2);
                f32 w1 = GetTriangleArea(P, V2, V0);
                f32 w2 = GetTriangleArea(P, V0, V1);
                if(w0 >= 0 && w1 >= 0 && w2 >= 0)
                {
                    w0 /= total_area;
                    w1 /= total_area;
                    w2 /= total_area;
                    vec3 tmp = { w0, w1, w2 };
                    color += tmp;
                    f32 depth = V0.z + w1 * (V1.z - V0.z) + w2 * (V2.z - V0.z);
                    if(depth < closest_depth)
                    {
                        closest_depth = depth;
                    }
                }
                else
                {
                    color += background_color;
                }
            }
            color /= 4;
            if(closest_depth < GetDepth(img, x, y))
            {
                WritePixel(img, x, y, (u8)(color.x * 255), (u8)(color.y * 255), (u8)(color.z * 255));
                WriteDepth(img, x, y, closest_depth);
            }
        }
    }
}

struct triangle_mesh
{
    vec3 *vertices;
    u32 vertex_count;
    u32 *indices;
    u32 index_count; 
};

triangle_mesh CreateMeshFromFile(const char *filename)
{
    triangle_mesh mesh = {};
    FILE *f = fopen(filename, "r");
    if(!f)
    {
        printf("cannot open file: %s.\n", filename);
        return(mesh);
    }
    char line[256];
    u32 vertex_count = 0;
    u32 index_count = 0;
    while(fgets(line, 128, f) != NULL)
    {
        if(line[0] == 'v')
        {
            vertex_count++;
        }
        else if(line[0] == 'f')
        {
            index_count+=3;
        }
    }
    fseek(f, 0, SEEK_SET);
    vec3 *vertices = (vec3*)malloc((vertex_count + 1) * sizeof(vec3));
    u32 *indices = (u32*)malloc(index_count * sizeof(u32));
    u32 vi = 1;
    u32 fi = 0;

    while(fgets(line, 256, f) != NULL)
    {
        if(line[0] == 'v')
        {
            f32 x, y, z;
            if(sscanf(line + 1, "%f %f %f", &x, &y, &z) != 3)
            {
                printf("error.\n");
            }
            vertices[vi++] = {x, y, z};
        }
        else if(line[0] == 'f')
        {
            u32 a, b, c;
            if(sscanf(line + 1, "%u %u %u", &a, &b, &c) != 3)
            {
                // TODO: handle format with /
                printf("error.\n");
            }
            indices[fi++] = a;
            indices[fi++] = b;
            indices[fi++] = c;
        }
    }
    fclose(f);

    mesh.vertex_count = vertex_count;
    mesh.index_count = index_count;
    mesh.vertices = vertices;
    mesh.indices = indices;
    return(mesh);
}

void DrawMesh(image *img, triangle_mesh *mesh, f32 left, f32 right, f32 bottom, f32 top, f32 znear)
{
    mat4x4 transform = Scaling(0.2f) * RotationY(-45.0f) * Translation(0.0f, 0.0f, -1.5f);
    for(u32 i = 0; i < mesh->index_count; i+=3)
    {
        u32 idx0 = mesh->indices[i]; 
        u32 idx1 = mesh->indices[i + 1];
        u32 idx2 = mesh->indices[i + 2];

        vec3 V0 = RasterFromView(transform * mesh->vertices[idx0], left, right, bottom, top, znear, img->width, img->height);
        vec3 V1 = RasterFromView(transform * mesh->vertices[idx1], left, right, bottom, top, znear, img->width, img->height);
        vec3 V2 = RasterFromView(transform * mesh->vertices[idx2], left, right, bottom, top, znear, img->width, img->height);

        DrawTriangle(img, V0, V1, V2);
    }
}

int main(void)
{
    s32 image_width = 1920;
    s32 image_height = 1080;

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

    ClearScreen(&img, (u8)(background_color.x * 255), (u8)(background_color.y * 255), (u8)(background_color.z * 255));

    triangle_mesh cow = CreateMeshFromFile("assets/cow.obj");
    DrawMesh(&img, &cow, left, right, bottom, top, znear);

    WriteImageToFile(&img, "out.png");

    exit(0);
}