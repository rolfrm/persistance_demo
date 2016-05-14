
typedef struct{
  vec3 axis;
  bool btn1_clicked;
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
  kind_target,
  kind_turret,
  kind_max

}circle_kinds;

typedef struct{
  vec2 offset;
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
  bool active;
  int base_circle;
  int gun_circle;
  float cooldown;
}turret;
typedef struct{
  int width, height;
  int x, y;
  bool initialized;
  char title[64];
}window_state;

void game_loop();
void render_game();
