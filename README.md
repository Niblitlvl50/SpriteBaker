# Baker

A simple tool for baking a bunch of images into one larger image. 

### Usage
```
baker -width 512 -height 512 -input [image1.png image1.png ...] -output sprite_atlas.png
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
-bg_color       Red, green, blue, alpha channel color values for the background, value between 0 and 255.
-padding        Padding around each sub image in pixels.
-trim_images    Trim fully transparent pixels in the input images.
-sprite_format  Output special sprite format. 
```

[input1]: https://github.com/Niblitlvl50/Baker/blob/master/res/cat-bump.png
[input2]: https://github.com/Niblitlvl50/Baker/blob/master/res/cat-jump-1.png
[input3]: https://github.com/Niblitlvl50/Baker/blob/master/res/cat-jump-2.png
[baked_image]: https://github.com/Niblitlvl50/Baker/blob/master/res/baked_image.png


### Implementation

This tools is build using [nothings stb libraries](https://github.com/nothings/stb), image reader/writer library as well as the rect packing library. For reading and writing json files [nlohmann's json library](https://github.com/nlohmann/json) is used.
