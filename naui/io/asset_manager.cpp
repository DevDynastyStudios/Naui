#include "asset_manager.h"

#include <unordered_map>
#include <filesystem>
#include <string>

static std::unordered_map<std::string, NauiImage> images;

void naui_asset_manager_initialize(const char *assets_directory)
{
    namespace fs = std::filesystem;

    if (!fs::exists(assets_directory))
        return;
    
    for (auto &entry : fs::directory_iterator(assets_directory))
    {
        if (!entry.is_regular_file())
            continue;

        auto path = entry.path();
        if (path.extension() == ".png")
        {
            auto name = path.stem().string();
            auto image = naui_create_image(path.string().c_str());
            images[name] = image;
        }
    }
}

void naui_asset_manager_shutdown(void)
{
    for (auto& image : images)
        naui_destroy_image(&image.second);
}

NauiImage &naui_get_image(const char *path)
{
    return images[path];
}