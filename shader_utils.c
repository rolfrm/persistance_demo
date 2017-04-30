#include <stdio.h>
#include <stddef.h> 
#include <stdint.h>
#include <stdbool.h>
#include <iron/types.h>
#include <iron/linmath.h>
#include <string.h>
#include <unistd.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

i32 make_shader(u32 kind, char * source, u32 length){
  i32 ref = glCreateShader(kind);
  glShaderSource(ref,1,(const GLchar **)&source,(i32 *) &length);
  glCompileShader(ref);

  GLint status = 0;
  glGetShaderiv(ref, GL_COMPILE_STATUS, &status);
  if(status == GL_FALSE){
    GLint infoLogLength = 0;
    glGetShaderiv(ref, GL_INFO_LOG_LENGTH, &infoLogLength);
    char log[infoLogLength];
    glGetShaderInfoLog(ref, infoLogLength, NULL, log);
    printf("Failure to compile shader. Info log:\n **************************\n");
    printf(log);
    printf("\n***************************\n");
    printf("Source:\n");
    printf("%s", source);
    printf("\n***************************\n");
    glDeleteShader(ref);
    return -1;
  }else{
    GLint infoLogLength = 0;
    glGetShaderiv(ref, GL_INFO_LOG_LENGTH, &infoLogLength);
    char log[infoLogLength];
    memset(log, 0, sizeof(log));
    glGetShaderInfoLog(ref, infoLogLength, NULL, log);
    printf("Shader Info log:\n **************************\n");
    printf(log);
    printf("\n***************************\n");
    //printf("Source:\n");
    //printf("%s", source);
    //printf("\n***************************\n");
  }

  return ref;
}

i32 load_simple_shader(char * vertsrc, i32 vslen, char * fragsrc, i32 fslen){
  i32 fs = make_shader(GL_FRAGMENT_SHADER, fragsrc, fslen);
  i32 vs = make_shader(GL_VERTEX_SHADER, vertsrc, vslen);
  if(vs == -1 || fs == -1){
    if(vs > 0) glDeleteShader(vs);
    if(fs > 0) glDeleteShader(fs);
    return -1;
  }
  u32 program = glCreateProgram();
  glAttachShader(program,fs);
  glAttachShader(program,vs);
  glLinkProgram(program);
  glDeleteShader(fs);
  glDeleteShader(vs);
  GLint status;
  glGetProgramiv(program, GL_LINK_STATUS, &status);
  if(status == GL_FALSE){
    GLint infoLogLength;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
    
    GLchar strInfoLog[infoLogLength];
    glGetProgramInfoLog(program, infoLogLength, NULL, strInfoLog);
    fprintf(stderr, "Linker failure: %s\n", strInfoLog);
    glDeleteProgram(program);
    return -1;
  }
  return (i32)program;
}

i32 load_simple_shader2(char * geomsrc, i32 gslen, char * vertsrc, i32 vslen, char * fragsrc, i32 fslen){
  printf("GS\n");
  i32 gs = make_shader(GL_GEOMETRY_SHADER, geomsrc, gslen);
  printf("FS\n");
  i32 fs = make_shader(GL_FRAGMENT_SHADER, fragsrc, fslen);
  printf("VS\n");
  i32 vs = make_shader(GL_VERTEX_SHADER, vertsrc, vslen);
  if(vs == -1 || fs == -1 || gs == -1){
    if(gs > 0) glDeleteShader(gs);
    if(vs > 0) glDeleteShader(vs);
    if(fs > 0) glDeleteShader(fs);
    return -1;
  }
  u32 program = glCreateProgram();
  glAttachShader(program, gs);
  glAttachShader(program, fs);
  glAttachShader(program, vs);
  glLinkProgram(program);
  glDeleteShader(fs);
  glDeleteShader(vs);
  glDeleteShader(gs);
  GLint status;
  glGetProgramiv(program, GL_LINK_STATUS, &status);
  if(status == GL_FALSE){
    GLint infoLogLength;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
    
    GLchar strInfoLog[infoLogLength];
    glGetProgramInfoLog(program, infoLogLength, NULL, strInfoLog);
    fprintf(stderr, "Linker failure: %s\n", strInfoLog);
    glDeleteProgram(program);
    return -1;
  }
  return (i32)program;
}

void uniform_vec3(int loc, vec3 value){
  glUniform3f(loc, value.x, value.y, value.z);
}

void uniform_mat4(int loc, mat4 mat){
  glUniformMatrix4fv(loc, 1, false, (float *)&mat);
}
