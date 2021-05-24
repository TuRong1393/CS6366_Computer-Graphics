#pragma once

#include <iostream>
// #include <fstream>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "nanogui/nanogui.h"

#include "stb_image.h"

#include "Shader.h"
#include "Camera.h"

class Renderer
{
public:
	static GLFWwindow *m_window;
	static Camera *m_camera;
	static nanogui::Screen *m_nanogui_screen;

public:
	Renderer();
	~Renderer();
	void run();

private:
	void init();
	void nanogui_init(GLFWwindow *window);
	void display(GLFWwindow *window);
	void setup_uniform_values(Shader &shader);
};
