#include "Renderer.h"

// Pre-define gui variable handles:
#define GUI_DOUBLE nanogui::detail::FormWidget<double, std::integral_constant<bool, true> >
#define GUI_STRING nanogui::detail::FormWidget<std::string, std::true_type>
#define GUI_COLOR nanogui::detail::FormWidget<nanogui::Color, std::true_type>
#define GUI_BOOL nanogui::detail::FormWidget<bool, std::integral_constant<bool, true> >
#define GUI_FLOAT nanogui::detail::FormWidget<float, std::integral_constant<bool, true> >

Camera *Renderer::m_camera = new Camera();
GLFWwindow *Renderer::m_window = nullptr;
nanogui::Screen *Renderer::m_nanogui_screen = nullptr;
std::string Renderer::model_name = "../src/objs/cyborg.obj";
LocObject *m_object = new LocObject(Renderer::model_name);
DynamicLight *m_pointLight = new DynamicLight(Renderer::m_camera->getCamPos());

enum render_type
{
	Point = 0,
	Line = 1,
	Triangle = 2
};

enum culling_type
{
	CW = 0,
	CCW = 1
};

enum shading_type
{
	Smooth = 0,
	Flat = 1
};

enum depth_type
{
	Less = 0,
	Always = 1
};

#define GUI_RENDERTYPE nanogui::detail::FormWidget<render_type, std::integral_constant<bool, true> >
#define GUI_CULLINGTYPE nanogui::detail::FormWidget<culling_type, std::integral_constant<bool, true> >
#define GUI_SHADINGTYPE nanogui::detail::FormWidget<shading_type, std::integral_constant<bool, true> >
#define GUI_DEPTHTYPE nanogui::detail::FormWidget<depth_type, std::integral_constant<bool, true> >

// Gui variable handles to edit fields in the gui:
GUI_RENDERTYPE *gui_RenderType;
GUI_CULLINGTYPE *gui_CullingType;
GUI_SHADINGTYPE *gui_ShadingType;
GUI_DEPTHTYPE *gui_DepthType;
GUI_COLOR *gui_ColObject;
GUI_STRING *gui_ObjectFile;
GUI_DOUBLE *gui_RotValue;
GUI_DOUBLE *gui_PositionX;
GUI_DOUBLE *gui_PositionY;
GUI_DOUBLE *gui_PositionZ;
GUI_DOUBLE *gui_NearPlane;
GUI_DOUBLE *gui_FarPlane;

GUI_FLOAT *gui_ObjectShininess;
GUI_BOOL *gui_DirectionLightStatus;
GUI_COLOR *gui_DirectionLightAmbientColor;
GUI_COLOR *gui_DirectionLightDiffuseColor;
GUI_COLOR *gui_DirectionLightSpecularColor;
GUI_BOOL *gui_PointLightStatus;
GUI_COLOR *gui_PointLightAmbientColor;
GUI_COLOR *gui_PointLightDiffuseColor;
GUI_COLOR *gui_PointLightSpecularColor;
GUI_BOOL *gui_PointLightRotateX;
GUI_BOOL *gui_PointLightRotateY;
GUI_BOOL *gui_PointLightRotateZ;

// Variable defaults modified in the gui
//varibales in control bar_1
double dPositionX = 0.0;
double dPositionY = 0.0;
double dPositionZ = -10.0;
double dRotValue = 0.0;
double dNearPlane = Renderer::m_camera->NEAR_PLANE;
double dFarPlane = Renderer::m_camera->FAR_PLANE;
render_type render_val = render_type::Line;
culling_type culling_val = culling_type::CCW;
shading_type shading_val = shading_type::Smooth;
depth_type depth_val = depth_type::Less;

//varibales in control bar_2
nanogui::Color colObject(0.5f, 0.5f, 0.7f, 1.0f);
float fObjectShininess = 32.0f;
bool bDirectionLightStatus = true;
nanogui::Color colDirectionLightAmbient(1.0f, 1.0f, 1.0f, 1.0f);
nanogui::Color colDirectionLightDiffuse(1.0f, 1.0f, 1.0f, 1.0f);
nanogui::Color colDirectionLightSpecular(1.0f, 1.0f, 1.0f, 1.0f);
bool bPointLightStatus = false;
nanogui::Color colPointLightAmbient(1.0f, 1.0f, 1.0f, 1.0f);
nanogui::Color colPointLightDiffuse(1.0f, 1.0f, 1.0f, 1.0f);
nanogui::Color colPointLightSpecular(1.0f, 1.0f, 1.0f, 1.0f);
bool bPointLightRotateX = false;
bool bPointLightRotateY = false;
bool bPointLightRotateZ = false;

GLuint VBO, VAO;

//function foward declarations
void setCulling();
void setDepth();
void draw();
void setViewLoc(float min, float max);
void reset();
void resetPointLight();
void reloadObjectModel();
void bind_vaovbo(LocObject &cur_obj);

Renderer::Renderer() {}
Renderer::~Renderer() {}

void Renderer::run()
{
	init();
	display(Renderer::m_window);
}

void Renderer::init()
{
	// Initialize GLFW
	glfwInit();
	// Configure GLFW
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
#if defined(__APPLE__)
	//for MAC OS
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// Create a GLFWwindow object
	Renderer::m_window = glfwCreateWindow(Renderer::m_camera->width, Renderer::m_camera->height, "CS6366 Assignment 2", nullptr, nullptr);
	// Make the context of our window the main context on the current thread
	glfwMakeContextCurrent(Renderer::m_window);
	// Set the size of the rendering window
	glViewport(0, 0, Renderer::m_camera->width, Renderer::m_camera->height);

	nanogui_init(Renderer::m_window);

	glewExperimental = GL_TRUE;
	glewInit();
}

void Renderer::display(GLFWwindow *window)
{
	// Build and compile our shader program
	Shader ourShader("../src/shader/basic.vert", "../src/shader/basic.frag");
	reloadObjectModel();
	// int matrixLoc = ourShader.getLoc("MVP");
	// int colorLoc = ourShader.getLoc("ourColor");

	// Main frame while loop
	while (!glfwWindowShouldClose(Renderer::m_window))
	{
		// Specify culling type
		setCulling();
		// Specify depth type
		setDepth();
		// Check if any events are triggered
		glfwPollEvents();
		// Clear the color buffer
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Draw the triangle
		ourShader.use();
		glBindVertexArray(VAO);

		m_pointLight->setLightPos(Renderer::m_camera->getCamPos());
		m_pointLight->rotate(bPointLightRotateX, bPointLightRotateY, bPointLightRotateZ);

		// Set uniform values of shader
		setup_uniform_values(ourShader);

		// Draw with specified render type .
		draw();

		glBindVertexArray(0);

		// Set draw mode back to fill & draw gui
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		Renderer::m_nanogui_screen->drawWidgets();
		// swap the color buffer and show it to the screen
		glfwSwapBuffers(Renderer::m_window);
	}

	// De-allocate all resources once they've outlived their purpose
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	ourShader.del();

	// Terminate GLFW, clearing all previously allocated GLFW resources.
	glfwTerminate();
}

/* Setup the unifomr values of the shader*/
void Renderer::setup_uniform_values(Shader &shader)
{
	shader.setBool("isSmooth", shading_val == shading_type::Smooth);
	shader.setMatrix4fv("MVP", 1, GL_FALSE, &(Renderer::m_camera->getMVPMatrix()[0][0]));
	shader.setVec3("colObject", colObject.r(), colObject.g(), colObject.b());
	shader.setFloat("shininess", fObjectShininess);
	shader.setVec3("camPos", 1, &(Renderer::m_camera->getCamPos()[0]));

	// Temp colors
	glm::vec3 colAmbient(0, 0, 0);
	glm::vec3 colDiffuse(0, 0, 0);
	glm::vec3 colSpecular(0, 0, 0);

	// Direction light
	if (bDirectionLightStatus)
	{
		colAmbient = glm::vec3(colDirectionLightAmbient.r(), colDirectionLightAmbient.g(), colDirectionLightAmbient.b());
		colDiffuse = glm::vec3(colDirectionLightDiffuse.r(), colDirectionLightDiffuse.g(), colDirectionLightDiffuse.b());
		colSpecular = glm::vec3(colDirectionLightSpecular.r(), colDirectionLightSpecular.g(), colDirectionLightSpecular.b());
	}
	shader.setVec3("colDirectionLightAmbient", colAmbient.x, colAmbient.y, colAmbient.z);
	shader.setVec3("colDirectionLightDiffuse", colDiffuse.x, colDiffuse.y, colDiffuse.z);
	shader.setVec3("colDirectionLightSpecular", colSpecular.x, colSpecular.y, colSpecular.z);

	// Reset to zeros
	colAmbient = glm::vec3(0, 0, 0);
	colDiffuse = glm::vec3(0, 0, 0);
	colSpecular = glm::vec3(0, 0, 0);

	// Point Light
	if (bPointLightStatus)
	{
		colAmbient = glm::vec3(colPointLightAmbient.r(), colPointLightAmbient.g(), colPointLightAmbient.b());
		colDiffuse = glm::vec3(colPointLightDiffuse.r(), colPointLightDiffuse.g(), colPointLightDiffuse.b());
		colSpecular = glm::vec3(colPointLightSpecular.r(), colPointLightSpecular.g(), colPointLightSpecular.b());
	}
	shader.setVec3("colPointLightAmbient", colAmbient.x, colAmbient.y, colAmbient.z);
	shader.setVec3("colPointLightDiffuse", colDiffuse.x, colDiffuse.y, colDiffuse.z);
	shader.setVec3("colPointLightSpecular", colSpecular.x, colSpecular.y, colSpecular.z);
	shader.setVec3("pointLightLoc", 1, &(m_pointLight->lightPos[0]));
}

/* Covers overhead for each time a model is loaded. */
void reloadObjectModel()
{
	m_object->Vertices.clear(); // Refresh vertex buffer.
	if (m_object->load_obj(Renderer::model_name))
	{
		setViewLoc(m_object->Min_Z, m_object->Max_Z);
		if (m_object->Vertices.size() > 0)
		{
			bind_vaovbo(*m_object);
		}
	}
}

/* bind VAO and VBO */
void bind_vaovbo(LocObject &cur_obj)
{
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	// Bind the Vertex Array Object
	glBindVertexArray(VAO);

	// Copy our vertices array in a buffer for OpenGL to use
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(LocObject::Vertex) * cur_obj.Vertices.size(), &(cur_obj.Vertices[0]), GL_STATIC_DRAW);

	// Set the vertex attributes pointers
	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LocObject::Vertex), (void *)0);
	glEnableVertexAttribArray(0);

	// Normal attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(LocObject::Vertex), (void *)offsetof(LocObject::Vertex, Normal));
	glEnableVertexAttribArray(1);

	// Unbind VAO
	glBindVertexArray(0);
}

/* Configures OpenGl vars for culling. */
void setCulling()
{
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	if (culling_val == culling_type::CCW)
	{
		glFrontFace(GL_CCW);
	}
	else
	{
		glFrontFace(GL_CW);
	}
}

/* Configures OpenGl vars for depth. */
void setDepth()
{
	if (depth_val == depth_type::Always)
	{
		glDisable(GL_DEPTH_TEST);
	}
	else
	{
		glEnable(GL_DEPTH_TEST);
	}
}

/*Configures OpenGl vars for render type.*/
void draw()
{
	if (render_val == render_type::Triangle)
	{
		glDrawArrays(GL_TRIANGLES, 0, m_object->Vertices.size());
	}
	else if (render_val == render_type::Point)
	{
		// glEnable(GL_PROGRAM_POINT_SIZE);
		glPointSize(2.0f); // set point size
		glDrawArrays(GL_POINTS, 0, m_object->Vertices.size());
	}
	else if (render_val == render_type::Line)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawArrays(GL_TRIANGLES, 0, m_object->Vertices.size());
	}
}

/* Centers the object in the screen when loaded. */
void setViewLoc(float min, float max)
{
	float center, dist;

	center = (max + min) / 2;
	dist = abs(max - min) * 2;

	Renderer::m_camera->setModelCenter(center, dist);

	// Round to 2 decimal places for gui
	dPositionX = 0;
	dPositionY = ((double)((int)(center * 100))) / 100;
	dPositionZ = ((double)((int)(((double)dist) * 100))) / 100;

	// Update gui handles
	gui_PositionX->setValue(dPositionX);
	gui_PositionY->setValue(dPositionY);
	gui_PositionZ->setValue(dPositionZ);
}

/* Initialize Nanogui settings */
void Renderer::nanogui_init(GLFWwindow *window)
{
	// Create a nanogui screen and pass the glfw pointer to initialize
	Renderer::m_nanogui_screen = new nanogui::Screen();
	Renderer::m_nanogui_screen->initialize(window, true);

	// Create nanogui gui
	bool enabled = true;
	nanogui::FormHelper *gui = new nanogui::FormHelper(Renderer::m_nanogui_screen);

	// Nanogui Control Bar_1
	nanogui::ref<nanogui::Window> nanoguiWindow = gui->addWindow(Eigen::Vector2i(10, 10), "Nanogui Control Bar_1"); // Gui Header

	// Set vars for camera position:
	gui->addGroup("Position");
	gui_PositionX = gui->addVariable("X", dPositionX);
	gui_PositionY = gui->addVariable("Y", dPositionY);
	gui_PositionZ = gui->addVariable("Z", dPositionZ);

	gui_PositionX->setSpinnable(true);
	gui_PositionY->setSpinnable(true);
	gui_PositionZ->setSpinnable(true);

	// Camera local rotation:
	gui->addGroup("Rotate");
	gui_RotValue = gui->addVariable("Rotate Value", dRotValue);
	gui_RotValue->setSpinnable(true);

	gui->addButton("Rotate Right+", []() { Renderer::m_camera->rotateByVal(Camera_Movement::Right_Pos, dRotValue); });
	gui->addButton("Rotate Right-", []() { Renderer::m_camera->rotateByVal(Camera_Movement::Right_Neg, dRotValue); });
	gui->addButton("Rotate Up+", []() { Renderer::m_camera->rotateByVal(Camera_Movement::Up_Pos, dRotValue); });
	gui->addButton("Rotate Up-", []() { Renderer::m_camera->rotateByVal(Camera_Movement::Up_Neg, dRotValue); });
	gui->addButton("Rotate Front+", []() { Renderer::m_camera->rotateByVal(Camera_Movement::Front_Pos, dRotValue); });
	gui->addButton("Rotate Front-", []() { Renderer::m_camera->rotateByVal(Camera_Movement::Front_Neg, dRotValue); });

	// Camera viewing plane & render settings:
	gui->addGroup("Configuration");
	gui_NearPlane = gui->addVariable("Z Near", dNearPlane);
	gui_FarPlane = gui->addVariable("Z Far", dFarPlane);

	gui_NearPlane->setSpinnable(true);
	gui_FarPlane->setSpinnable(true);

	gui_RenderType = gui->addVariable("Render Type", render_val, enabled);
	gui_RenderType->setItems({"Point", "Line", "Triangle"});

	gui_CullingType = gui->addVariable("Culling Type", culling_val, enabled);
	gui_CullingType->setItems({"CW", "CCW"});

	gui_ShadingType = gui->addVariable("Shading Type", shading_val, enabled);
	gui_ShadingType->setItems({"Smooth", "Flat"});

	gui_DepthType = gui->addVariable("Depth Type", depth_val, enabled);
	gui_DepthType->setItems({"Less", "Always"});

	gui_ObjectFile = gui->addVariable("Model Name", Renderer::model_name);

	gui->addButton("Reload Model", &reloadObjectModel);
	gui->addButton("Reset Camera", &reset);

	// Callbacks to set global variables when changed in the gui:
	gui_ObjectFile->setCallback([](const std::string &str) { Renderer::model_name = str; });
	gui_RotValue->setCallback([](double val) { dRotValue = val; });
	gui_PositionX->setCallback([](double val) { dPositionX = val; Renderer::m_camera->localPos.x = dPositionX; m_pointLight->curCamPos.x = dPositionX; });
	gui_PositionY->setCallback([](double val) { dPositionY = val; Renderer::m_camera->localPos.y = dPositionY; m_pointLight->curCamPos.y = dPositionY; });
	gui_PositionZ->setCallback([](double val) { dPositionZ = val; Renderer::m_camera->localPos.z = dPositionZ; m_pointLight->curCamPos.z = dPositionZ; });
	gui_FarPlane->setCallback([](double val) { dFarPlane = val; Renderer::m_camera->farPlane = dFarPlane; });
	gui_NearPlane->setCallback([](double val) { dNearPlane = val; Renderer::m_camera->nearPlane = dNearPlane; });
	gui_RenderType->setCallback([](const render_type &val) { render_val = val; });
	gui_CullingType->setCallback([](const culling_type &val) {culling_val = val; setCulling(); });

	gui_ShadingType->setCallback([](const shading_type &val) { shading_val = val; });
	gui_DepthType->setCallback([](const depth_type &val) { depth_val = val; });

	// Nanogui Control Bar_2
	nanogui::ref<nanogui::Window> nanoguiWindow_2 = gui->addWindow(Eigen::Vector2i(270, 10), "Nanogui Control Bar_2"); // Gui Header

	// Object color:
	gui->addGroup("Lighting");
	gui_ColObject = gui->addVariable("Object Color", colObject);
	// Shininess
	gui_ObjectShininess = gui->addVariable("Object Shininess", fObjectShininess);
	// Direction
	gui_DirectionLightStatus = gui->addVariable("Direction Light Status", bDirectionLightStatus);
	gui_DirectionLightAmbientColor = gui->addVariable("Direction Light Ambient Color", colDirectionLightAmbient);
	gui_DirectionLightDiffuseColor = gui->addVariable("Direction Light Diffuse Color", colDirectionLightDiffuse);
	gui_DirectionLightSpecularColor = gui->addVariable("Direction Light Specular Color", colDirectionLightSpecular);
	// Point
	gui_PointLightStatus = gui->addVariable("Point Light Status", bPointLightStatus);
	gui_PointLightAmbientColor = gui->addVariable("Point Light Ambient Color", colPointLightAmbient);
	gui_PointLightDiffuseColor = gui->addVariable("Point Light Diffuse Color", colPointLightDiffuse);
	gui_PointLightSpecularColor = gui->addVariable("Point Light Specular Color", colPointLightSpecular);
	// Point Light Rotation
	gui_PointLightRotateX = gui->addVariable("Point Light Rotate on X", bPointLightRotateX);
	gui_PointLightRotateY = gui->addVariable("Point Light Rotate on Y", bPointLightRotateY);
	gui_PointLightRotateZ = gui->addVariable("Point Light Rotate on Z", bPointLightRotateZ);
	// Reset Point Light
	gui->addButton("Reset Point Light", &resetPointLight);

	// Set Callbacks
	gui_ColObject->setCallback([](const nanogui::Color &c) { colObject = c; });
	// gui_RotValue->setCallback([](double val) { dRotValue = val; });
	gui_ObjectShininess->setCallback([](float val) { fObjectShininess = val; });
	gui_DirectionLightStatus->setCallback([](bool b) { bDirectionLightStatus = b; });
	gui_DirectionLightAmbientColor->setCallback([](const nanogui::Color &c) { colDirectionLightAmbient = c; });
	gui_DirectionLightDiffuseColor->setCallback([](const nanogui::Color &c) { colDirectionLightDiffuse = c; });
	gui_DirectionLightSpecularColor->setCallback([](const nanogui::Color &c) { colDirectionLightSpecular = c; });
	gui_PointLightStatus->setCallback([](bool b) { bPointLightStatus = b; });
	gui_PointLightAmbientColor->setCallback([](const nanogui::Color &c) { colPointLightAmbient = c; });
	gui_PointLightDiffuseColor->setCallback([](const nanogui::Color &c) { colPointLightDiffuse = c; });
	gui_PointLightSpecularColor->setCallback([](const nanogui::Color &c) { colPointLightSpecular = c; });
	gui_PointLightRotateX->setCallback([](bool b) { bPointLightRotateX = b; });
	gui_PointLightRotateY->setCallback([](bool b) { bPointLightRotateY = b; });
	gui_PointLightRotateZ->setCallback([](bool b) { bPointLightRotateZ = b; });

	// Init screen
	Renderer::m_nanogui_screen->setVisible(true);
	Renderer::m_nanogui_screen->performLayout();

	// Set additional callbacks:
	glfwSetCursorPosCallback(window,
							 [](GLFWwindow *, double x, double y) {
								 Renderer::m_nanogui_screen->cursorPosCallbackEvent(x, y);
							 });

	glfwSetMouseButtonCallback(window,
							   [](GLFWwindow *, int button, int action, int modifiers) {
								   Renderer::m_nanogui_screen->mouseButtonCallbackEvent(button, action, modifiers);
							   });

	glfwSetKeyCallback(window,
					   [](GLFWwindow *, int key, int scancode, int action, int mods) {
						   Renderer::m_nanogui_screen->keyCallbackEvent(key, scancode, action, mods);
					   });

	glfwSetCharCallback(window,
						[](GLFWwindow *, unsigned int codepoint) {
							Renderer::m_nanogui_screen->charCallbackEvent(codepoint);
						});

	glfwSetDropCallback(window,
						[](GLFWwindow *, int count, const char **filenames) {
							Renderer::m_nanogui_screen->dropCallbackEvent(count, filenames);
						});

	glfwSetScrollCallback(window,
						  [](GLFWwindow *, double x, double y) {
							  Renderer::m_nanogui_screen->scrollCallbackEvent(x, y);
						  });

	glfwSetFramebufferSizeCallback(window,
								   [](GLFWwindow *, int width, int height) {
									   Renderer::m_nanogui_screen->resizeCallbackEvent(width, height);
								   });
}

/* Reset the camera to its original position  */
void reset()
{
	Renderer::m_camera->reset();
	setViewLoc(m_object->Min_Z, m_object->Max_Z);
}

/* Reset point light */
void resetPointLight()
{
	// set point light position to camera position
	m_pointLight->reset();

	// Set rotate flags to false
	bPointLightRotateX = false;
	bPointLightRotateY = false;
	bPointLightRotateZ = false;

	// Update gui handles
	gui_PointLightRotateX->setValue(bPointLightRotateX);
	gui_PointLightRotateY->setValue(bPointLightRotateY);
	gui_PointLightRotateZ->setValue(bPointLightRotateZ);
}
