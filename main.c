
#include <GL/glew.h>
#include <iron/full.h>
#include "persist.h"
#include "persist_oop.h"
#include "shader_utils.h"
#include "game.h"

#include <GLFW/glfw3.h>

#include "gui.h"
#include "gui_test.h"

void table2_test();
void test_walls();
int main(){
  table2_test();
  test_walls();
  
  //return 0;
  if (!glfwInit())
    return -1;
  test_gui();
  return 0;
}
