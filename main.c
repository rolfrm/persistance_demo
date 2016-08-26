
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
int main(){
  table2_test();
  test_persist_oop();
  test_walls();

  if (!glfwInit())
    return -1;
  test_gui();
  return 0;
}
