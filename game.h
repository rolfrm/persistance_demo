
typedef struct{
  bool left_clicked;
  bool picking_color;
  vec2 mouse_pos;
  int selected_kind;
  int scroll_amount;
  vec3 color;
  float size;
  float phase;
  bool changing_size;

  bool changing_color_h;
  bool changing_color_s;
  bool changing_color_v;
  bool changing_kind;
  bool changing_phase;
  bool deleting;
}edit_mode;

typedef enum{
  MODE_GAME = 0,
  MODE_EDIT
}main_modes;
typedef struct{
  main_modes mode;
}main_state;


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
  kind_decal,
  kind_worm,
  kind_laser,
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
  int health;
}turret;

typedef struct{
  bool active;
  u32 bodies[6];
  u32 body_cnt;
}worm;

typedef struct{
  bool active;
  int bullet;
  float length;
  int laser;
}laser;

typedef struct{
  u64 ids[64 * 64];
}chunk;


worm * get_new_worm(worm ** worms, u64 * cnt);

void game_loop();
void render_game();

turret * find_turret(circle * c);
turret * get_new_turret();
void turret_disable(turret * t);

circle * get_new_circle(circle ** circles, u64 * cnt);
laser * get_new_laser();
