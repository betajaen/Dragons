#define RETRO_WINDOW_CAPTION "Dragons"
#define RETRO_ARENA_SIZE Kilobytes(32)
#define RETRO_WINDOW_DEFAULT_WIDTH 640
#define RETRO_WINDOW_DEFAULT_HEIGHT 480
#define RETRO_CANVAS_DEFAULT_WIDTH (RETRO_WINDOW_DEFAULT_WIDTH / 2)
#define RETRO_CANVAS_DEFAULT_HEIGHT (RETRO_WINDOW_DEFAULT_HEIGHT / 2)

#define SECTION_WIDTH 22  //(RETRO_CANVAS_DEFAULT_WIDTH / TILE_SIZE)
#define SECTION_HEIGHT 15 //(RETRO_CANVAS_DEFAULT_HEIGHT / TILE_SIZE)

#define LEVEL_WIDTH (SECTION_WIDTH * 2)
#define LEVEL_HEIGHT (SECTION_HEIGHT)
#define MAX_OBJECTS_PER_SECTION 16
#define SECTIONS_PER_LEVEL (4)


#define VERSION "1.0"

#include "retro.c"

static Font           FONT_NEOSANS;
static Bitmap         SPRITESHEET;
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
typedef uint8_t u16;
typedef uint8_t u32;
typedef uint8_t u64;
typedef uint8_t s8;
typedef uint8_t s16;
typedef uint8_t s32;
typedef uint8_t s64;

typedef enum 
{
  AC_ACTION,
  AC_MUSIC,
  AC_RESET
} Actions;

void Init(Settings* settings)
{
  Palette_Make(&settings->palette);
  Palette_LoadFromBitmap("palette.png", &settings->palette);

  Input_BindKey(SDL_SCANCODE_SPACE, AC_ACTION);
  Input_BindKey(SDL_SCANCODE_M, AC_MUSIC);

  Font_Load("NeoSans.png", &FONT_NEOSANS, Colour_Make(0,0,255), Colour_Make(255,0,255));
  Bitmap_Load("cave.png", &SPRITESHEET, 0);
  Bitmap_Load("cats.png", &CATSHEET, 16);
  Bitmap_Load("player.png", &PLAYERSHEET, 16);
  Bitmap_Load("coco.png", &COCO, 255);

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

void Splat_Tile(Bitmap* bitmap, S32 x, S32 y, S32 s, U32 index)
{
  SDL_Rect src, dst;
  

  if (index == 0)
  {
    src.x = 0;
    src.y = 0;
  }
  else
  {
    src.x = (index % 16) * s;
    src.y = (index / 16) * s;
  }

  src.w = s;
  src.h = s;

  dst.x = x;
  dst.y = y;
  dst.w = s;
  dst.h = s;

  Canvas_Splat3(bitmap, &dst, &src);
}

typedef enum
{
  PF_Idle,
  PF_Walking
} PlayerState;

typedef struct
{
    u32 x, y;
} Game;

Game*      GAME;
bool       STEP_MODE;

void Start()
{
  GAME = Scope_New(Game);
  Canvas_SetFlags(0, CNF_Render | CNF_Clear, 8);
  Scope_Push('COCO');
}

void Step()
{
}
