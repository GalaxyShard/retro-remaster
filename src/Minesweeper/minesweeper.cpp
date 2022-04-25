#include <Galaxy/engine.hpp>
#include <utils.hpp>
#include <dlfcn.h>
enum TileFlags : ucharG { ISBOMB = 1<<0, ISREVEALED = 1<<1, ISFLAGGED = 1<<2 };
struct Tile
{
    ucharG flags = 0;
    ucharG bombCount = 0;
    UIImage *img = 0;
};
struct Minesweeper
{
    unsigned int mapWidth = 16, mapHeight = 16;
    unsigned int bombs = 40;
    unsigned int tiles = mapWidth * mapHeight;
    bool gameStarted = 0, gameEnded = 0, gameWon = 0;
    
    double startTime;
    double elapsed = 0;
    unsigned int flagCount = 0;
    uintg revealCount = 0;
    Tile *map = 0;

    UIGroup *group;

    Listener renderConn, aspectConn;
    UIText *timer, *flagsText;

    UIText *endText = 0;
    UIImage *endBg = 0;
#if OS_MOBILE || OS_WEB
    UIImage *flagButton;
#endif
    std::map<UIImage*, unsigned int> imgToTile;
    AssetRef<Texture> tileTex, flagTex, bombTex, restartTex, continueTex, menuTex;

    Minesweeper();
    ~Minesweeper();

    void end_game(bool won);
    void continue_game();
    void start_game();
    void clear_board();
    Tile* find(int x, int y);
    void reveal_tile(int x, int y);
    void on_click();
    void pre_render();
    void fix_board();
};
std::string filter_time(int t)
{ return t < 10 ? "0"+std::to_string(t) : std::to_string(t); }
std::string hms(int t)
{
    int seconds = t % 60;
    int minutes = t / 60;
    return filter_time(minutes)+":"+filter_time(seconds);
}
void Minesweeper::fix_board()
{
    Vector2 min,max;
    Vector2 ratio = Renderer::aspectRatio;
    if (ratio.y > 1.25f) min = Vector2(-1,-1), max = Vector2(1,1);
    else min = Vector2(-0.9,-1), max = Vector2(0.9,0.8);

    group->pos = (min+max)/2;
    group->scale = (max-min)/2;
    group->cache();
}
void Minesweeper::clear_board()
{
    Scene::destroy(Scene::activeScene);
    Scene::create("Minesweeper");
}
void Minesweeper::pre_render()
{
    timer->str = hms(Time::get()-startTime+elapsed);
    timer->refresh();
}
void Minesweeper::end_game(bool won)
{
    if (gameEnded) return;
    gameEnded = 1;
    gameWon = won;
    elapsed = Time::get()-startTime + elapsed;
    renderConn.disconnect();

    if (won)
    {
        for (uintg i = 0; i < tiles; ++i)
        {
            //if ((map[i].flags & ISBOMB) && !(map[i].flags & ISFLAGGED))
            if (map[i].flags == ISBOMB)
            {
                map[i].flags |= ISFLAGGED;
                map[i].img->texture = flagTex.get();
                ++flagCount;
            }
        }
        flagsText->str = std::to_string(flagCount);
        flagsText->refresh();
    }

    endBg = UIImage::create();
    constexpr float grey = 0.2f;
    endBg->tint = Vector4(grey,grey,grey,0.5f);
    endBg->render_order(1);
    endBg->scale = Vector2(1, 0.25f);
    endBg->anchor = Vector2(0,0);
    endText = text_for_img(won ? "You Win!" : "Game Over", endBg);
}
Tile* Minesweeper::find(int x, int y)
{
    if (!Math::within<int>(x, 0, mapWidth-1) || !Math::within<int>(y, 0, mapHeight-1))
        return 0;
    return &map[x+y*mapWidth];
}
void Minesweeper::continue_game()
{
    if (!gameEnded) return;
    gameEnded = 0;
    UIImage::destroy(endBg), endBg = 0;
    UIText::destroy(endText), endText = 0;

    if (gameWon) return;
    startTime = Time::get();
    renderConn = Renderer::pre_render().connect(CLASS_LAMBDA(pre_render));
}
void Minesweeper::start_game()
{
    gameStarted = 1;
    startTime = Time::get();
    renderConn = Renderer::pre_render().connect(CLASS_LAMBDA(pre_render));
    for (unsigned int i = 0; i < bombs; ++i)
    {
        int bomb;
        do bomb = rand() % tiles;
        while (map[bomb].flags & ISBOMB);
        map[bomb].flags |= ISBOMB;
    }
}
void Minesweeper::reveal_tile(int x, int y)
{
    Tile *tile = find(x,y);
    if (!tile || (tile->flags & (ISREVEALED | ISFLAGGED)))
        return;

    tile->flags |= ISREVEALED;
    if (tile->flags & ISBOMB)
    {
        tile->img->texture = bombTex.get();
        end_game(0);
        return;
    }
    ++revealCount;
    if (revealCount == tiles-bombs)
        end_game(1);
    
    ucharG bombCount = 0;

    auto isBomb = [&](int xoff, int yoff)->bool
    {
        Tile *newTile = find(x+xoff, y+yoff);
        return newTile && newTile->flags & ISBOMB;
    };
    bombCount += isBomb(-1, -1);
    bombCount += isBomb(-1,  0);
    bombCount += isBomb(-1,  1);
    bombCount += isBomb( 1, -1);
    bombCount += isBomb( 1,  0);
    bombCount += isBomb( 1,  1);
    bombCount += isBomb( 0,  1);
    bombCount += isBomb( 0, -1);

    if (!bombCount)
    {
        reveal_tile(x-1, y-1);
        reveal_tile(x-1, y  );
        reveal_tile(x-1, y+1);
        reveal_tile(x+1, y-1);
        reveal_tile(x+1, y  );
        reveal_tile(x+1, y+1);
        reveal_tile(x,   y-1);
        reveal_tile(x,   y+1);
    }
    tile->bombCount = bombCount;
    UIText *text = text_for_img(std::to_string(bombCount), tile->img);
    text->scale *= 1.25f;
}
void Minesweeper::on_click()
{
    if (gameEnded)
        return;
    UIImage *img = UIImage::get_held();
    unsigned int index = imgToTile[img];
    Tile *tile = map+index;
    if (!gameStarted)
    {
        start_game();
        if (tile->flags & ISBOMB)
        {
            int bomb;
            do bomb = rand() % tiles;
            while (map[bomb].flags & ISBOMB);
            map[bomb].flags |= ISBOMB;
            tile->flags &= ~ISBOMB;
        }
    }
    uintg x = index % mapWidth;
    uintg y = index / mapWidth;
    if (tile->flags & ISREVEALED)
    {
        if ((tile->flags & ISBOMB) || !tile->bombCount)
            return;
        auto isFlagged = [&](int xoff, int yoff)->bool
        {
            Tile *newTile = find(x+xoff, y+yoff);
            return newTile && newTile->flags & ISFLAGGED;
        };
        uintg flagCount = 0;
        flagCount += isFlagged(-1, -1);
        flagCount += isFlagged(-1,  0);
        flagCount += isFlagged(-1,  1);
        flagCount += isFlagged( 1, -1);
        flagCount += isFlagged( 1,  0);
        flagCount += isFlagged( 1,  1);
        flagCount += isFlagged( 0,  1);
        flagCount += isFlagged( 0, -1);
        if (flagCount == tile->bombCount)
        {
            reveal_tile(x-1, y-1);
            reveal_tile(x-1, y  );
            reveal_tile(x-1, y+1);
            reveal_tile(x+1, y-1);
            reveal_tile(x+1, y  );
            reveal_tile(x+1, y+1);
            reveal_tile(x,   y-1);
            reveal_tile(x,   y+1);
        }
        return;
    }

    if (Input::is_held("place_flag"))
    {
        tile->flags ^= ISFLAGGED;
        if (tile->flags & ISFLAGGED)
        {
            img->texture = flagTex.get();
            ++flagCount;
        }
        else
        {
            img->texture = tileTex.get();
            --flagCount;
        }
        flagsText->str = std::to_string(flagCount);
        flagsText->refresh();
    }
    else
    {
        reveal_tile(x,y);
    }
}
Minesweeper::Minesweeper()
{
    tileTex = Texture::load(Assets::path()+"/textures/minetile.png", Texture::Pixel);
    flagTex = Texture::load(Assets::path()+"/textures/mineflag.png", Texture::Pixel);
    bombTex = Texture::load(Assets::path()+"/textures/minebomb.png", Texture::Pixel);
    restartTex = Texture::load(Assets::path()+"/textures/restart.png", Texture::Pixel);
    continueTex = Texture::load(Assets::path()+"/textures/continue.png", Texture::Pixel);
    menuTex = Texture::load(Assets::path()+"/textures/menuButton.png", Texture::Pixel);

    UIImage *nav = UIImage::create();
    nav->group = UIGroup::safeArea;
    nav->tint = Vector4(0.2, 0.2, 0.2, 1);
    nav->anchor = Vector2(0, 1);
    nav->scale = Vector2(20, 0.18f);
    nav->pos = Vector2(0, -nav->scale.y*0.5f);

    timer = UIText::create("00:00");
    timer->group = UIGroup::safeArea;
    timer->anchor = Vector2(1, 1);
    timer->scale = Vector2(0.3, 0.3);
    timer->pivot = Vector2(0, 0);
    timer->pos = Vector2(-0.2f, -0.1f);

    flagsText = UIText::create("0");
    flagsText->group = UIGroup::safeArea;
    flagsText->anchor = Vector2(-1, 1);
    flagsText->scale = Vector2(0.1, 0.1);
    flagsText->pivot = Vector2(0, 0);
    flagsText->pos = Vector2(0.2f, -0.1f);


#if OS_MOBILE || OS_WEB
    flagButton = UIImage::create(flagTex.get());
    flagButton->group = UIGroup::safeArea;
    flagButton->scale = Vector2(0.4f, 0.4f);
    flagButton->pos += flagButton->scale / 2;
    flagButton->pos.x = -flagButton->pos.x;
    flagButton->anchor = Vector2(1, -1);
    if (globalScene->get_component<MinesweeperOptions>()->flagModeToggle)
    {
        flagButton->onClick = [](){ Input::trigger("place_flag", !Input::is_held("place_flag")); };
    }
    else
    {
        flagButton->onTouchDown = [](){ Input::trigger("place_flag", 1); };
        flagButton->onTouchUp = [](){ Input::trigger("place_flag", 0); };
    }
#endif
    UIImage *restartBtn = UIImage::create(restartTex.get());
    restartBtn->group = UIGroup::safeArea;
    restartBtn->anchor = Vector2(-1, 1);
    restartBtn->scale = Vector2(0.15, 0.15);
    restartBtn->pos = Vector2(0.5f, -restartBtn->scale.y/2-0.02);
    restartBtn->onClick = CLASS_LAMBDA(clear_board);

    UIImage *continueBtn = UIImage::create(continueTex.get());
    continueBtn->group = UIGroup::safeArea;
    continueBtn->anchor = Vector2(1, 1);
    continueBtn->scale = Vector2(0.15, 0.15);
    continueBtn->pos = Vector2(-0.5f, -continueBtn->scale.y/2-0.02);
    continueBtn->onClick = CLASS_LAMBDA(continue_game);

    UIImage *menuBtn = UIImage::create(menuTex.get());
    menuBtn->group = UIGroup::safeArea;
	TINT_ON_CLICK(menuBtn, (1,1,1,1), (0.75,0.75,0.75,1));
    menuBtn->anchor = Vector2(1, 1);
    menuBtn->scale = Vector2(0.15, 0.15);
    menuBtn->pos = Vector2(-0.7f, -menuBtn->scale.y/2-0.02);
    menuBtn->onClick = []()
    {
        Scene::destroy(Scene::activeScene);
        Scene::create("Start");
    };


    map = new Tile[tiles];
    
    group = UIGroup::create();
    group->parent(UIGroup::aspectRatio);
    group->anchor = Vector2(0,0);
    fix_board();
    aspectConn = Renderer::aspect_ratio_changed().connect(CLASS_LAMBDA(fix_board));

    Vector2 imgSize = Vector2(std::min(2.f/mapWidth, 2.f/mapHeight),0);
    imgSize.y = imgSize.x;

    for (unsigned int i = 0; i < tiles; ++i)
    {
        Vector2Int tile = Vector2Int(i % mapWidth, i / mapWidth);
        UIImage *img = UIImage::create(tileTex.get());
        map[i].img = img;
        img->group = group;

        img->scale = imgSize;
        img->pos = imgSize*tile + img->scale*0.5f;

        imgToTile[img] = i;
        img->onClick = CLASS_LAMBDA(on_click);
    }
#if OS_MOBILE || OS_WEB
    Input::add_bind("place_flag", Key::LeftShift, [this](bool p)
    { flagButton->tint = p ? Vector4(0.5, 0.5, 0.5, 1) : Vector4(1,1,1,1); });
#else
    Input::add_bind("place_flag", Key::LeftShift);
#endif
}
Minesweeper::~Minesweeper()
{
    Input::trigger("place_flag", 0);
    Input::remove_bind("place_flag");
    delete[] map;
}
ADD_SCENE_COMPONENT("Minesweeper", Minesweeper);