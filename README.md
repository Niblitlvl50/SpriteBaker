# Baker

Usage
```
baker -width 512 -height 512 -input [image1.png image1.png ...] -output sprite_atlas.png
```

Arguments

*Required*
```
-width		Output image width.
-height		Output image height.
-input		A list of input images that will be packed into the output image.
-output		The output image file name.
```

*Optional*
```
-bg_red		Red channel color value for the background, value between 0 and 255.
-bg_green	Green channel color value for the background, value between 0 and 255.
-bg_blue	Blue channel color value for the background, value between 0 and 255.
-bg_alpha	Alpha channel color value for the background, value between 0 and 255.
-padding	Padding around each sub image in pixels.
```
