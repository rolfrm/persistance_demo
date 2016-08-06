typedef struct{
  vec2 position;
  vec2 size;
} body;

void load_game_board(u64 id);
void update_game_board(u64 id);
u64 get_visible_items(u64 id, u64 * items, u64 items_cnt);

CREATE_TABLE_DECL2(body, u64, body);
CREATE_TABLE_DECL2(board_elements, u64, u64)

// here?
CREATE_MULTI_TABLE_DECL(faction_visible_items, u64, u64)

CREATE_STRING_TABLE_DECL(name, u64);
CREATE_MULTI_TABLE_DECL(inventory, u64, u64);

CREATE_TABLE_DECL(target, u64, vec2);
CREATE_TABLE_DECL(is_paused, u64, bool);
CREATE_TABLE_DECL(should_exit, u64, bool);
CREATE_TABLE_DECL(is_instant, u64, bool);
CREATE_TABLE_DECL(wielded_item, u64, u64);
CREATE_TABLE_DECL(item_command_item, u64, u64);
CREATE_TABLE_DECL(is_wall, u64, bool);
CREATE_TABLE_DECL2(faction, u64, u64);
