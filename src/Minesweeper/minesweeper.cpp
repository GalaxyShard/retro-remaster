#include <Galaxy/engine.hpp>
#include <utils.hpp>
namespace {

// Easy: 8x8, 9 bombs
// Medium: 16x16, 40 bombs
// Hard: 30x16, 99 bombs

// Game settings
//int mapWidth = 8, mapHeight = 8;
//int bombs = 9;

//int mapWidth = 16, mapHeight = 16;
int mapWidth = 16, mapHeight = 16;
int bombs = 40;

//int mapWidth = 30, mapHeight = 16;
//int bombs = 99;

int tiles = mapWidth * mapHeight;

struct Tile
{
    bool isBomb = 0;
    bool isFlagged = 0;
    bool isRevealed = 0;
    int bombsTouching = -1;

    int x = 0;
    int y = 0;

    UIImage *img = 0;
    UIText *text = 0;
};
struct Game
{
    UIImage *endImage = nullptr;
    UIText *endText = nullptr;

    //int mapWidth = 16, mapHeight = 16;
    //int tiles = mapWidth * mapHeight;
    //int bombs = 40;
    //Listener preRenderConn;

    float timerBegin = 0;
    float timerOffset = 0;
    bool gameFinished = 0;
    bool placeFlag = 0;
};
Game *game = nullptr;
int preRenderID = -1;
#if OS_MOBILE || OS_WEB
UIImage *flagButton;
#endif
UIText *timerText, *flagsText;

int flagsPlaced = 0;
int revealedTiles = 0;
AssetRef<Texture> tileTex, flagTex, bombTex, restartTex, continueTex, menuTex;
Scene *scene;
Tile *map = nullptr;

std::unordered_map<UIImage*, Tile*> imgToTile;

void pre_render();
void toggle_flag(Tile *tile);
Tile *get_tile(int x, int y) { return map + (x + y*mapWidth); }

std::string filter_time(int t)
{ return t < 10 ? "0"+std::to_string(t) : std::to_string(t); }
std::string hms(int t)
{
    int seconds = t % 60;
    int minutes = t / 60;
    return filter_time(minutes)+":"+filter_time(seconds);
}
void start_timer()
{
    if (preRenderID == -1)
    {
        preRenderID = Renderer::pre_render().connect_int(&pre_render);
        game->timerBegin = Time::get();
    }
}
void stop_timer()
{
    if (preRenderID != -1)
    {
        Renderer::pre_render().disconnect_int(preRenderID);
        preRenderID = -1;
        game->timerOffset += Time::get() - game->timerBegin;
        game->timerBegin = 0;
    }
}

void destroy_map()
{
    imgToTile.clear();
    if (map) delete[] map;
}
void on_click();
void continue_game()
{
    if (game->endImage)
    {
        if (!game->gameFinished) start_timer();
        for (int i = 0; i < tiles; ++i)
            map[i].img->onClick = &on_click;
        
        delete game->endImage;
        delete game->endText;
        game->endImage = nullptr;
        game->endText = nullptr;
    }
}
void restart_game()
{
    stop_timer();
    delete game;
    game = nullptr;
    destroy_map();
    delete scene;
    new Scene("Minesweeper");

}
void end_game(bool won)
{
    if (won)
    {
        game->gameFinished = 1;
        for (int i = 0; i < tiles; ++i)
        {
            Tile *t = map+i;
            if (t->isBomb && !t->isFlagged) toggle_flag(t);
        }
    }
    stop_timer();
    for (int i = 0; i < tiles; ++i) map[i].img->onClick = nullptr;
    
    UIImage *endImage = new UIImage();
    endImage->scale = Vector2(1, 0.3f);
    endImage->anchor = Vector2(0,0);
    endImage->tint = Vector4(.5,.5,.5,.75);

    UIText *endText = text_for_img(won ? "You Won!" : "Game over", endImage);
    endText->scale = endImage->scale * 0.95f;

    game->endImage = endImage;
    game->endText = endText;
}
bool tile_exists(int x, int y)
{ return Math::within(x, 0, mapWidth-1) && Math::within(y, 0, mapHeight-1); }
bool is_bomb(int x, int y)
{
    if (!tile_exists(x, y)) return 0;
    return get_tile(x, y)->isBomb;
}
bool is_flagged(int x, int y)
{
    if (!tile_exists(x, y)) return 0;
    return get_tile(x, y)->isFlagged;
}

int adj_flags(Tile *tile)
{
    int x = tile->x;
    int y = tile->y;
    int touching = 0;
    touching += is_flagged(x-1, y-1);
    touching += is_flagged(x,   y-1);
    touching += is_flagged(x+1, y-1);

    touching += is_flagged(x-1, y+1);
    touching += is_flagged(x,   y+1);
    touching += is_flagged(x+1, y+1);

    touching += is_flagged(x-1, y);
    touching += is_flagged(x+1, y);
    return touching;
}
void check_tile(int x, int y);
void check_tile(Tile *tile)
{
    if (tile->isFlagged || tile->isRevealed) return;

    start_timer();
    
    tile->isRevealed = 1;
    if (tile->isBomb)
    {
        tile->img->texture = bombTex.get();
        logmsg("no text\n");
        end_game(0);
        return;
    }
    else
    {
        ++revealedTiles;
        if (revealedTiles == tiles - bombs)
            end_game(1);
        
        int x = tile->x;
        int y = tile->y;
        int touching = 0;
        touching += is_bomb(x-1, y-1);
        touching += is_bomb(x,   y-1);
        touching += is_bomb(x+1, y-1);

        touching += is_bomb(x-1, y+1);
        touching += is_bomb(x,   y+1);
        touching += is_bomb(x+1, y+1);

        touching += is_bomb(x-1, y);
        touching += is_bomb(x+1, y);

        if (touching == 0)
        {
            check_tile(x-1, y-1);
            check_tile(x,   y-1);
            check_tile(x+1, y-1);

            check_tile(x-1, y+1);
            check_tile(x,   y+1);
            check_tile(x+1, y+1);

            check_tile(x-1, y);
            check_tile(x+1, y);
        }
        tile->bombsTouching = touching;
        auto *txt = text_for_img(std::to_string(touching), tile->img);
        txt->scale = tile->img->scale*1.25f;
        //auto *txt2 = text_for_img(std::to_string(touching), tile->img);
        //txt2->scale = tile->img->scale*1.25f;
        auto *img = img_for_text(txt);
        img->tint = Vector4((float)rand()/RAND_MAX, (float)rand()/RAND_MAX, (float)rand()/RAND_MAX, 1);
        tile->text = txt;
        logmsg("text: %o\n", txt->text);
    }
}
void check_tile(int x, int y)
{
    if (tile_exists(x, y)) check_tile(get_tile(x, y));
}
void toggle_flag(Tile *tile)
{
    if (tile->isRevealed) return;

//#if OS_MOBILE || OS_WEB
    //flagButton->tint = Vector4(1,1,1,1);
//#endif
    //game->placeFlag = 0;

    if (tile->isFlagged)
    {
        --flagsPlaced;
        tile->isFlagged = 0;
        tile->img->texture = tileTex.get();
    }
    else
    {
        ++flagsPlaced;
        tile->isFlagged = 1;
        tile->img->texture = flagTex.get();
    }
    flagsText->text = std::to_string(flagsPlaced);
    flagsText->refresh();
}
void on_click()
{
    UIImage *img = UIImage::get_held();
    Tile *tile = imgToTile[img];
    if (Input::is_held("place_flag") || game->placeFlag)
    {
        toggle_flag(tile);
    }
    else
    {
        if (tile->isRevealed)
        {
            int flags = adj_flags(tile);
            if (tile->bombsTouching == flags)
            {
                int x = tile->x;
                int y = tile->y;
                check_tile(x-1, y-1);
                check_tile(x,   y-1);
                check_tile(x+1, y-1);

                check_tile(x-1, y+1);
                check_tile(x,   y+1);
                check_tile(x+1, y+1);

                check_tile(x-1, y);
                check_tile(x+1, y);
            }
        }
        else
            check_tile(tile);
        //if (tile->text)
        //    logmsg("text: %o\n", tile->text->text);
        //else logmsg("No text\n");
    }
}
void pre_render()
{
    timerText->text = hms(Time::get() - game->timerBegin + game->timerOffset);
    timerText->refresh();
}
void create_scene()
{
    //game = std::make_unique<Game>();
    game = new Game();

    revealedTiles = 0;
    flagsPlaced = 0;
    scene = Scene::activeScene;

    timerText = new UIText("00:00");
    timerText->anchor = Vector2(1, 1);
    timerText->scale = Vector2(0.3, 0.3);
    timerText->pivot = Vector2(0, 0);
    timerText->pos = Vector2(-0.2f, -0.1f);

    flagsText = new UIText("0");
    flagsText->anchor = Vector2(-1, 1);
    flagsText->scale = Vector2(0.1, 0.1);
    flagsText->pivot = Vector2(0, 0);
    flagsText->pos = Vector2(0.2f, -0.1f);

    //map = new Tile*[tiles];
    map = new Tile[tiles];
    for (int i = 0; i < tiles; ++i)
    {
        map[i].x = i % mapWidth;
        map[i].y = i / mapWidth;
        //map[i] = new Tile();
        //map[i]->x = i % mapWidth;
        //map[i]->y = i / mapWidth;
    }
    
    for (int i = 0; i < bombs; ++i)
    {
        int bomb;
        do bomb = rand() % tiles;
        while (map[bomb].isBomb);
        //while (map[bomb]->isBomb);

        map[bomb].isBomb = 1;
        //map[bomb]->isBomb = 1;
    }
#if OS_MOBILE || OS_WEB
    flagButton = new UIImage(flagTex.get());
    //flagButton->onClick = []()
    //{
    //    game->placeFlag = !game->placeFlag;
    //    flagButton->tint = game->placeFlag ? Vector4(0.5, 0.5, 0.5, 1) : Vector4(1,1,1,1);
    //};
    flagButton->onTouchDown = []()
    {
        game->placeFlag = 1;
        flagButton->tint = Vector4(0.5, 0.5, 0.5, 1);
    };
    flagButton->onTouchUp = []()
    {
        game->placeFlag = 0;
        flagButton->tint = Vector4(1,1,1,1);
    };
    flagButton->scale = Vector2(0.4f, 0.4f);
    flagButton->pos += flagButton->scale / 2;
    flagButton->pos.x = -flagButton->pos.x;
    flagButton->anchor = Vector2(1, -1);
#endif
    UIImage *restartBtn = new UIImage(restartTex.get());
    restartBtn->anchor = Vector2(-1, 1);
    restartBtn->scale = Vector2(0.15, 0.15);
    restartBtn->pos = Vector2(0.5f, -restartBtn->scale.y/2-0.02);

    restartBtn->onClick = &restart_game;


    UIImage *continueBtn = new UIImage(continueTex.get());
    continueBtn->anchor = Vector2(1, 1);
    continueBtn->scale = Vector2(0.15, 0.15);

    continueBtn->pos = Vector2(-0.5f, -continueBtn->scale.y/2-0.02);
    continueBtn->onClick = &continue_game;


    UIImage *menuButton = new UIImage(menuTex.get());
    menuButton->anchor = Vector2(1, 1);
    menuButton->scale = Vector2(0.15, 0.15);

    menuButton->pos = Vector2(-0.7f, -menuButton->scale.y/2-0.02);
    menuButton->onClick = []()
    {
        stop_timer();
        delete game;
        game = nullptr;
        delete scene;
        new Scene("Start");
    };

#if OS_MOBILE
    Vector2 min = Vector2(-1,-1), max = Vector2(1,1);
#else
    Vector2 min = Vector2(-0.9,-1), max = Vector2(0.9,0.8);
#endif

    Vector2 mapSize = Vector2(mapWidth, mapHeight);
    Vector2 imgSize = (max-min) / mapSize;
    {
        float minSize = Math::min(imgSize.x, imgSize.y);
        imgSize = Vector2(minSize, minSize);
    }
    for (int y = 0; y < mapHeight; ++y)
    {
        for (int x = 0; x < mapWidth; ++x)
        {
            UIImage *img = new UIImage(tileTex.get());
            //map[x + y*mapWidth]->img = img;
            map[x + y*mapWidth].img = img;
            img->group = UIGroup::aspectRatio;

            img->scale = imgSize;
            img->pos = imgSize*Vector2(x, y) + img->scale/2 + (Vector2(1,1)+min);

            //imgToTile[img] = map[x + y*mapWidth];
            imgToTile[img] = map + (x + y*mapWidth);
            img->onClick = &on_click;
        }
    }
}

void first_init()
{
    tileTex = Texture::load(Assets::path()+"/textures/minetile.png", Texture::Pixel);
    flagTex = Texture::load(Assets::path()+"/textures/mineflag.png", Texture::Pixel);
    bombTex = Texture::load(Assets::path()+"/textures/minebomb.png", Texture::Pixel);
    restartTex = Texture::load(Assets::path()+"/textures/restart.png", Texture::Pixel);
    continueTex = Texture::load(Assets::path()+"/textures/continue.png", Texture::Pixel);
    
    menuTex = Texture::load(Assets::path()+"/textures/menuButton.png", Texture::Pixel);

    Input::add_bind("place_flag", Key::LeftShift);
    Scene::set_create_callback("Minesweeper", &create_scene);
}
FIRST_INIT_FUNC(first_init);
//void clean()
//{
//    game = nullptr;
//}
//CLEANUP_FUNC(clean);
}