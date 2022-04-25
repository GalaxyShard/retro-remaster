#include "utils.hpp"

void UIElement::destroy()
{
    if (type == IMAGE) UIImage::destroy((UIImage*)data);
    else if (type == TEXT) UIText::destroy((UIText*)data);
    else if (type == GROUP) UIGroup::destroy((UIGroup*)data);
    else assert(false);
}
void UIContainer::add(UIElement element)
{
    ui.push_back(element);
}
UIContainer::~UIContainer()
{
    for (UIElement &e : ui)
        e.destroy();
    UIGroup::destroy(group);
}
UIText* text_for_img(std::string str, UIImage *img)
{
    UIText *text = UIText::create(str);
    text->group = img->group;
    text->scale = img->scale;
    text->pos = img->pos;
    text->pivot = Vector2(0,0);
    text->anchor = img->anchor;
    text->render_order(img->render_order());
    return text;
}
UIImage* img_for_text(UIText *txt)
{
    UIImage *img = UIImage::create();
    img->group = txt->group;
    img->scale = txt->scale;
    img->pos = txt->pos;
    img->anchor = txt->anchor;
    img->render_order(txt->render_order());
    return img;
}