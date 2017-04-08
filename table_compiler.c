#include <iron/full.h>


void build_template(int argc, char ** argv, const char * template_file, const char * output_file){
  
  char * name = argv[1];
  char * template = read_file_to_string(template_file);
  template = realloc(template, 1000000);
  replace_inplace(template, "TABLE_NAME", name);
  char *columns1[argc - 2];
  char *columns2[argc - 2];
  //char * column_names[argc - 2];
  char * column_types[argc - 2];
  char * mem_areas[argc - 2];
  char * ptr_attrs1[argc - 2];
  char * ptr_attrs2[argc - 2];
  char * column_sizes[argc - 2];
  char * column_name_strs[argc - 2];
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
    ptr_attrs1[i - 2] = fmtstr("(void* )&%s", column_name);
    ptr_attrs2[i - 2] = fmtstr("(void* )%s", column_name);
    column_sizes[i - 2] = fmtstr("sizeof(%s)", column_type);
    column_name_strs[i - 2] = fmtstr("(char *)\"%s\"", column_name);
  }
  char * value_columns = string_join(array_count(columns1), ";\n  ", columns2);
  char * value_columns2 = string_join(array_count(columns1), ", ", columns1);
  char * value_columns3 = string_join(array_count(columns2), ", ", columns2);
  char * areas = string_join(array_count(mem_areas), ";\n  ", mem_areas);
  char * ptrs1 = string_join(array_count(columns2), ", ", ptr_attrs1);
  char * ptrs2 = string_join(array_count(columns2), ", ", ptr_attrs2);
  char * colcount = fmtstr("%i", argc - 2);
  char * colsize = fmtstr("{%s}", string_join(array_count(columns2),", ", column_sizes));
  char * namearray = fmtstr("{%s}", string_join(array_count(columns2),", ", column_name_strs));
  replace_inplace(template, "VALUE_COLUMNS1", value_columns);
  replace_inplace(template, "VALUE_COLUMNS2", value_columns2);
  replace_inplace(template, "VALUE_COLUMNS3", value_columns3);
  replace_inplace(template, "MEM_AREAS", areas);
  replace_inplace(template, "INDEX_TYPE", column_types[0]);
  replace_inplace(template, "VALUE_PTRS1", ptrs1);
  replace_inplace(template, "VALUE_PTRS2", ptrs2);
  replace_inplace(template, "COLUMN_COUNT", colcount);
  replace_inplace(template, "COLUMN_SIZES", colsize);
  replace_inplace(template, "COLUMN_NAMES", namearray);
  write_buffer_to_file(template, strlen(template), output_file);
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
