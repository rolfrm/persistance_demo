#include <iron/full.h>


void build_template(int argc, char ** argv, const char * template_file, const char * output_file){
  
  char * name = argv[1];
  char * template = read_file_to_string(template_file);
  template = realloc(template, strlen(template) * 2);
  replace_inplace(template, "TABLE_NAME", name);
  char *columns1[argc - 2];
  char *columns2[argc - 2];
  //char * column_names[argc - 2];
  char * column_types[argc - 2];
  char * mem_areas[argc - 2];
  for(int i = 2; i < argc; i++){
    char * column = argv[i];
    int splitcnt;    
    char ** parts = string_split(column, ":", &splitcnt);
    ASSERT(splitcnt == 2);
    char * column_name = parts[0];
    char * column_type = parts[1];

    columns1[i - 2] = fmtstr("%s %s", column_type, column_name);
    columns2[i - 2] = fmtstr("%s * %s", column_type, column_name);
    //column_names[i - 2] = column_name;
    column_types[i - 2] = column_type;
    mem_areas[i - 2] = fmtstr("mem_area * %s_area", column_name);
  }
  char * value_columns = string_join(array_count(columns1), ";\n  ", columns2);
  char * value_columns2 = string_join(array_count(columns1), ", ", columns1);
  char * value_columns3 = string_join(array_count(columns2), ", ", columns2);
  char * areas = string_join(array_count(mem_areas), ";\n  ", mem_areas);
  replace_inplace(template, "VALUE_COLUMNS1", value_columns);
  replace_inplace(template, "VALUE_COLUMNS2", value_columns2);
  replace_inplace(template, "VALUE_COLUMNS3", value_columns3);
  replace_inplace(template, "MEM_AREAS", areas);
  replace_inplace(template, "INDEX_TYPE", column_types[0]);
  write_buffer_to_file(template, strlen(template) + 1, output_file);
}


//usage
// compile_table [name] 
int main(int argc, char ** argv){
  logd("Args:");
  for(int i = 0; i < argc; i++){
    logd("%i %s\n", i, argv[i]);
  }
  build_template(argc, argv, "table_template.h", fmtstr("%s.h", argv[1]));
  build_template(argc, argv, "table_template.c", fmtstr("%s.c", argv[1]));
  
}
