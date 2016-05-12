
typedef struct{
  vec3 axis;
  int btn1_clicks;
  vec2 direction;
  bool should_exit;
}controller_state;

typedef enum{
  kind_player,
  kind_wall,
  kind_enemy,
  kind_coin,
  kind_gun,
  kind_bullet,
  kind_max

}circle_kinds;

typedef struct{
  vec2 offset;
  int btn1_last;
}map_pos;

typedef struct{
  bool active;
  vec2 pos;
  float size;
  vec3 color;
  vec2 vel;
  int kind;
  float phase;
}circle;

typedef struct{
  int width, height;
  int x, y;
  bool initialized;
  char title[64];
}window_state;

void game_loop();
void render_game();
