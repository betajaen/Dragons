
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


static Font           FONT_NEOSANS;
static Bitmap         SPRITESHEET;
static Bitmap         SPRITESHEET_HUMANOID[2];
static Bitmap         SPRITESHEET_REPTILE[2];
static Bitmap         SPRITESHEET_WALLS;
static Bitmap         SPRITESHEET_FLOORS;

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
  return (u32) ((max - min + 1)*scaled + min);
}

typedef struct
{
  U16   col[SECTION_WIDTH * SECTION_HEIGHT];
  U16   non[SECTION_WIDTH * SECTION_HEIGHT];
} SectionData;

SectionData* SECTION_DATA;
U32 SECTION_DATA_COUNT;

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

        if (v > 820)
        {
          section->col[i] = v - 820;
          section->non[i] = 0;
        }
        else
        {
          section->col[i] = 0;
          section->non[i] = v - 1;
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

  Font_Load("NeoSans.png", &FONT_NEOSANS, Colour_Make(0,0,255), Colour_Make(255,0,255));
  Bitmap_Load("Humanoid0.png", &SPRITESHEET_HUMANOID[0], 0);
  Bitmap_Load("Humanoid1.png", &SPRITESHEET_HUMANOID[1], 0);
  Bitmap_Load("Reptile0.png",  &SPRITESHEET_REPTILE[0], 0);
  Bitmap_Load("Reptile1.png",  &SPRITESHEET_REPTILE[1], 0);
  Bitmap_Load("Wall.png", &SPRITESHEET_WALLS, 0);
  Bitmap_Load("Floor.png", &SPRITESHEET_FLOORS, 0);

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
  SectionData* section = &SECTION_DATA[index]; 

  for(u32 x=0;x < SECTION_WIDTH;x++)
  {
    for(u32 y=0;y < SECTION_HEIGHT;y++)
    {
      u32 idx = x + (y * SECTION_WIDTH);

      if (section->col[idx] > 0)
      {
        u32 s = section->col[idx];
        u32 sx = (s % 20);
        u32 sy = (s / 20);
        Tile_Draw(&SPRITESHEET_WALLS, x * TILE_SIZE, y * TILE_SIZE, sx, sy);
      }
      else
      {
        u32 s = section->non[idx];
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
  u32    section;
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

bool CanCollide(u32 x, u32 y)
{
  u32 idx = x + (y * SECTION_WIDTH);
  SectionData* section = &SECTION_DATA[GAME->section];
  if (section->col[idx])
  {
    return true;
  }
  else
  {
    return section->non[idx] < 45;
  }
}

Bitmap* Object_GetBitmap(u8 type)
{
  switch(type)
  {
    case OT_Human:       return &SPRITESHEET_HUMANOID[AnimationTimer % 2];
    case OT_FireDragon:  return &SPRITESHEET_REPTILE[AnimationTimer % 2];
    case OT_WaterDragon: return &SPRITESHEET_REPTILE[AnimationTimer % 2];
    case OT_EarthDragon: return &SPRITESHEET_REPTILE[AnimationTimer % 2];
    case OT_AirDragon:   return &SPRITESHEET_REPTILE[AnimationTimer % 2];
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

    if (CanCollide(tx, ty))
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

  for(u32 d=0;d < 4;d++)
  {
    for(int i=0;i < 4;i++)
    {
      u32 x = randr(0, SECTION_WIDTH - 1);
      u32 y = randr(0, SECTION_HEIGHT - 1);
      if (!CanCollide(x, y))
      {
        Monster_New(OT_FireDragon + d, x, y);
      }
    }
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
