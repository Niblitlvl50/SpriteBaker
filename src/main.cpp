
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_RECT_PACK_IMPLEMENTATION

#include "stb_image.h"
#include "stb_image_write.h"
#include "stb_rect_pack.h"

#include <vector>
#include <string>
#include <cstdio>
#include <unordered_map>

struct Context
{
    std::vector<std::string> input_files;
    std::string output_file;
    int output_width;
    int output_height;
    int padding = 0;
};

struct ImageData
{
    int width;
    int height;
    int color_components;
    std::vector<unsigned char> data;
};

void ParseArguments(int argv, const char** argc, Context& context)
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

    const auto width_it = options_table.find("width");
    const auto height_it = options_table.find("height");
    const auto input_it = options_table.find("input");
    const auto output_it = options_table.find("output");
    const auto end = options_table.end();

    if(width_it == end || height_it == end || input_it == end || output_it == end)
        throw std::runtime_error("Invalid arguments");

    context.output_width = std::stoi(width_it->second);
    context.output_height = std::stoi(height_it->second);
    context.output_file = output_it->second;

    char* data = &input_it->second.front();
    while(const char* token = std::strtok(data, " "))
    {
        context.input_files.push_back(token);
        data = nullptr;
    }

    const auto padding_it = options_table.find("padding");
    if(padding_it != options_table.end())
        context.padding = std::stoi(padding_it->second);
}

std::vector<ImageData> LoadImages(const std::vector<std::string>& image_files)
{
    std::vector<ImageData> images;
    images.reserve(image_files.size());

    for(const std::string& file : image_files)
    {
        ImageData image;
        stbi_uc* data = stbi_load(file.c_str(), &image.width, &image.height, &image.color_components, 0);
        if(!data)
            throw std::runtime_error(stbi_failure_reason() + std::string(" '") + file + "'");

        const int image_size = image.width * image.height * image.color_components;
        image.data.resize(image_size);
        std::memcpy(image.data.data(), data, image_size);
        images.push_back(std::move(image));

        stbi_image_free(data);
    }

    return images;
}

std::vector<stbrp_rect> PackImages(const std::vector<ImageData>& images, int width, int height, int padding)
{
    std::vector<stbrp_rect> pack_rects;
    pack_rects.reserve(images.size());

    for(size_t index = 0; index < images.size(); ++index)
    {
        const ImageData& image_data = images[index];

        stbrp_rect rect;
        rect.id = index;
        rect.w = image_data.width + padding * 2;
        rect.h = image_data.height + padding * 2;

        pack_rects.push_back(rect);
    }

    stbrp_context pack_context;
    
    std::vector<stbrp_node> nodes;
    nodes.resize(width);

    stbrp_init_target(&pack_context, width, height, nodes.data(), nodes.size());
    const bool success = stbrp_pack_rects(&pack_context, pack_rects.data(), pack_rects.size()) != 0;
    if(!success)
        throw std::runtime_error("Unable to pack all images, consider a bigger output image.");

    for(stbrp_rect& rect : pack_rects)
    {
        rect.x += padding;
        rect.y += padding;
    }

    return pack_rects;
}

void WriteImage(
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

            std::memcpy(&output_image_bytes[output_offset], &image.data[image_offset], bytes_to_copy);
        }
    }

    constexpr int stride = 0;
    const bool success = stbi_write_png(output_file.c_str(), width, height, 4, output_image_bytes.data(), stride) != 0;
    if(!success)
        throw std::runtime_error("Unable to write output image");
}

bool WriteSpriteFiles(const std::vector<stbrp_rect>& rects)
{
    return true;
}

int main(int argv, const char* argc[])
{
    Context context;

    try
    {
        ParseArguments(argv, argc, context);
        const std::vector<ImageData>& images = LoadImages(context.input_files);
        const std::vector<stbrp_rect>& rects = PackImages(images, context.output_width, context.output_height, context.padding);
        WriteImage(images, rects, context.output_file, context.output_width, context.output_height);
        WriteSpriteFiles(rects);
    }
    catch(const std::runtime_error& error)
    {
        std::printf("\n%s\n", error.what());
        std::printf("\n");
        std::printf("Usage: baker -width 512 -height 512 -padding 4 -input [image1.png image1.png ...] -output sprite_atlas.png\n");
        std::printf("\n");

        return 1;
    }

    std::printf("Successfully baked\n");
    for(const std::string& file : context.input_files)
        std::printf("\t'%s'\n", file.c_str());
    
    std::printf("to '%s'\n", context.output_file.c_str());

    return 0;
}
