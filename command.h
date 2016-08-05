typedef enum{
  COMMAND_ARG_POSITION,
  COMMAND_ARG_ENTITY,
  COMMAND_ARG_ITEM,

} command_arg_type;

typedef struct{
  command_arg_type type;
  union{
    struct{
      int x;
      int y;
    };
    u64 id;
  };

}command_arg;

typedef enum{
  CMD_DONE,
  CMD_NOT_DONE
}command_state;

u64 parse_command(u64 id, const char * str, command_arg * out_args, u64 * in_out_cnt);

CREATE_TABLE_DECL(command_invocation, u64, u64);
CREATE_TABLE_DECL2(command_queue, u64, u64);
CREATE_MULTI_TABLE_DECL(command_args, u64, command_arg);
CREATE_MULTI_TABLE_DECL(available_commands, u64, u64);
CREATE_STRING_TABLE_DECL(command_name, u64);

extern u64 invoke_command_method;
extern u64 command_class;
