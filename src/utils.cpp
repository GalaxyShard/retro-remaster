#include <Galaxy/engine.hpp>
#include "utils.hpp"


UIText* text_for_img(std::string str, UIImage *img)
{
    UIText *text = new UIText(str);
    text->group = img->group;
    text->scale = img->scale;
    text->pos = img->pos;
    text->pivot = Vector2(0,0);
    text->anchor = img->anchor;
    text->set_render_order(1);
    return text;
}
UIImage* img_for_text(UIText *txt)
{
    UIImage *img = new UIImage();
    img->group = txt->group;
    img->scale = txt->scale;
    img->pos = txt->pos;
    img->anchor = txt->anchor;
    return img;
}