#include "base.h"
#include "asset_manager.h"
#include "utils/list.h"

#include <stdio.h>
#if NAUI_WINDOWS
#include <dirent/dirent.h>
#else
#include <dirent.h>
#endif
#include <stb/stb_image.h>
#include <stb/stb_rect_pack.h>

#define NAUI_IMAGE_ATLAS_SIZE 4096

static Naui_ImageHashEntry *image_hm = NULL;

// TODO(doomguy): use arena for this.
Naui_Map(Naui_ImageHashEntry) naui_asset_manager_load_images(const char *const images_path)
{
    typedef struct
    {
        int32_t width, height;
        uint8_t *pixels;
    }
    Naui_TempImageData;

    Naui_List(Naui_TempImageData) images = NULL;

    // looping thru the images directory.
    {
        struct dirent *dp;
        DIR *dir = opendir(images_path);
        if (!dir)
        {
            fprintf(stderr, "[Naui]: failed to open %s directory for images\n", images_path);
            exit(1);
        }

        char path[256];

        // TODO(doomguy): does windows have the ./.. directories when using dirent?
        // if yes, replace the strcmp with a simple int.

        while (dp = readdir(dir))
        {
            if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) continue;

            // this is a hack, use getcwd instead.
            snprintf(path, sizeof(path), "%s/%s", images_path, dp->d_name);
            char name[128];
            strncpy(name, dp->d_name, sizeof(name));
            name[sizeof(name) - 1] = '\0';
            char *image_name = strtok(name, "."); // this is also a hack, should iterate backwards thru str instead.

            Naui_TempImageData image;
            int32_t temp_channels;
            image.pixels = stbi_load(path, &image.width, &image.height, &temp_channels, 4);
            if (!image.pixels)
            {
                fprintf(stderr, "[Naui]: failed to load image: %s\n", path);
                exit(1);
            }
            const Naui_Image sprite = (Naui_Image){ .width = image.width, .height = image.height };
            naui_strmap_put(image_hm, strdup(image_name), sprite);
            naui_list_push(images, image);
        }
    }

    // building the atlas.
    {
        const size_t node_count = NAUI_IMAGE_ATLAS_SIZE,
                     atlas_size = NAUI_IMAGE_ATLAS_SIZE * NAUI_IMAGE_ATLAS_SIZE * 4,
                     image_count = naui_list_len(images);

        stbrp_context ctx;
        stbrp_node *nodes = malloc(sizeof(*nodes) * node_count);
        stbrp_rect *rects = malloc(sizeof(*rects) * image_count);
        uint8_t *pixels = malloc(atlas_size);

        stbrp_init_target(&ctx, NAUI_IMAGE_ATLAS_SIZE, NAUI_IMAGE_ATLAS_SIZE, nodes, node_count);
        for (int i=0; i < image_count; ++i)
        {
            stbrp_rect *r = &rects[i];
            const Naui_TempImageData *img = &images[i];
            r->w = img->width, r->h = img->height;
            r->id = i;
        }

        if (!stbrp_pack_rects(&ctx, rects, image_count))
        {
            fprintf(stderr, "[Naui]: image atlas size is not enough\n");
            exit(1);
        }

        for (int i = 0; i < image_count; ++i)
        {
            const stbrp_rect r = rects[i];
            Naui_Image *sprite = &image_hm[i].value;

            for (int y = 0; y < r.h; ++y)
            {
                uint8_t *dst_row = pixels + ((r.y + y) * NAUI_IMAGE_ATLAS_SIZE + r.x) * 4;
                uint8_t *src_row = images[i].pixels + (y * r.w) * 4;
                memcpy(dst_row, src_row, r.w * 4);
            }

            const float inv_img_atlas_size = 1.0f / (float)NAUI_IMAGE_ATLAS_SIZE;
            sprite->texture_area[0] = (float)r.x * inv_img_atlas_size;
            sprite->texture_area[1] = (float)r.y * inv_img_atlas_size;
            sprite->texture_area[2] = (float)r.w * inv_img_atlas_size;
            sprite->texture_area[3] = (float)r.h * inv_img_atlas_size;
        }

        extern void naui_renderer_build_atlas(uint32_t width, uint32_t height, void *data);
        naui_renderer_build_atlas(NAUI_IMAGE_ATLAS_SIZE, NAUI_IMAGE_ATLAS_SIZE, pixels);

        free(nodes);
        free(rects);
        free(pixels);
    }

    for (int32_t i = 0; i < naui_list_len(images); ++i)
        stbi_image_free(images[i].pixels);
    naui_list_free(images);

    return image_hm;
}

void naui_asset_manager_free(void)
{
    naui_strmap_free(image_hm);
}

const Naui_Image *naui_get_image(const char *const name)
{
    return &naui_strmap_get(image_hm, name);
}