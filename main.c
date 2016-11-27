
#include "retro.h"
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>


#define TILE_SIZE 16
#define SECTION_WIDTH   (RETRO_CANVAS_DEFAULT_WIDTH / TILE_SIZE)
#define SECTION_HEIGHT  (RETRO_CANVAS_DEFAULT_HEIGHT / TILE_SIZE)

#define LEVEL_WIDTH (SECTION_WIDTH * 2)
#define LEVEL_HEIGHT (SECTION_HEIGHT)
#define SECTION_COUNT 4

#define MAX_MONSTERS 256


#define VERSION "1.0"

enum 
{
  OT_None,
  OT_Human,
  OT_FireDragon,
  OT_WaterDragon,
  OT_EarthDragon,
  OT_AirDragon,
  OT_Egg,
  OT_UsedObject,
  OT_Potion_Slow,
  OT_Potion_Wall,
  OT_Door,
  OT_Key
};


static Font           FONT_NEOSANS;
static Bitmap         SPRITESHEET;
static Bitmap         SPRITESHEET_HUMANOID[2];
static Bitmap         SPRITESHEET_REPTILE[2];
static Bitmap         SPRITESHEET_TOOL;
static Bitmap         SPRITESHEET_WALLS;
static Bitmap         SPRITESHEET_FLOORS;
static Bitmap         SPRITESHEET_POTION;
static Bitmap         SPRITESHEET_DOOR[2];
static Bitmap         SPRITESHEET_KEY;

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
  AC_WAIT,
  AC_USE
} Actions;

u32 randr(u32 min, u32 max)
{
  if (min == max)
    return min;
  double scaled = (double)rand()/RAND_MAX;
  return (u32) ((max - min + 1)*scaled + min);
}

typedef struct
{
  U16   col[SECTION_WIDTH * SECTION_HEIGHT];
  U16   non[SECTION_WIDTH * SECTION_HEIGHT];
  U16   obj[SECTION_WIDTH * SECTION_HEIGHT];
} SectionData;

SectionData* SECTION_DATA;
U32 SECTION_DATA_COUNT;
SectionData* SECTION;
char LOG_TEXT[256];

void logText(const char* format, ...)
{
  va_list argptr;
  va_start(argptr, format);
  vsprintf(LOG_TEXT, format, argptr);
  va_end(argptr);
  printf(LOG_TEXT);
}

char* skipWhitespace(char* s)
{
  while(*s != 0 && isspace(*s))
    s++;
  return s;
}

char* skipToDigit(char* s)
{
  while(*s != 0 && !isdigit(*s))
    s++;
  return s;
}

char* skipString(char* s, const char* str)
{
  s = skipWhitespace(s);
  s += strlen(str);
  s = skipWhitespace(s);
  return s;
}

bool skipToString(char* s, char** t, const char* str)
{
  (*t) = strstr(s, str);
  if ((*t) == NULL)
    return false;
  return true;
}

char* readUInt(char* s, U32* i)
{
  (*i) = 0;

  while(*s != 0 && isdigit(*s))
  {
    (*i) = (*i) * 10  + ((*s) - '0');
    s++;
  }

  return s;
}

void LoadSectionData()
{
  SECTION_DATA = malloc(sizeof(SectionData) * SECTION_COUNT);

  U8 sectionIndex = 0;

  U32 dataSize;
  char* data = TextFile_Load("sections.tmx", &dataSize);

  if (data == NULL)
    printf("Bad text file\n");

  while(*data != 0)
  {
    char* t;
    SectionData* section = &SECTION_DATA[sectionIndex++];
    if (skipToString(data, &t, "\"csv\">"))
    {
      data = skipString(t, "\"csv\">");
      for(U32 i=0;i < (SECTION_WIDTH * SECTION_HEIGHT); i++)
      {
        U32 v = 0;
        data = readUInt(data, &v);
        data = skipToDigit(data);

        if (v >= 0 && v < 820)
        {
          section->col[i] = 0;
          section->non[i] = v - 1;
          section->obj[i] = 0;
        }
        else if (v >= 820 && v < 1840)
        {
          section->col[i] = v - 820;
          section->non[i] = 0;
          section->obj[i] = 0;
        }
        else if (v >= 1840)
        {
          v -= 1840;
          section->col[i] = 0;
          section->non[i] = 0;

          if (v == 0)
          {
            section->obj[i] = OT_Door;
          }
        }
      }
    }
    else
      break;
  }

  SECTION_DATA_COUNT = sectionIndex;
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
  Input_BindKey(SDL_SCANCODE_SPACE, AC_USE);

  Font_Load("NeoSans.png", &FONT_NEOSANS, Colour_Make(0,0,255), Colour_Make(255,0,255));
  Bitmap_Load("Humanoid0.png", &SPRITESHEET_HUMANOID[0], 0);
  Bitmap_Load("Humanoid1.png", &SPRITESHEET_HUMANOID[1], 0);
  Bitmap_Load("Reptile0.png",  &SPRITESHEET_REPTILE[0], 0);
  Bitmap_Load("Reptile1.png",  &SPRITESHEET_REPTILE[1], 0);
  Bitmap_Load("Wall.png", &SPRITESHEET_WALLS, 0);
  Bitmap_Load("Floor.png", &SPRITESHEET_FLOORS, 0);
  Bitmap_Load("Tool.png", &SPRITESHEET_TOOL, 0);
  Bitmap_Load("Potion.png", &SPRITESHEET_POTION, 0);
  Bitmap_Load("Door0.png", &SPRITESHEET_DOOR[0], 0);
  Bitmap_Load("Door1.png", &SPRITESHEET_DOOR[1], 0);
  LoadSectionData();
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

void Section_Draw(int index)
{
  for(u32 x=0;x < SECTION_WIDTH;x++)
  {
    for(u32 y=0;y < SECTION_HEIGHT;y++)
    {
      u32 idx = x + (y * SECTION_WIDTH);

      if (SECTION->col[idx] > 0)
      {
        u32 s = SECTION->col[idx];
        u32 sx = (s % 20);
        u32 sy = (s / 20);
        Tile_Draw(&SPRITESHEET_WALLS, x * TILE_SIZE, y * TILE_SIZE, sx, sy);
      }
      else
      {
        u32 s = SECTION->non[idx];
        u32 sx = (s % 21);
        u32 sy = (s / 21);
        Tile_Draw(&SPRITESHEET_FLOORS, x * TILE_SIZE, y * TILE_SIZE, sx, sy);
      }
    }
  }

}

typedef struct
{
  s32 x, y;
} XY;

static const XY XYDir[8] = {
  {-1, -1},
  {+0, -1},
  {+1, -1},
  {-1, +0},
  {+1, +0},
  {-1, +1},
  {+0, +1},
  {+1, +1},
};

typedef struct
{
  s32 x, y, dX, dY;
  u8  type;
  u8  health;
  u8  moveTime;
  u8  actionTime;
  u8  noMove;
} Object;

typedef struct
{
  Object player;
  Object objects[MAX_MONSTERS];
  u32    nbObjects;
  u32    section;
  u8     potion;
  u8     held;
  bool   keyDropped;
} Game;

Game*      GAME;
bool       STEP_MODE;

bool CanCollide(u32 x, u32 y)
{
  u32 idx = x + (y * SECTION_WIDTH);

  if (SECTION->col[idx] > 0)
  {
    return true;
  }

  return SECTION->non[idx] < 45;
}

bool Object_IsDragon(u8 type)
{
  switch(type)
  {
    case OT_FireDragon:
    case OT_WaterDragon:
    case OT_EarthDragon:
    case OT_AirDragon:      return true;
  }
  return false;
}

Bitmap* Object_GetBitmap(u8 type)
{
  switch(type)
  {
    case OT_Human:          return &SPRITESHEET_HUMANOID[AnimationTimer % 2];
    case OT_FireDragon:     return &SPRITESHEET_REPTILE[AnimationTimer % 2];
    case OT_WaterDragon:    return &SPRITESHEET_REPTILE[AnimationTimer % 2];
    case OT_EarthDragon:    return &SPRITESHEET_REPTILE[AnimationTimer % 2];
    case OT_AirDragon:      return &SPRITESHEET_REPTILE[AnimationTimer % 2];
    case OT_Egg:            return &SPRITESHEET_TOOL;
    case OT_Potion_Slow:    return &SPRITESHEET_POTION;
    case OT_Potion_Wall:    return &SPRITESHEET_POTION;
    case OT_Door:           return &SPRITESHEET_DOOR[AnimationTimer % 2];
  }
  return NULL;
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
    case OT_Egg:         xy->x = 1; xy->y = 0; return;
    case OT_Potion_Slow: xy->x = 0; xy->y = 0; return;
    case OT_Potion_Wall: xy->x = 1; xy->y = 0; return;
    case OT_Door:        xy->x = 0; xy->y = 0; return;
    case OT_Key:         xy->x = 0; xy->y = 0; return;
  }
  xy->x = 0; xy->y = 0;
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
  return UINT32_MAX;
}

u32 Object_MaxActionTime(u8 type)
{
  switch(type)
  {
    case OT_Egg:           return 6;
  }
  return UINT32_MAX / 2;;
}

void Object_Draw(Object* object)
{
  Bitmap* bitmap = Object_GetBitmap(object->type);

  if (bitmap == NULL)
    return;

  Rect r;
  r.left    = object->x * TILE_SIZE;
  r.top     = object->y * TILE_SIZE;
  r.right   = r.left    + TILE_SIZE;
  r.bottom  = r.top     + TILE_SIZE;

  XY xy;
  Object_GetSpriteTileIndex(object->type, &xy);
  Tile_Draw(bitmap, object->x * TILE_SIZE, object->y * TILE_SIZE,  xy.x, xy.y);
  Canvas_PrintF(object->x * TILE_SIZE, object->y * TILE_SIZE, &FONT_NEOSANS, 1, "%i", object->noMove);
}

void Player_New(u32 x, u32 y)
{
  Object* player = &GAME->player;
  player->type = OT_Human;
  player->health = 1;
  player->moveTime = 0;
  player->actionTime = Object_MaxActionTime(OT_Human) + randr(1, 10);
  player->x = x;
  player->y = y;
}

void Object_New(u8 type, u32 x, u32 y)
{
  Object* object = &GAME->objects[GAME->nbObjects++];
  object->type = type;
  object->health = 1;
  object->moveTime = Object_MoveSpeed(type);
  object->actionTime = Object_MaxActionTime(type) + randr(1, 10);
  object->x = x;
  object->y = y;
}

bool Object_CanPassthrough(u8 type)
{

  switch(type)
  {
    case OT_Egg:         
    case OT_UsedObject:  
    case OT_Potion_Slow: 
    case OT_Potion_Wall:
      return true;
  }

  return false;
}

bool TestBounds(s32 tx, s32 ty)
{
  bool canMove = true;
  if (tx < 0)
    canMove = false;

  if (tx >= SECTION_WIDTH)
    canMove = false;

  if (ty < 0)
    canMove = false;

  if (ty >= SECTION_WIDTH)
    canMove = false;

  return canMove;
}

bool TestDiagonal(s32 x, s32 y, s32* dx_, s32* dy_)
{
  u32 dx = *dx_;
  u32 dy = *dy_;

  // Test X - If false, then dX = 0
  if (CanCollide(x, y + dy))
    dx = 0;

  // Test Y - If false;
  if (CanCollide(x + dx, dy))
    dy = 0;

  *dx_ = dx;
  *dy_ = dy;

  return dx != 0 && dy != 0;
}

u32 Object_Count(u8 type)
{
  u32 count = 0;
  for(u32 i=0;i < GAME->nbObjects;i++)
  {
    if(GAME->objects[i].type == type)
      count++;
  }
  return count;
}

u32 Object_CountDragon()
{
  u32 count = 0;
  for(u32 i=0;i < GAME->nbObjects;i++)
  {
    if(GAME->objects[i].type >= OT_AirDragon && GAME->objects[i].type <= OT_FireDragon)
      count++;
  }
  return count;
}

void Object_PickupPotion(Object* object)
{
  GAME->potion = object->type;
  object->type = OT_UsedObject;
}

void Game_KeySpawnChance(u32 x, u32 y)
{
  if (GAME->keyDropped == false)
  {
    // any eggs
    u32 eggCount = Object_Count(OT_Egg);
    u32 dragonCount = Object_CountDragon();

    int chance = eggCount + dragonCount;

    if (randr(1, chance) == 1)
    {
      Object_New(OT_Key, x, y);
      GAME->keyDropped = true;
    }
  }
}

bool Object_Tick(Object* object)
{
  bool canMove = false;
  bool isPlayer = (object == &GAME->player);

  if (object->moveTime == 0)
  {
    s32 tx = object->x + object->dX;
    s32 ty = object->y + object->dY;
    
    canMove = TestBounds(tx, ty);

    if (CanCollide(tx, ty))
    {
      if (TestDiagonal(object->x , object->y, &object->dX, &object->dY) == false)
      {
        canMove = false;
      }
    }

    if (canMove)
    {
      for(u32 i=0;i < GAME->nbObjects;i++)
      {
        Object* other = &GAME->objects[i];
      
        if (other == object)
          continue;

        bool otherIsPlayer = (other == &GAME->player);
        bool otherIsPassThrough    = Object_CanPassthrough(other->type);
      
        if (other->x == tx && other->y == ty)
        {
          if (isPlayer && otherIsPassThrough)
          {
            switch(other->type)
            {
              case OT_Egg:
                other->type = OT_UsedObject;
                Game_KeySpawnChance(tx, ty);
                logText("An Egg was crushed");
              break;
              case OT_Potion_Slow:
                logText("Picked up Splash Potion of Slowness");
                Object_PickupPotion(other);
              break;
              case OT_Potion_Wall:
                logText("Picked up Splash Potion of Wall Making");
                Object_PickupPotion(other);
              break;
            }
            canMove = true;
          }
          else if (!otherIsPlayer)
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
    else
    {
      object->noMove++;
    }
  }
  else
  {
    object->moveTime--;
  }

  if (object->actionTime == 0)
  {
    switch(object->type)
    {
      case OT_Egg:
        object->type = randr(OT_FireDragon, OT_AirDragon);
        object->moveTime = Object_MoveSpeed(object->type) - 1;
        switch(object->type)
        {
          case OT_FireDragon:  logText("A Fire Dragon has hatched!");  break;
          case OT_WaterDragon: logText("A Water Dragon has hatched!"); break;
          case OT_AirDragon:   logText("A Air Dragon has hatched!");   break;
          case OT_EarthDragon: logText("A Earh Dragon has hatched!");  break;
        }
      break;
    }
    object->actionTime = Object_MaxActionTime(object->type) + randr(1, 10);
  }
  else
  {
    object->actionTime--;
  }

  object->dX = 0;
  object->dY = 0;

  return canMove;
}

#define SPAWN_BEGIN(N, M) \
  capacity = randr(N, M); tries = 20;\
  while(true)\
  {\
    tries--; if (tries == 0) break; \
    x = randr(0, SECTION_WIDTH - 1);\
    y = randr(0, SECTION_HEIGHT - 1);\
    if (!CanCollide(x, y))\
    { capacity--; tries = 20; if (capacity == 0) break;

#define SPAWN_END }}

void RestartLevel()
{
  memset(GAME, 0, sizeof(Game));
  u32 sectionId = randr(0,SECTION_DATA_COUNT - 1);
  memcpy(SECTION, &SECTION_DATA[sectionId],sizeof(SectionData));
  
  Player_New(SECTION_WIDTH / 2, SECTION_HEIGHT / 2);

  for(u32 i=0;i < SECTION_WIDTH * SECTION_HEIGHT;i++)
  {
    if (SECTION->obj[i] > 0)
    {
      u32 x = i % SECTION_WIDTH;
      u32 y = i / SECTION_WIDTH;

      Object_New((u8)SECTION->obj[i], x, y);
    }
  }

  s32 x, y, capacity, tries;

  SPAWN_BEGIN(6, 8)
  {
    Object_New(OT_Egg, x, y);
  }
  SPAWN_END;

  SPAWN_BEGIN(4, 12)
  {
    Object_New(randr(OT_Potion_Slow, OT_Potion_Wall), x, y);
  }
  SPAWN_END;

  logText(" Your surrounded by Dragon Eggs. Find the Key and Escape!");
}

void Start()
{
  GAME = Scope_New(Game);
  SECTION = Scope_New(SectionData);
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

  if (Input_GetActionReleased(AC_USE) && GAME->potion != 0)
  {
    GAME->player.dX = 0;
    GAME->player.dY = 0;
    move = true;
    wait = false;

    switch(GAME->potion)
    {
      case OT_Potion_Slow:
      {
        u32 count = 0;
        for(u32 i=0; i < GAME->nbObjects;i++)
        {
          Object* obj = &GAME->objects[i];
          int dx = (obj->x - GAME->player.x);
          int dy = (obj->y - GAME->player.y);
          
          if ((dx * dx) + (dy * dy) < (7 * 7) && Object_IsDragon(obj->type))
          {
            obj->moveTime += 6;
            count++;
          }
        }


        logText("Used Splash Potion of Slowness. %i Dragons Slowed.", count);
      }
      break;
      case OT_Potion_Wall:
      {
        for(u32 i=0;i < 8;i++)
        {
          XY xy = XYDir[i];
          xy.x += GAME->player.x;
          xy.y += GAME->player.y;
          if (!CanCollide(xy.x, xy.y))
          {
            SECTION->col[xy.x + (xy.y * SECTION_WIDTH)] = 63;
            SECTION->non[xy.x + (xy.y * SECTION_WIDTH)] = 0;
            break;
          }

          logText("Used Splash Potion of Wall Making");
        }
      }
      break;
    }

    GAME->potion = 0;
  }

  Section_Draw(0);

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

        if (obj->noMove >= 3)
        {
          obj->noMove = 0;
          obj->dX = randr(0,2) - 1;
          obj->dY = randr(0,2) - 1;
        }
        else
        {
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
        }
        
        Object_Tick(obj);

        if (GAME->player.x == obj->x && GAME->player.y == obj->y && Object_IsDragon(obj->type))
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

  if (GAME->potion != 0)
  {
    XY xy;
    Object_GetSpriteTileIndex(GAME->potion, &xy);
    Bitmap* bitmap = Object_GetBitmap(GAME->potion);
    if (bitmap != NULL)
    {
      Tile_Draw(bitmap, 0, 0,  xy.x, xy.y);
    }
  }

  Canvas_PrintF(1, Canvas_GetHeight() - 12, &FONT_NEOSANS, 15, "> %s", LOG_TEXT);

}
