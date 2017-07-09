# Sprite Baker

A simple tool for baking a bunch of images into one larger image. Will write a sprite atlas and a json file that tells you where the sub images are. 

### Usage
```
spritebaker -width 512 -height 512 -input [image1.png image1.png ...] -output sprite_atlas.png
```

#### Arguments

*Required*
```
-width		Output image width.
-height		Output image height.
-input		A list of input images that will be packed into the output image.
-output		The output image file name.
```

*Optional*
```
-bg_color       Red, green, blue, alpha channel color values for the background. Range 0 - 255.
-padding        Padding around each sub image in pixels.
-trim_images    Trim fully transparent pixels in the input images.
-sprite_format  Output special sprite format. 
```

# Example

Here's two outputs from the tool. The first one is 650 x 650 without trimming and the second one is 512 x 512 with trimming. 

<img src="https://github.com/Niblitlvl50/Baker/blob/master/res/baked_image.png" width="256" /> <img src="https://github.com/Niblitlvl50/Baker/blob/master/res/baked_image_trimmed.png" width="256 /">


### Implementation

This tools is build using [nothings stb libraries](https://github.com/nothings/stb), image reader/writer library as well as the rect packing library. For reading and writing json files [nlohmann's json library](https://github.com/nlohmann/json) is used.
