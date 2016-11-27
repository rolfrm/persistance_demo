
#include <GL/glew.h>
#include <iron/full.h>
#include "persist.h"
#include "sortable.h"
#include "persist_oop.h"
#include "shader_utils.h"
#include "game.h"

#include <GLFW/glfw3.h>

#include "gui.h"
#include "gui_test.h"

void table2_test();
void test_walls();
void test_persist_oop();
bool test_elf_reader2();
bool test_elf_reader();
bool test_type_system();
int main(){
  //table2_test();
  //test_persist_oop();
  //test_walls();
  //TEST(test_elf_reader2);
  TEST(test_elf_reader);
  //TEST(test_type_system);
  //return 0;
  if (!glfwInit())
    return -1;
  test_gui();
  return 0;
}
