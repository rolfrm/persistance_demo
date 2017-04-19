void node_roguelike_ui_init();
void node_roguelike_ui_update(graphics_context * ctx);
void node_roguelike_ui_show(u32 uinode, u32 entity);
typedef void (*ui_node_action)(u32 node_id);
void node_roguelike_ui_register(u32 actionid, ui_node_action action);
extern u32_lookup * ui_nodes;
extern u32_to_u32 * ui_subnodes;
extern u32_to_u32 * ui_node_actions;
