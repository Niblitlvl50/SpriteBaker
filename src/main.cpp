
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_RECT_PACK_IMPLEMENTATION

#include "stb_image.h"
#include "stb_image_write.h"
#include "stb_rect_pack.h"

#include "json.hpp"

#include <vector>
#include <string>
#include <unordered_map>
#include <fstream>
#include <regex>
#include <limits>
#include <chrono>

constexpr const char* version = "1.6.0";

struct Context
{
    // Required
    std::vector<std::string> input_files;
    std::string output_file;
    int output_width;
    int output_height;

    // Optional arguments
    int padding = 0;
    unsigned char background_r = 0;
    unsigned char background_g = 0;
    unsigned char background_b = 0;
    unsigned char background_a = 0;
    bool trim_images = false;
    bool write_sprite_format = false;
    std::string sprite_folder;
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
            options_table[current_option];
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

    const bool has_width = width_it != end && !width_it->second.empty();
    const bool has_height = height_it != end && !height_it->second.empty();
    const bool has_input = input_it != end && !input_it->second.empty();
    const bool has_output = output_it != end && !output_it->second.empty();

    if(!has_width)
        throw std::runtime_error("Invalid arguments, missing valid 'width'.");
    else if(!has_height)
        throw std::runtime_error("Invalid arguments, missing valid 'height'.");
    else if(!has_input)
        throw std::runtime_error("Invalid arguments, missing valid 'input'.");
    else if(!has_output)
        throw std::runtime_error("Invalid arguments, missing valid 'output'.");

    context.output_width = std::stoi(width_it->second);
    context.output_height = std::stoi(height_it->second);
    context.output_file = output_it->second;

    std::string temp;
    std::istringstream input_stream(input_it->second);
    while(input_stream >> temp)
        context.input_files.push_back(temp);

    const auto padding_it = options_table.find("padding");
    if(padding_it != options_table.end())
        context.padding = std::stoi(padding_it->second);

    const auto bg_color_it = options_table.find("bg_color");
    if(bg_color_it != end)
    {
        int red, green, blue, alpha = 0;
        std::istringstream(bg_color_it->second) >> red >> green >> blue >> alpha;

        context.background_r = red;
        context.background_g = green;
        context.background_b = blue;
        context.background_a = alpha;
    }

    context.trim_images = (options_table.find("trim_images") != end);
    context.write_sprite_format = (options_table.find("sprite_format") != end);
    const auto sprite_folder_it = options_table.find("sprite_folder");
    if(sprite_folder_it != options_table.end())
        context.sprite_folder = sprite_folder_it->second;
}

void TrimImage(ImageData& image)
{
    const size_t width_in_bytes = image.width * 4;

    size_t first_non_transparent = std::numeric_limits<size_t>::max();
    size_t last_non_transparent = 0;

    size_t left_non_transparent = std::numeric_limits<size_t>::max();
    size_t right_non_transparent = 0;

    // First alpha component is at index 3 and the stride to next is 4
    for(size_t index = 3; index < image.data.size(); index += 4)
    {
        const unsigned char alpha = image.data[index];
        if(alpha != 0)
        {
            first_non_transparent = std::min(first_non_transparent, index);
            last_non_transparent = std::max(last_non_transparent, index);

            const size_t current_row = index / width_in_bytes;
            const size_t row_index = index - current_row * width_in_bytes;

            left_non_transparent = std::min(left_non_transparent, row_index);
            right_non_transparent = std::max(right_non_transparent, row_index);
        }
    }

    // Correct the byte index to start at the red component
    first_non_transparent -= 3;
    last_non_transparent -= 3;
    left_non_transparent -= 3;
    right_non_transparent -= 3;

    const size_t top_rows_to_delete = first_non_transparent / width_in_bytes;
    const size_t bottom_rows_to_delete = image.height - (last_non_transparent / width_in_bytes) -1;

    const size_t bytes_to_copy = right_non_transparent - left_non_transparent + 4;

    ImageData trimmed_image;
    trimmed_image.width = bytes_to_copy / 4;
    trimmed_image.height = image.height - top_rows_to_delete - bottom_rows_to_delete;
    trimmed_image.color_components = image.color_components;
    trimmed_image.data.resize(trimmed_image.width * trimmed_image.height * 4);

    for(size_t row = 0; row < size_t(trimmed_image.height); ++row)
    {
        const size_t destination_offset = row * trimmed_image.width * 4;
        const size_t source_offset = (row + top_rows_to_delete) * width_in_bytes;
        void* dest = trimmed_image.data.data() + destination_offset;
        const void* src = image.data.data() + source_offset + left_non_transparent;
        std::memcpy(dest, src, bytes_to_copy);
    }

    image = std::move(trimmed_image);
}

std::vector<ImageData> LoadImages(const std::vector<std::string>& image_files, bool trim_images)
{
    std::vector<ImageData> images;
    images.reserve(image_files.size());

    for(const std::string& file : image_files)
    {
        ImageData image;

        // Force to 4 color components (RGBA).
        constexpr int color_components = 4;
        stbi_uc* data = stbi_load(file.c_str(), &image.width, &image.height, &image.color_components, color_components);
        if(!data)
            throw std::runtime_error(stbi_failure_reason() + std::string(" '") + file + "'");

        const int image_size = image.width * image.height * color_components;
        image.data.resize(image_size);
        std::memcpy(image.data.data(), data, image_size);

        if(trim_images)
            TrimImage(image);

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
        rect.w -= (padding * 2);
        rect.h -= (padding * 2);
    }

    return pack_rects;
}

void WriteImage(const std::vector<ImageData>& images, const std::vector<stbrp_rect>& rects, const Context& context)
{
    const int width = context.output_width;
    const int height = context.output_height;

    // RGBA
    constexpr int color_components = 4;
    const int image_size = width * height * color_components;
    std::vector<unsigned char> output_image_bytes(image_size, 0);

    for(size_t index = 0; index < output_image_bytes.size(); ++index)
    {
        output_image_bytes[index] = context.background_r;
        output_image_bytes[++index] = context.background_g;
        output_image_bytes[++index] = context.background_b;
        output_image_bytes[++index] = context.background_a;
    }

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
    const bool success = stbi_write_png(context.output_file.c_str(), width, height, 4, output_image_bytes.data(), stride) != 0;
    if(!success)
        throw std::runtime_error("Unable to write output image");
}

void WriteSpriteFiles(const std::vector<stbrp_rect>& rects, const Context& context)
{
    struct RectId_Suffix
    {
        size_t rect_id;
        std::string name_suffix;
    };

    std::string output_folder;

    const size_t slash_pos = context.output_file.find_last_of('/');
    if(slash_pos != std::string::npos)
        output_folder = context.output_file.substr(0, slash_pos +1);

    const std::regex filename_matcher("(.+\\/)?(\\S*?)([\\d]+)?\\.");
    std::unordered_map<std::string, std::vector<RectId_Suffix>> sprite_files;

    for(size_t index = 0; index < context.input_files.size(); ++index)
    {
        const std::string& file = context.input_files[index];

        std::smatch match_result;
        if(!std::regex_search(file, match_result, filename_matcher))
            continue;

        //const std::string& folder_capture = match_result[1];
        const std::string& filename_capture = match_result[2];
        const std::string& integer_capture = match_result[3];
        //std::printf("Folder: %s, file: %s, int: %s\n",
        //    folder_capture.c_str(),
        //    filename_capture.c_str(),
        //    integer_capture.c_str());

        RectId_Suffix id_suffix;
        id_suffix.rect_id = index;
        id_suffix.name_suffix = integer_capture;

        std::vector<RectId_Suffix>& frame_ids = sprite_files[filename_capture];
        frame_ids.push_back(id_suffix);
    }

    const std::string real_output_folder = (context.sprite_folder.empty() ? output_folder : context.sprite_folder);

    nlohmann::json all_sprite_files;

    for(const auto& pair : sprite_files)
    {
        const std::string& sprite_file = real_output_folder + pair.first + ".sprite";

        nlohmann::json default_frames;
        default_frames.push_back(0);
        default_frames.push_back(-50);
        
        nlohmann::json default_animation;
        default_animation["name"] = "default";
        default_animation["loop"] = true;
        default_animation["frames"] = default_frames;

        nlohmann::json animations;
        animations.push_back(default_animation);

        try
        {
            std::ifstream input_stream(sprite_file);
            const nlohmann::json& parsed_sprite_file = nlohmann::json::parse(input_stream);
            auto anim_it = parsed_sprite_file.find("animations");
            if(anim_it != parsed_sprite_file.end() && anim_it->is_array())
                animations = *anim_it;
        }
        catch(const std::exception& error)
        { 
            std::printf("%s\n", error.what());
        }
        catch(...)
        { }

        nlohmann::json frames;

        for(const RectId_Suffix& frame_index : pair.second)
        {
            const stbrp_rect& rect = rects[frame_index.rect_id];

            nlohmann::json object;
            object["name"] = pair.first + frame_index.name_suffix;
            object["x"] = rect.x;
            object["y"] = rect.y;
            object["w"] = rect.w;
            object["h"] = rect.h;
            object["x_offset"] = 0.0f;
            object["y_offset"] = 0.0f;
            frames.push_back(object);
        }

        std::ofstream out_file(sprite_file);
        if(!out_file)
            throw std::runtime_error("Unable to write to '" + sprite_file + "'");

        nlohmann::json texture_size;
        texture_size["w"] = context.output_width;
        texture_size["h"] = context.output_height;

        nlohmann::json json;
        json["texture"] = context.output_file;
        json["texture_size"] = texture_size;
        json["animations"] = animations;
        json["frames"] = frames;

        out_file << std::setw(4) << json << std::endl;
        all_sprite_files.push_back(sprite_file);
    }

    nlohmann::json all_sprite_files_json;
    all_sprite_files_json["all_sprites"] = all_sprite_files;

    std::ofstream out_all_file(real_output_folder + "all_sprite_files.json");
    out_all_file << std::setw(4) << all_sprite_files_json << std::endl;

}

void WriteGenericJson(const std::vector<stbrp_rect>& rects, const Context& context)
{
    nlohmann::json frames;

    for(const stbrp_rect& rect : rects)
    {
        nlohmann::json frame;
        frame["x"] = rect.x;
        frame["y"] = rect.y;
        frame["w"] = rect.w;
        frame["h"] = rect.h;

        nlohmann::json pivot;
        pivot["x"] = 0.5f;
        pivot["y"] = 0.5f;

        nlohmann::json source_size;
        source_size["w"] = rect.w;
        source_size["h"] = rect.h;

        nlohmann::json sprite_source_size;
        sprite_source_size["x"] = 0;
        sprite_source_size["y"] = 0;
        sprite_source_size["w"] = rect.w;
        sprite_source_size["h"] = rect.h;

        nlohmann::json object;
        object["filename"] = context.input_files[rect.id];
        object["rotated"] = false;
        object["trimmed"] = context.trim_images;
        object["frame"] = frame;
        object["pivot"] = pivot;
        object["sourceSize"] = source_size;
        object["spriteSourceSize"] = sprite_source_size;

        frames.push_back(object);
    }

    nlohmann::json output_size;
    output_size["w"] = context.output_width;
    output_size["h"] = context.output_height;

    nlohmann::json meta;
    meta["app"]     = "https://github.com/Niblitlvl50/SpriteBaker";
    meta["version"] = "1.0";
    meta["image"]   = context.output_file;
    meta["format"]  = "RGBA8888";
    meta["size"]    = output_size;
    meta["scale"]   = "1";

    nlohmann::json json;
    json["frames"]  = frames;
    json["meta"]    = meta;

    const size_t dot_pos = context.output_file.find_last_of(".");
    const std::string& json_filename = context.output_file.substr(0, dot_pos) + ".json";

    std::ofstream out_file(json_filename);
    out_file << std::setw(4) << json << std::endl;
}

int main(int argv, const char* argc[])
{
    const auto& start_time = std::chrono::system_clock::now();
    
    Context context;

    try
    {
        ParseArguments(argv, argc, context);
        const std::vector<ImageData>& images = LoadImages(context.input_files, context.trim_images);
        const std::vector<stbrp_rect>& rects = PackImages(images, context.output_width, context.output_height, context.padding);
        WriteImage(images, rects, context);
        
        if(context.write_sprite_format)
            WriteSpriteFiles(rects, context);
        else
            WriteGenericJson(rects, context);
    }
    catch(const std::runtime_error& error)
    {
        std::printf("\nError: %s\n", error.what());
        std::printf("\n");
        std::printf("Usage: spritebaker -width 512 -height 512 -input [image1.png image1.png ...] -output sprite_atlas.png\n");
        std::printf("Required arguments:\n");
        std::printf("\t-width, -height, -input, -output\n");
        std::printf("\n");
        std::printf("Optional arguments:\n");
        std::printf("\t-bg_color [r g b a, 0 - 255], -padding [>= 0], -trim_images [flag], -sprite_format [flag]\n");
        std::printf("\nVersion: %s\n", version);
        std::printf("\n");

        return 1;
    }

    const auto& time_diff = std::chrono::system_clock::now() - start_time;
    const auto& ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_diff);

    std::printf("Successfully baked\n");
    for(const std::string& file : context.input_files)
        std::printf("\t'%s'\n", file.c_str());
    
    std::printf("to '%s' during %lld ms\n", context.output_file.c_str(), ms.count());

    return 0;
}
