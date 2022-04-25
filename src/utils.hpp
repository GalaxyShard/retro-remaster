#pragma once
#include <string>
class UIImage;
class UIText;

UIText* text_for_img(std::string str, UIImage *img);
UIImage* img_for_text(UIText *txt);

#define TINT_ON_CLICK(img, norm, pressed) (img)->tint = Vector4 norm;\
    (img)->onTouchDown = []() { UIImage::get_held()->tint = Vector4 pressed; }; \
    (img)->onTouchUp = []() { UIImage::get_held()->tint = Vector4 norm; }

struct UIElement
{
    void *data;
    enum {IMAGE,TEXT,GROUP} type;
    UIElement(UIImage *img) : data(img), type(IMAGE){}
    UIElement(UIText *txt) : data(txt), type(TEXT){}
    UIElement(UIGroup *g) : data(g), type(GROUP){}
    void destroy();
};
struct UIContainer
{
    UIGroup *group;
    std::vector<UIElement> ui;
    void add(UIElement element);
    UIContainer(UIGroup *g) : group(g){}
    ~UIContainer();
};