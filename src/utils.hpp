#pragma once
#include <string>
class UIImage;
class UIText;

UIText* text_for_img(std::string str, UIImage *img);
UIImage* img_for_text(UIText *txt);

#define TINT_ON_CLICK(img, norm, pressed) (img)->tint = Vector4 norm;\
    (img)->onTouchDown = []() { UIImage::get_held()->tint = Vector4 pressed; }; \
    (img)->onTouchUp = []() { UIImage::get_held()->tint = Vector4 norm; }
