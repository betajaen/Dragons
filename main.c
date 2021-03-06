
#define RETRO_WINDOW_CAPTION "Dragons are Hatching From Eggs And You Have To Escape!"
#define RETRO_ARENA_SIZE Kilobytes(64)
#define RETRO_WINDOW_DEFAULT_WIDTH 1280
#define RETRO_WINDOW_DEFAULT_HEIGHT 560
#define RETRO_CANVAS_DEFAULT_WIDTH (RETRO_WINDOW_DEFAULT_WIDTH / 2)
#define RETRO_CANVAS_DEFAULT_HEIGHT (RETRO_WINDOW_DEFAULT_HEIGHT / 2)

#include "retro.c"
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>

#define TILE_SIZE 16
#define SECTION_WIDTH   40
#define SECTION_HEIGHT  16

#define LEVEL_WIDTH (SECTION_WIDTH * 2)
#define LEVEL_HEIGHT (SECTION_HEIGHT)
#define SECTION_COUNT 4
#define TILESET_COUNT 3

#define MAX_MONSTERS 256

#define VERSION "1.0"
#define CHEAT_MODE 0
#define SECTION_START 0


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
  OT_Potion_Harming,
  OT_Door,
  OT_Key
};


static Font           FONT_NEOSANS;
static Bitmap         SPRITESHEET;
static Bitmap         SPRITESHEET_TILESET[3];
static Sound          SOUND_JUMP1;
static Sound          SOUND_JUMP2;
static Sound          SOUND_HURT1;
static Sound          SOUND_HURT2;
static Sound          SOUND_CRUSH;
static Sound          SOUND_PICKUP2;
static Sound          SOUND_SELECT;

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
  AC_USE,
  AC_CHEAT
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
u32 SECTION_ID;

void logText(const char* format, ...)
{
  va_list argptr;
  va_start(argptr, format);
  vsprintf(LOG_TEXT, format, argptr);
  va_end(argptr);
  printf(LOG_TEXT);
  printf("\n");
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

u8 GetTileType(u32 v)
{
  switch(v - 1)
  {
    case 7:
    case 16:
    case 17:
    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
    case 35:
    case 36:
    case 37:
    case 38:
    case 39:
    case 45:
    case 46:
    case 47:
    case 48:
    case 49:
    case 50:
    case 51:
    case 52:
    case 53:
    case 59:
      return 0; // Floor
    case 1:
      return 2; // Door (Object)
  }

  return 1; // Wall/Collidable
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

        u8 type = GetTileType(v);

        if (type == 0)
        {
          section->col[i] = 0;
          section->non[i] = v - 1;
          section->obj[i] = 0;
        }
        else if (type == 1)
        {
          section->col[i] = v - 1;
          section->non[i] = 0;
          section->obj[i] = 0;
        }
        else
        {
          section->col[i] = 0;
          section->non[i] = 0;

          if (v == 2)
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

  Input_BindKey(SDL_SCANCODE_ESCAPE, AC_QUIT);
  Input_BindKey(SDL_SCANCODE_W, AC_UP);
  Input_BindKey(SDL_SCANCODE_S, AC_DOWN);
  Input_BindKey(SDL_SCANCODE_A, AC_LEFT);
  Input_BindKey(SDL_SCANCODE_D, AC_RIGHT);
  Input_BindKey(SDL_SCANCODE_R, AC_WAIT);
  Input_BindKey(SDL_SCANCODE_SPACE, AC_USE);
  Input_BindKey(SDL_SCANCODE_1, AC_CHEAT);

  for(u32 i=0;i < TILESET_COUNT;i++)
  {
    char s[30];
    sprintf(s, "tileset%i.png", i + 1);
    Palette_LoadFromBitmap(s, &settings->palette);
    Bitmap_Load24(s, &SPRITESHEET_TILESET[i], 0xFF, 0x00, 0xFF);
  }

  Font_Load("NeoSans.png", &FONT_NEOSANS, Colour_Make(0,0,255), Colour_Make(255,0,255));

  Sound_Load(&SOUND_HURT1,   "hurt1.wav");
  Sound_Load(&SOUND_HURT2,   "hurt2.wav");
  Sound_Load(&SOUND_JUMP1,   "jump1.wav");
  Sound_Load(&SOUND_JUMP2,   "jump2.wav");
  Sound_Load(&SOUND_CRUSH,   "crush.wav");
  Sound_Load(&SOUND_PICKUP2, "pickup2.wav");
  Sound_Load(&SOUND_SELECT,  "select.wav");

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

      u32 s = SECTION->col[idx] > 0 ? SECTION->col[idx] : SECTION->non[idx];

      u32 sx = (s % 10);
      u32 sy = (s / 10);
      Tile_Draw(&SPRITESHEET_TILESET[(SECTION_ID) % TILESET_COUNT], x * TILE_SIZE, y * TILE_SIZE, sx, sy);
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
  u32    level;
  u8     win;
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

  return false;
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
    case OT_Human:          
    case OT_FireDragon:     
    case OT_WaterDragon:    
    case OT_EarthDragon:    
    case OT_AirDragon:      
    case OT_Egg:            
    case OT_Potion_Slow:    
    case OT_Potion_Wall:    
    case OT_Potion_Harming:    
    case OT_Key:
                        return &SPRITESHEET_TILESET[(SECTION_ID + 1) % TILESET_COUNT];
    case OT_Door:           
                        return &SPRITESHEET_TILESET[(SECTION_ID) % TILESET_COUNT];
  }
  return NULL;
}


void Object_GetSpriteTileIndex(u8 type, XY* xy)
{
  u32 idx = 0;

  xy->x = 0; xy->y = 0;
  switch(type)
  {
    case OT_Human:          idx = 190; break;
    case OT_FireDragon:     idx = 221; break;
    case OT_WaterDragon:    idx = 228; break;
    case OT_EarthDragon:    idx = 229; break;
    case OT_AirDragon:      idx = 212; break;
    case OT_Egg:            idx = 173; break;
    case OT_Potion_Slow:    idx = 97;  break;
    case OT_Potion_Wall:    idx = 98;  break;
    case OT_Potion_Harming: idx = 99;  break;
    case OT_Door:           idx = 2;   break;
    case OT_Key:            idx = 80;  break;
  }

  if (idx > 0)
  {
    xy->x = idx % 10;
    xy->y = idx / 10;
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
  return UINT32_MAX;
}

s32 Object_Animation(u8 type)
{
  switch(type)
  {
    case OT_FireDragon: 
    case OT_WaterDragon:
    case OT_EarthDragon:
    case OT_AirDragon:   return (AnimationTimer % 2) == 0 ? 0 : 1;
  }
  return 0;
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

  XY xy;
  Object_GetSpriteTileIndex(object->type, &xy);
  Tile_Draw(bitmap, object->x * TILE_SIZE, object->y * TILE_SIZE + Object_Animation(object->type),  xy.x, xy.y);

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
    case OT_Potion_Harming:
    case OT_Key:
    case OT_Door:
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
  if (GAME->potion == 0)
  {
    GAME->potion = object->type;
    object->type = OT_UsedObject;
    Sound_Play(&SOUND_SELECT, RETRO_SOUND_DEFAULT_VOLUME);
  }
}

void Game_KeySpawnChance(u32 x, u32 y)
{
  if (GAME->keyDropped == false)
  {
    // any eggs
    u32 eggCount = Object_Count(OT_Egg);
    u32 dragonCount = Object_CountDragon();

    int chance = eggCount + dragonCount;
    int r = randr(1, chance);

    if (r == 1)
    {
      Object_New(OT_Key, x, y);
      GAME->keyDropped = true;
      logText("A key was dropped");
    }
  }
}

void RestartLevel();

void NextLevel()
{
  logText("Win!");
  GAME->level++;
  if (GAME->level == SECTION_DATA_COUNT)
  {
    Scope_Push('WIN');
  }

  RestartLevel();
}

bool Object_Tick(Object* object)
{
  bool canMove = false, win = false;
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
        bool otherIsPassThrough  = Object_CanPassthrough(other->type);

        if (other->x == tx && other->y == ty)
        {
          if (Object_IsDragon(object->type) && Object_IsDragon(other->type))
          {
            canMove = true;
            object->type = OT_UsedObject;
            other->type  = OT_UsedObject;
            Game_KeySpawnChance(object->x, object->y);
            Game_KeySpawnChance(other->x, other->y);
            logText("Two dragons have collided");
            Sound_Play(&SOUND_HURT1, RETRO_SOUND_DEFAULT_VOLUME);
          }
          else if (isPlayer && otherIsPassThrough)
          {
            canMove = true;
            switch(other->type)
            {
              case OT_Egg:
                other->type = OT_UsedObject;
                Game_KeySpawnChance(tx, ty);
                logText("An Egg was crushed");
                Sound_Play(&SOUND_CRUSH, RETRO_SOUND_DEFAULT_VOLUME);
              break;
              case OT_Potion_Slow:
                logText("Picked up Splash Potion of Slowness");
                Object_PickupPotion(other);
              break;
              case OT_Potion_Wall:
                logText("Picked up Splash Potion of Wall Making");
                Object_PickupPotion(other);
              break;
              case OT_Key:
                other->type = OT_UsedObject;
                GAME->held  = OT_Key;
              break;
              case OT_Door:
                canMove = false;
                
                if (GAME->held == OT_Key)
                {
                  GAME->win = true;
                }
              break;
            }
          }
          else if (!otherIsPlayer)
          {
            canMove = false;
          }
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
          case OT_EarthDragon: logText("A Earth Dragon has hatched!");  break;
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
  u32 level = GAME->level;
  memset(GAME, 0, sizeof(Game));
  GAME->level = level;
  SECTION_ID = GAME->level;
  GAME->win = false;

  Canvas_SetFlags(0, CNF_Render | CNF_Clear, (SECTION_ID * 5) + 1);

  u32 sectionId = GAME->level;
  memcpy(SECTION, &SECTION_DATA[sectionId],sizeof(SectionData));

  u32 mx = SECTION_WIDTH / 2;
  u32 my = SECTION_HEIGHT / 2;

#if CHEAT_MODE
  mx = 4;
  my = 4;
#endif
  Player_New(mx, my);

#if CHEAT_MODE
  GAME->held = OT_Key;
#endif

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



  SPAWN_BEGIN(6, 6 + (GAME->level * 10))
  {
    Object_New(OT_Egg, x, y);
  }
  SPAWN_END;

  SPAWN_BEGIN(4, 12)
  {
    Object_New(randr(OT_Potion_Slow, OT_Potion_Wall), x, y);
  }
  SPAWN_END;

  logText("Your surrounded by Dragon Eggs. Find the Key and Escape!");
}

void Start()
{
  GAME = Scope_New(Game);
  GAME->level = SECTION_START;
  SECTION = Scope_New(SectionData);
  Scope_Push('LEVL');
  RestartLevel();
  Music_Play("dragon2.mod");
}

void Step()
{
  if (Scope_GetName() == 'WIN')
  {
    Canvas_SetFlags(0, CNF_Render | CNF_Clear, 0);

    u32 x = (RETRO_CANVAS_DEFAULT_WIDTH / 2) - 100;
    u32 y = (RETRO_CANVAS_DEFAULT_HEIGHT / 2) - 40;

    Canvas_PrintF(x, y, &FONT_NEOSANS, 3, "Well done you escaped!");
    Canvas_PrintF(x, y + 10, &FONT_NEOSANS, 3, "Press <USE> to play again");
    
    XY xy;
    Object_GetSpriteTileIndex(OT_Human, &xy);
    Bitmap* bitmap = Object_GetBitmap(OT_Human);
    if (bitmap != NULL)
    {
      Tile_Draw(bitmap, x - 16, y, xy.x, xy.y);
    }

    if (Input_GetActionReleased(AC_USE))
    {
      GAME->level = SECTION_START;
      RestartLevel();
      Scope_Pop();
    }
    return;
  }

  bool move = false;
  bool wait = false;

  if (Input_GetActionReleased(AC_CHEAT))
  {
    NextLevel();
  }
  else if (Input_GetActionReleased(AC_UP))
  {
    GAME->player.dY = -1;
    move = true;
  }
  else if (Input_GetActionReleased(AC_DOWN))
  {
    GAME->player.dY = 1;
    move = true;
  }
  else if (Input_GetActionReleased(AC_LEFT))
  {
    GAME->player.dX = -1;
    move = true;
  }
  else if (Input_GetActionReleased(AC_RIGHT))
  {
    GAME->player.dX = 1;
    move = true;
  }
  else if (Input_GetActionReleased(AC_WAIT))
  {
    GAME->player.dX = 0;
    GAME->player.dY = 0;
    move = true;
    wait = true;
  }
  else if (Input_GetActionReleased(AC_USE) && GAME->potion != 0)
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
            SECTION->col[xy.x + (xy.y * SECTION_WIDTH)] = 15;
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

  if (GAME->win)
  {
    NextLevel();
    return;
  }

  FrameCount++;
  AnimationTimer += (FrameCount % 4 == 0 ? 1 : 0);

  for(u32 i=0; i < GAME->nbObjects;i++)
  {
    Object_Draw(&GAME->objects[i]);
  }

  Object_Draw(&GAME->player);

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

  if (GAME->held != 0)
  {
    XY xy;
    Object_GetSpriteTileIndex(GAME->held, &xy);
    Bitmap* bitmap = Object_GetBitmap(GAME->held);
    if (bitmap != NULL)
    {
      Tile_Draw(bitmap, 16, 0,  xy.x, xy.y);
    }
  }
  Canvas_PrintF(4, Canvas_GetHeight() - 12, &FONT_NEOSANS, (SECTION_ID * 5) + 3, "> %s", LOG_TEXT);

}
