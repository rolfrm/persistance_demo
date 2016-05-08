#include <GL/glew.h>
#include <iron/full.h>
#include "persist.h"
#include "shader_utils.h"
#include "game.h"

#include <GLFW/glfw3.h>


void window_pos_callback(GLFWwindow* window, int xpos, int ypos)
{
  UNUSED(window);
  window_state * w = persist_alloc("win_state", sizeof(window_state));
  w->x = xpos;
  w->y = ypos;
  glfwGetWindowPos(window, &w->x, &w->y);
}

void window_size_callback(GLFWwindow* window, int width, int height)
{
  UNUSED(window);
  window_state * w = persist_alloc("win_state", sizeof(window_state));
  w->width = width;
  w->height = height;
}

int main(){
  if (!glfwInit())
    return -1;
  window_state * w = persist_alloc("win_state", sizeof(window_state));
  if(w->initialized == false){
    w->width = 640;
    w->height = 640;
    w->x = 200;
    w->y = 200;
    w->initialized = true;
    sprintf(w->title, "%s", "Win!");
  }
  GLFWwindow * window = glfwCreateWindow(w->width, w->height, w->title, NULL, NULL);
  glfwSetWindowPos(window, w->x, w->y);

  glfwSetWindowPosCallback(window, window_pos_callback);
  glfwSetWindowSizeCallback(window, window_size_callback);
  glfwMakeContextCurrent(window);
  glewInit();
  controller_state * controller = persist_alloc("controller", sizeof(controller_state));
  memset(controller, 0, sizeof(controller_state)); // reset controller on start.
  bool spaceDown = false;
  while(true){
    controller->axis.x = 0;
    controller->axis.y = 0;
    
    if(glfwGetKey(window, GLFW_KEY_LEFT)){
      controller->axis.x -= 0.1;
    }
    if(glfwGetKey(window, GLFW_KEY_RIGHT)){
      controller->axis.x += 0.1;
    }
    if(glfwGetKey(window, GLFW_KEY_UP)){
      controller->axis.y += 0.1;
    }
    if(glfwGetKey(window, GLFW_KEY_DOWN)){
      controller->axis.y -= 0.1;
    }
    if(glfwGetKey(window, GLFW_KEY_SPACE)){
      if(spaceDown == false)
	controller->btn1_clicks += 1;
      spaceDown = true;
    }else{
      spaceDown = false;
    }
    glViewport(0, 0, w->width, w->height);
    game_loop();
    render_game();
    glfwSwapBuffers(window);
    glfwPollEvents();
    //iron_sleep(0.1);
  }
  glfwTerminate();
  return 0;
}
