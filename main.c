#define RETRO_WINDOW_CAPTION "Dragons"
#define RETRO_ARENA_SIZE Kilobytes(32)
#define RETRO_WINDOW_DEFAULT_WIDTH 1280
#define RETRO_WINDOW_DEFAULT_HEIGHT 560
#define RETRO_CANVAS_DEFAULT_WIDTH (RETRO_WINDOW_DEFAULT_WIDTH / 2)
#define RETRO_CANVAS_DEFAULT_HEIGHT (RETRO_WINDOW_DEFAULT_HEIGHT / 2)

#define TILE_SIZE 16
#define SECTION_WIDTH   (RETRO_CANVAS_DEFAULT_WIDTH / TILE_SIZE)
#define SECTION_HEIGHT  (RETRO_CANVAS_DEFAULT_HEIGHT / TILE_SIZE)

#define LEVEL_WIDTH (SECTION_WIDTH * 2)
#define LEVEL_HEIGHT (SECTION_HEIGHT)
#define MAX_OBJECTS_PER_SECTION 16
#define SECTIONS_PER_LEVEL (4)

#define MAX_MONSTERS 256


#define VERSION "1.0"

#include "retro.c"

static Font           FONT_NEOSANS;
static Bitmap         SPRITESHEET;
static Bitmap         SPRITESHEET_HUMANOID[2];
static Bitmap         SPRITESHEET_REPTILE[2];
static Bitmap         CATSHEET;
static Bitmap         PLAYERSHEET;
static Bitmap         COCO;
static Animation      ANIMATEDSPRITE_PLAYER_IDLE;
static Animation      ANIMATEDSPRITE_PLAYER_WALK;
static Animation      ANIMATEDSPRITE_CAT1_IDLE;
static Animation      ANIMATEDSPRITE_CAT1_WALK;
static Animation      ANIMATEDSPRITE_CAT2_IDLE;
static Animation      ANIMATEDSPRITE_CAT2_WALK;
static Animation      ANIMATEDSPRITE_CAT3_IDLE;
static Animation      ANIMATEDSPRITE_CAT3_WALK;
static Animation      ANIMATEDSPRITE_CAT_SHADOW_IDLE;
static Animation      ANIMATEDSPRITE_CAT_SHADOW_WALK;
static Sound          SOUND_JUMP1;
static Sound          SOUND_JUMP2;
static Sound          SOUND_HURT1;
static Sound          SOUND_HURT2;
static Sound          SOUND_PICKUP1;
static Sound          SOUND_PICKUP2;
static Sound          SOUND_SELECT;
static Bitmap         TILES1;


typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

static u32 FrameCount     = 0;
static u32 AnimationTimer = 0;

typedef enum 
{
  AC_QUIT,
  AC_UP,
  AC_DOWN,
  AC_LEFT,
  AC_RIGHT,
  AC_WAIT
} Actions;

u32 randr(u32 min, u32 max)
{
  double scaled = (double)rand()/RAND_MAX;

  return (max - min +1)*scaled + min;
}

void Init(Settings* settings)
{
  Palette_Make(&settings->palette);
  Palette_LoadFromBitmap("palette.png", &settings->palette);

  Input_BindKey(SDL_SCANCODE_ESCAPE, AC_QUIT);
  Input_BindKey(SDL_SCANCODE_W, AC_UP);
  Input_BindKey(SDL_SCANCODE_S, AC_DOWN);
  Input_BindKey(SDL_SCANCODE_A, AC_LEFT);
  Input_BindKey(SDL_SCANCODE_D, AC_RIGHT);
  Input_BindKey(SDL_SCANCODE_R, AC_WAIT);

  Font_Load("NeoSans.png", &FONT_NEOSANS, Colour_Make(0,0,255), Colour_Make(255,0,255));
  Bitmap_Load("cave.png", &SPRITESHEET, 0);
  Bitmap_Load("cats.png", &CATSHEET, 16);
  Bitmap_Load("player.png", &PLAYERSHEET, 16);
  Bitmap_Load("coco.png", &COCO, 255);
  Bitmap_Load("Humanoid0.png", &SPRITESHEET_HUMANOID[0], 0);
  Bitmap_Load("Humanoid1.png", &SPRITESHEET_HUMANOID[1], 0);
  Bitmap_Load("Reptile0.png",  &SPRITESHEET_REPTILE[0], 0);
  Bitmap_Load("Reptile1.png",  &SPRITESHEET_REPTILE[1], 0);

  Animation_LoadHorizontal(&ANIMATEDSPRITE_PLAYER_IDLE, &PLAYERSHEET, 1, 100, 0, 0, 46, 50);
  Animation_LoadHorizontal(&ANIMATEDSPRITE_PLAYER_WALK, &PLAYERSHEET, 8, 120, 0, 150, 46, 50);

  Animation_LoadHorizontal(&ANIMATEDSPRITE_CAT1_IDLE, &CATSHEET, 1, 100, 0, 0, 18, 18);
  Animation_LoadHorizontal(&ANIMATEDSPRITE_CAT1_WALK, &CATSHEET, 3, 100, 0, 0, 18, 18);
  Animation_LoadHorizontal(&ANIMATEDSPRITE_CAT2_IDLE, &CATSHEET, 1, 100, 0, 18, 18, 18);
  Animation_LoadHorizontal(&ANIMATEDSPRITE_CAT2_WALK, &CATSHEET, 3, 100, 0, 18, 18, 18);
  Animation_LoadHorizontal(&ANIMATEDSPRITE_CAT3_IDLE, &CATSHEET, 1, 100, 0, 36, 18, 18);
  Animation_LoadHorizontal(&ANIMATEDSPRITE_CAT3_WALK, &CATSHEET, 3, 100, 0, 36, 18, 18);
  Animation_LoadHorizontal(&ANIMATEDSPRITE_CAT_SHADOW_IDLE, &CATSHEET, 1, 100, 0, 234, 18, 18);
  Animation_LoadHorizontal(&ANIMATEDSPRITE_CAT_SHADOW_WALK, &CATSHEET, 3, 100, 0, 234, 18, 18);

  Sound_Load(&SOUND_HURT1,   "hurt1.wav");
  Sound_Load(&SOUND_HURT2,   "hurt2.wav");
  Sound_Load(&SOUND_JUMP1,   "jump1.wav");
  Sound_Load(&SOUND_JUMP2,   "jump2.wav");
  Sound_Load(&SOUND_PICKUP1, "pickup1.wav");
  Sound_Load(&SOUND_PICKUP2, "pickup2.wav");
  Sound_Load(&SOUND_SELECT,  "select.wav");

  Bitmap_Load("tiles1.png", &TILES1, 16);

}

void Tile_Draw(Bitmap* bitmap, S32 x, S32 y, U32 ox, U32 oy)
{
  SDL_Rect src, dst;
  
  src.x = ox * TILE_SIZE;
  src.y = oy * TILE_SIZE;
  
  src.w = TILE_SIZE;
  src.h = TILE_SIZE;

  dst.x = x;
  dst.y = y;
  dst.w = TILE_SIZE;
  dst.h = TILE_SIZE;

  Canvas_Splat3(bitmap, &dst, &src);
}

typedef struct
{
  s32 x, y;
} XY;

typedef struct
{
  s32 x, y, dX, dY;
  u8  type;
  u8  health;
  u8  moveTime;
} Object;

typedef struct
{
  Object player;
  Object objects[MAX_MONSTERS];
  u32    nbObjects;
} Game;

Game*      GAME;
bool       STEP_MODE;

enum 
{
  OT_None,
  OT_Human,
  OT_FireDragon,
  OT_WaterDragon,
  OT_EarthDragon,
  OT_AirDragon,
};

Bitmap* Object_GetBitmap(u8 type)
{
  switch(type)
  {
    case OT_Human:       return &SPRITESHEET_HUMANOID[AnimationTimer % 2];
    case OT_FireDragon:  return &SPRITESHEET_REPTILE[AnimationTimer % 2];
    case OT_WaterDragon: return &SPRITESHEET_REPTILE[AnimationTimer % 2];
    case OT_EarthDragon: return &SPRITESHEET_REPTILE[AnimationTimer % 2];
    case OT_AirDragon:  return &SPRITESHEET_REPTILE[AnimationTimer % 2];
  }
  return 0;
}


void Object_GetSpriteTileIndex(u8 type, XY* xy)
{
  switch(type)
  {
    case OT_Human:       xy->x = 0; xy->y = 0; return;
    case OT_FireDragon:  xy->x = 1; xy->y = 3; return;
    case OT_WaterDragon: xy->x = 4; xy->y = 3; return;
    case OT_EarthDragon: xy->x = 5; xy->y = 2; return;
    case OT_AirDragon:   xy->x = 3; xy->y = 2; return;
  }
}

u32 Object_MoveSpeed(u8 type)
{
  switch(type)
  {
    case OT_Human:       return 1;
    case OT_FireDragon:  return 2;
    case OT_WaterDragon: return 3;
    case OT_EarthDragon: return 3;
    case OT_AirDragon:   return 2;
  }
  return 0;
}

void Object_Draw(Object* object)
{
  Bitmap* bitmap = Object_GetBitmap(object->type);

  Rect r;
  r.left    = object->x * TILE_SIZE;
  r.top     = object->y * TILE_SIZE;
  r.right   = r.left   + TILE_SIZE;
  r.bottom  = r.top + TILE_SIZE;

//  Canvas_DrawRectangle(2, r);
  XY xy;
  Object_GetSpriteTileIndex(object->type, &xy);
  Tile_Draw(bitmap, object->x * TILE_SIZE, object->y * TILE_SIZE,  xy.x, xy.y);
}

void Player_New(u32 x, u32 y)
{
  Object* player = &GAME->player;
  player->type = OT_Human;
  player->health = 1;
  player->moveTime = 1;
  player->x = x;
  player->y = y;
}

void Monster_New(u8 type, u32 x, u32 y)
{
  Object* monster = &GAME->objects[GAME->nbObjects++];
  monster->type = type;
  monster->health = 1;
  monster->moveTime = 1;
  monster->x = x;
  monster->y = y;
}

bool Object_Tick(Object* object)
{
  bool canMove = false;
  bool isPlayer = (object == &GAME->player);

  if (object->moveTime == 0)
  {
    s32 tx = object->x + object->dX;
    s32 ty = object->y + object->dY;
    
    canMove = true;

    if (tx < 0)
      canMove = false;

    if (tx >= SECTION_WIDTH)
      canMove = false;

    if (ty < 0)
      canMove = false;

    if (ty >= SECTION_WIDTH)
      canMove = false;

    if (canMove)
    {
      for(u32 i=0;i < GAME->nbObjects;i++)
      {
        Object* other = &GAME->objects[i];
      
        if (other == object)
          continue;

        bool otherIsPlayer = (other == &GAME->player);
      
        if (other->x == tx && other->y == ty && !otherIsPlayer)
        {
          canMove = false;
        }
      
      }
    }

    if (canMove)
    {
      object->x = tx;
      object->y = ty;
      object->moveTime = Object_MoveSpeed(object->type) - 1;
    }
  }
  else
  {
    object->moveTime--;
  }

  object->dX = 0;
  object->dY = 0;

  return canMove;
}

void RestartLevel()
{
  memset(GAME, 0, sizeof(Game));

  Player_New(SECTION_WIDTH / 2, SECTION_HEIGHT / 2);

  for(int i=0;i < 4;i++)
  {
    Monster_New(OT_FireDragon, randr(0, SECTION_WIDTH - 1), randr(0, SECTION_HEIGHT - 1));
  }
  for(int i=0;i < 4;i++)
  {
    Monster_New(OT_WaterDragon, randr(0, SECTION_WIDTH - 1), randr(0, SECTION_HEIGHT - 1));
  }
  for(int i=0;i < 4;i++)
  {
    Monster_New(OT_EarthDragon, randr(0, SECTION_WIDTH - 1), randr(0, SECTION_HEIGHT - 1));
  }
  for(int i=0;i < 4;i++)
  {
    Monster_New(OT_AirDragon, randr(0, SECTION_WIDTH - 1), randr(0, SECTION_HEIGHT - 1));
  }
}

void Start()
{
  GAME = Scope_New(Game);
  Canvas_SetFlags(0, CNF_Render | CNF_Clear, 0);
  Scope_Push('LEVL');
  RestartLevel();
}

void Step()
{
  bool move = false;
  bool wait = false;

  if (Input_GetActionReleased(AC_UP))
  {
    GAME->player.dY = -1;
    move = true;
  }
  else if (Input_GetActionReleased(AC_DOWN))
  {
    GAME->player.dY = 1;
    move = true;
  }
  
  if (Input_GetActionReleased(AC_LEFT))
  {
    GAME->player.dX = -1;
    move = true;
  }
  else if (Input_GetActionReleased(AC_RIGHT))
  {
    GAME->player.dX = 1;
    move = true;
  }

  if (Input_GetActionReleased(AC_WAIT))
  {
    GAME->player.dX = 0;
    GAME->player.dY = 0;
    move = true;
    wait = true;
  }

  if (move || wait)
  {
    bool didMove = Object_Tick(&GAME->player);
  
    if (didMove || wait)
    {

      for(u32 i=0; i < GAME->nbObjects;i++)
      {
        Object* obj = &GAME->objects[i];
        int dx = (obj->x - GAME->player.x);
        int dy = (obj->y - GAME->player.y);

        if (dx > 0)
          obj->dX = -1;
        else if (dx < 0)
          obj->dX = 1;
        else
          obj->dX = 0;

        if (dy > 0)
          obj->dY = -1;
        else if (dy < 0)
          obj->dY = 1;
        else
          obj->dY = 0;

        Object_Tick(obj);

        if (GAME->player.x == obj->x && GAME->player.y == obj->y)
        {
          RestartLevel();
          return;
        }

      }
    }
  }



  FrameCount++;
  AnimationTimer += (FrameCount % 10 == 0 ? 1 : 0);
  Object_Draw(&GAME->player);
  for(u32 i=0; i < GAME->nbObjects;i++)
  {
    Object_Draw(&GAME->objects[i]);
  }



}
