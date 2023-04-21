#define _CRT_SECURE_NO_DEPRECATE
#include <stdio.h>
#define PNG_DEBUG 3
#include "png.h"
#include <stdlib.h>
#include <stdint.h>
#include <malloc.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>

/* A coloured pixel. */

typedef struct
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t alpha;
} pixel_t;

/* A picture. */

typedef struct
{
    pixel_t *pixels;
    size_t width;
    size_t height;
} bitmap_t;

/* Given "bitmap", this returns the pixel of bitmap at the point
("x", "y"). */

static pixel_t *pixel_at(bitmap_t *bitmap, int x, int y)
{
    return bitmap->pixels + bitmap->width * y + x;
}

// Write "bitmap" to a PNG file specified by "path"; returns 0 on
// success, non-zero on error.
static int save_png_to_file(bitmap_t *bitmap, const char *path)
{
    FILE *fp;
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    size_t x, y;
    png_bytepp row_pointers = NULL;
    /* "status" contains the return value of this function. At first
    it is set to a value which means 'failure'. When the routine
    has finished its work, it is set to a value which means
    'success'. */
    int status = -1;
    /* The following number is set by trial and error only. I cannot
    see where it it is documented in the libpng manual.
    */
    int pixel_size = 4;
    int depth = 8;

    fp = fopen(path, "wb");
    if (!fp)
    {
        goto fopen_failed;
    }

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL)
    {
        goto png_create_write_struct_failed;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL)
    {
        goto png_create_info_struct_failed;
    }

    /* Set up error handling. */

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        goto png_failure;
    }

    /* Set image attributes. */

    png_set_IHDR(png_ptr,
                 info_ptr,
                 bitmap->width,
                 bitmap->height,
                 depth,
                 PNG_COLOR_TYPE_RGBA,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);

    /* Initialize rows of PNG. */

    row_pointers =
        (png_bytepp)png_malloc(png_ptr, bitmap->height * sizeof(png_byte *));
    for (y = 0; y < bitmap->height; ++y)
    {
        png_bytep row =
            (png_bytep)png_malloc(png_ptr,
                                  sizeof(uint8_t) * bitmap->width * pixel_size);
        row_pointers[y] = row;
        for (x = 0; x < bitmap->width; ++x)
        {
            pixel_t *pixel = pixel_at(bitmap, x, y);
            *row++ = pixel->red;
            *row++ = pixel->green;
            *row++ = pixel->blue;
            *row++ = pixel->alpha;
        }
    }

    /* Write the image data to "fp". */

    png_init_io(png_ptr, fp);
    png_set_rows(png_ptr, info_ptr, row_pointers);
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

    /* The routine has successfully written the file, so we set
    "status" to a value which indicates success. */

    status = 0;

    for (y = 0; y < bitmap->height; y++)
    {
        png_free(png_ptr, row_pointers[y]);
    }
    png_free(png_ptr, row_pointers);

png_failure:
png_create_info_struct_failed:
    png_destroy_write_struct(&png_ptr, &info_ptr);
png_create_write_struct_failed:
    fclose(fp);
fopen_failed:
    return status;
}

/* Given "value" and "max", the maximum value which we expect "value"
to take, this returns an integer between 0 and 255 proportional to
"value" divided by "max". */
static int map(int value, int max)
{
    if (value < 0)
        return 0;
    return (int)(256.0 * ((double)(value) / (double)max));
}
float float_rand(float min, float max)
{
    float scale = rand() / (float)RAND_MAX; /* [0, 1.0] */
    return min + scale * (max - min);       /* [min, max] */
}

void fill_rand(int *line, int n, float fill)
{
    for (int i = 0; i < n * n; i++)
    {
        line[i] = (float_rand(0, 1) <= fill) ? (1) : (0);
    }

    line[(n / 2) * n + n / 2] = 2;
}

void print_matrix(int *line, int n, bitmap_t *image)
{
    unsigned int count = 0;
    pixel_t *pixel;
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            if (line[i * n + j] == 0)
            {
                pixel = pixel_at(image, i, j);
                pixel->red = 255;
                pixel->green = 255;
                pixel->blue = 0;
                pixel->alpha = 255;
            }
            else if (line[i * n + j] == 1)
            {
                pixel = pixel_at(image, i, j);
                pixel->red = 255;
                pixel->green = 0;
                pixel->blue = 0;
                pixel->alpha = 255;
            }
            else if (line[i * n + j] == 2)
            {
                pixel = pixel_at(image, i, j);
                pixel->red = 0;
                pixel->green = 255;
                pixel->blue = 0;
                pixel->alpha = 255;
            }
            else
            {
                pixel = pixel_at(image, i, j);
                pixel->red = 0;
                pixel->green = 0;
                pixel->blue = 255;
                pixel->alpha = 0;
                ++count;
            }
            // printf("%d ", a[0][i*n+j]);
        }
        // printf("\n");
    }
    printf("%d", count);
}

int transform(int i, int n, int mode)
{
    switch (mode)
    {
    default:
    case 0:
        return i;
    case 1:
        return n * n - 1 - i;
    case 2:
        return (i % n) * n + i / n;
    case 3:
        return (n - i % n - 1) * n + (n - i / n - 1);
    }
}

void apply_rules(int *line, float p1, float p2, int n, int mode)
{
    int i, j = -1; // j - индекс конца пред. подстроки
    for (i = 0; i < n * n; i++)
    {
        float p = float_rand(0, 1); //шанс применения подстановки
        float c = float_rand(0, 1); //шанс конца строки 0.5
        if (c <= 0.5 || i == n * n - 1)
        {
            if (i - j == 2)
            {
                //применяем подстановки
                int it = transform(i, n, mode), i_1t = transform(i - 1, n, mode);
                if (line[i_1t] == 1 && line[it] == 0 && p <= p1)
                {
                    line[i_1t] = 0;
                    line[it] = 1;
                }
                else if (line[i_1t] == 0 && line[it] == 1 && p <= p1)
                {
                    line[i_1t] = 1;
                    line[it] = 0;
                }
                else if (line[i_1t] == 1 && line[it] == 2 && p <= p2)
                {
                    line[i_1t] = 2;
                    line[it] = 2;
                }
            }
            j = i;
        }
    }
}

int main(void)
{
    bitmap_t image;
    int n = 10, generations = 100;
    int mode = 3;
    float p1 = 1.0;
    float p2 = 1.0;
    float fill = 0.3;

    // srand(time(NULL));

    // system("color 2");
    // system("chcp 1251");
    // system("cls");
    printf("Enter field size: ");
    scanf("%d", &n);
    printf("Enter number of generations: ");
    scanf("%d", &generations);
    printf("Enter fill percentage(0.0,1.0]: ");
    scanf("%f", &fill);
    int *line = (int *)malloc(sizeof(int) * n * n);

    /* Create an image. */
    image.width = n;
    image.height = n;
    image.pixels = (pixel_t *)malloc(sizeof(pixel_t) * image.width * image.height);

    printf("p1: %f\n", p1);
    printf("p2: %f\n", p2);

    // заполняем случайно а
    fill_rand(line, n, fill);
    // записываем матрицу в изображение
    printf("seed.png:\n\tGenerating png\n");
    print_matrix(line, n, &image);
    // сохраняем
    printf("\tSaving png\n");
    save_png_to_file(&image, "seed.png");

    //
    int it = 0;
    printf("generating:");
    int percents = 0;
    while (it < generations)
    {
        int p = (int)(((float)it) / generations * 100.f);
        while (percents < p)
        {
            percents += 5;
            printf(".");
        }
        mode = rand() % 4;                  // chose new mode randomly
        apply_rules(line, p1, p2, n, mode); // apply rules to a line
        ++it;
    }

    // записываем матрицу в изображение
    printf("\nresult.png:\n\tGenerating png");
    print_matrix(line, n, &image);
    // сохраняем
    printf("\n\tSaving png");
    save_png_to_file(&image, "result.png");

    //освобождаем ресурсы
    free(line);

    return 0;
}
