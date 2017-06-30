
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_RECT_PACK_IMPLEMENTATION

#include "stb_image.h"
#include "stb_image_write.h"
#include "stb_rect_pack.h"

#include <vector>
#include <string>
#include <memory>
#include <cstdio>
#include <unordered_map>

struct Context
{
    std::vector<std::string> input_files;
    std::string output_file;
    int output_width;
    int output_height;
};

struct ImageData
{
    int width;
    int height;
    int channels;
    std::unique_ptr<unsigned char> data;
};

bool ParseArguments(int argv, const char** argc, Context& context)
{
    std::unordered_map<std::string, std::string> options_table;
    std::string current_option;

    for(int index = 1; index < argv; ++index)
    {
        const char* arg = argc[index];
        if(*arg == '-')
        {
            current_option = arg;
            current_option.erase(0, 1);
        }
        else
        {
            std::string& value = options_table[current_option];
            if(!value.empty())
                value.append(" ");

            value.append(arg);
        }
    }

    auto width_it = options_table.find("width");
    auto height_it = options_table.find("height");
    auto input_it = options_table.find("input");
    auto output_it = options_table.find("output");
    auto end = options_table.end();

    if(width_it == end || height_it == end || input_it == end || output_it == end)
    {
        std::printf("\n");
        std::printf("Usage: baker -width 512 -height 512 -input [image1.png image1.png ...] -output sprite_atlas.png\n");
        std::printf("\n");
        return false;
    }

    context.output_width = std::stoi(width_it->second);
    context.output_height = std::stoi(height_it->second);
    context.output_file = output_it->second;

    char* data = &input_it->second.front();
    while(const char* token = std::strtok(data, " "))
    {
        context.input_files.push_back(token);
        data = nullptr;
    }

    return !context.input_files.empty();
}

std::vector<ImageData> LoadImages(const std::vector<std::string>& image_files)
{
    std::vector<ImageData> images;
    images.reserve(image_files.size());

    for(const std::string& file : image_files)
    {
        ImageData image;
        stbi_uc* data = stbi_load(file.c_str(), &image.width, &image.height, &image.channels, 0);
        if(!data)
        {
            std::printf("%s %s\n", stbi_failure_reason(), file.c_str());
            images.clear();
            break;
        }

        image.data.reset(data);
        images.push_back(std::move(image));
    }

    return images;
}

std::vector<stbrp_rect> PackImages(const std::vector<ImageData>& images, int width, int height)
{
    std::vector<stbrp_rect> pack_rects;
    pack_rects.reserve(images.size());

    for(size_t index = 0; index < images.size(); ++index)
    {
        const ImageData& image_data = images[index];

        stbrp_rect rect;
        rect.id = index;
        rect.w = image_data.width;
        rect.h = image_data.height;

        pack_rects.push_back(rect);
    }

    stbrp_context pack_context;
    
    std::vector<stbrp_node> nodes;
    nodes.resize(width);

    stbrp_init_target(&pack_context, width, height, nodes.data(), nodes.size());
    const bool success = stbrp_pack_rects(&pack_context, pack_rects.data(), pack_rects.size()) != 0;
    if(!success)
        pack_rects.clear();

    return pack_rects;
}

bool WriteImage(
    const std::vector<ImageData>& images, const std::vector<stbrp_rect>& rects, const std::string& output_file, int width, int height)
{
    // RGBA
    constexpr int color_components = 4;

    std::vector<unsigned char> output_image_bytes;
    output_image_bytes.resize(width * height * color_components);

    for(const stbrp_rect& rect : rects)
    {
        const int start_offset = rect.x + (rect.y * width);
        const ImageData& image = images[rect.id];

        for(int index = 0; index < image.height; ++index)
        {
            const int output_offset = (start_offset + (index * width)) * color_components;
            const int image_offset = index * image.width * color_components;
            const int bytes_to_copy = image.width * color_components;

            std::memcpy(&output_image_bytes[output_offset], &image.data.get()[image_offset], bytes_to_copy);
        }
    }

    constexpr int stride = 0;
    return stbi_write_png(output_file.c_str(), width, height, 4, output_image_bytes.data(), stride) != 0;
}

bool WriteSpriteFiles(const std::vector<stbrp_rect>& rects)
{
    return true;
}

int main(int argv, const char* argc[])
{
    Context context;

    const bool valid_arguments = ParseArguments(argv, argc, context);
    if(!valid_arguments)
        return std::printf("Invalid arguments.\n");

    const std::vector<ImageData>& images = LoadImages(context.input_files);
    if(images.empty())
        return std::printf("Unable to load images.\n");

    const std::vector<stbrp_rect>& rects = PackImages(images, context.output_width, context.output_height);
    if(rects.empty())
        return std::printf("Unable to pack all images.\n");

    const bool write_image_success = WriteImage(images, rects, context.output_file, context.output_width, context.output_height);
    if(!write_image_success)
        return std::printf("Unable to write output image.\n");

    const bool write_sprite_success = WriteSpriteFiles(rects);
    if(!write_sprite_success)
        return std::printf("Unable to write sprite files.\n");
    
    std::printf("Successfuly baked\n");
    for(const std::string& file : context.input_files)
        std::printf("\t'%s'\n", file.c_str());
    
    std::printf("to '%s'\n", context.output_file.c_str());

    return 0;
}
