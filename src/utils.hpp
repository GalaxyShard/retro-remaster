#pragma once
#include <string>
class UIImage;
class UIText;

UIText* text_for_img(std::string str, UIImage *img);
UIImage* img_for_text(UIText *txt);