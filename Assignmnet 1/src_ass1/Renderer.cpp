#include "Renderer.h"

// Pre-define gui variable handles:
#define GUI_DOUBLE      nanogui::detail::FormWidget<double, std::integral_constant<bool, true>>
#define GUI_STRING      nanogui::detail::FormWidget<std::string, std::true_type>
#define GUI_COLOR       nanogui::detail::FormWidget<nanogui::Color, std::true_type>

Camera* Renderer::m_camera = new Camera(); 
GLFWwindow* Renderer::m_window = nullptr;
nanogui::Screen* Renderer::m_nanogui_screen = nullptr;
std::string Renderer::model_name = "../src/objs/cyborg.obj";
LocObject* m_object = new LocObject(Renderer::model_name);
// LocObject* m_object = nullptr;

enum render_type {
	Point = 0,
	Line = 1,
	Triangle = 2
};

enum culling_type {
	CW = 0,
	CCW = 1
};

#define GUI_RENDERTYPE  nanogui::detail::FormWidget<render_type, std::integral_constant<bool, true>>
#define GUI_CULLINGTYPE nanogui::detail::FormWidget<culling_type, std::integral_constant<bool, true>>

// Gui variable handles to edit fields in the gui:
GUI_RENDERTYPE*  gui_RenderType;
GUI_CULLINGTYPE* gui_CullingType;
GUI_COLOR*  gui_ColObject;
GUI_STRING* gui_ObjectFile;
GUI_DOUBLE* gui_RotValue;
GUI_DOUBLE* gui_PositionX;
GUI_DOUBLE* gui_PositionY;
GUI_DOUBLE* gui_PositionZ;
GUI_DOUBLE* gui_NearPlane;
GUI_DOUBLE* gui_FarPlane;

// Variable defaults modified in the gui:
nanogui::Color colObject(0.5f, 0.5f, 0.7f, 1.f); // color of the object
double dPositionX = 0.0;
double dPositionY = 0.0;
double dPositionZ = -10.0;
double dRotValue = 0.0;
double dNearPlane = Renderer::m_camera->NEAR_PLANE;
double dFarPlane = Renderer::m_camera->FAR_PLANE;
render_type render_val = render_type::Line;
culling_type culling_val = culling_type::CW;

GLuint VBO, VAO;

//function foward declarations
void SetColor();
void setCulling();
void setViewLoc(float min, float max);
void reset();
void reloadObjectModel();

Renderer::Renderer() {}
Renderer::~Renderer() {}

void Renderer::run() {
	init();
	display(Renderer::m_window);
}

void Renderer::init() {
    // Init GLFW
	glfwInit();
	// Set all the required options for GLFW
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
	#if defined(__APPLE__)
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	#endif

	// Create a GLFWwindow object
	Renderer::m_window = glfwCreateWindow(Renderer::m_camera->width, Renderer::m_camera->height, "CS6366 Assignment 1", nullptr, nullptr);
	glfwMakeContextCurrent(Renderer::m_window);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	nanogui_init(Renderer::m_window);

	glewExperimental = GL_TRUE;
	glewInit();
}

void Renderer::display(GLFWwindow *window) {
    // Build and compile our shader program
	Shader ourShader("../src/shader/basic.vert", "../src/shader/basic.frag");
    reloadObjectModel();
	GLuint MatrixID = glGetUniformLocation(ourShader.program, "MVP");

    // Main frame while loop
    while (!glfwWindowShouldClose(Renderer::m_window)) {
        // Specify culling variables.
        setCulling(); 
		glfwPollEvents();
        // Clear the colorbuffer
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
        // Draw the triangle
		ourShader.use();
		glBindVertexArray(VAO);
        if (m_object->Vertices.size() > 0) {
			glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &(Renderer::m_camera->getMVPMatrix()[0][0]));
			
			// Set draw type:
			if (render_val == render_type::Triangle) {
				glDrawArrays(GL_TRIANGLES, 0, m_object->Vertices.size());
			}
			else if (render_val == render_type::Point) {
				glEnable(GL_PROGRAM_POINT_SIZE);
				glDrawArrays(GL_POINTS, 0, m_object->Vertices.size());
			}
			else if (render_val == render_type::Line) {
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				glDrawArrays(GL_TRIANGLES, 0, m_object->Vertices.size());
			}
		}
        glBindVertexArray(0);
        // Set draw mode back to fill & draw gui
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		Renderer::m_nanogui_screen->drawWidgets();
		// Swap the screen buffers
		glfwSwapBuffers(Renderer::m_window);
    }
    // De-allocate all resources once they've outlived their purpose
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	// Terminate GLFW, clearing any resources allocated by GLFW.
	glfwTerminate();
}

/* Covers overhead for each time a model is loaded. */
void reloadObjectModel() {
	m_object->Vertices.clear(); // Refresh vertex buffer.
	if(m_object->load_obj(Renderer::model_name)) {
		setViewLoc(m_object->Min_Z, m_object->Max_Z);
		SetColor();
	}
}

/* bind VAO and VBO */
void bind_vaovbo(LocObject &cur_obj) {
    glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	// Bind the Vertex Array Object first, then bind and set vertex buffer(s) and attribute pointer(s).
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(LocObject::Vertex) * cur_obj.Vertices.size(), &(cur_obj.Vertices[0]), GL_STATIC_DRAW);

	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LocObject::Vertex), (void*)0);
	glEnableVertexAttribArray(0);

	// Color attribute
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(LocObject::Vertex), (void*)offsetof(LocObject::Vertex, Color));

	glBindVertexArray(0); // Unbind VAO
}

/* Set the color value for each vertex. */
void SetColor() {
	for (int i = 0; i < m_object->Vertices.size(); i++) {
		m_object->Vertices[i].Color = glm::vec3(colObject.r(), colObject.g(), colObject.b());
	}
	if (m_object->Vertices.size() > 0) {
        bind_vaovbo(*m_object);
    }
}

/* Configures OpenGl vars for culling. */
void setCulling() {
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	if (culling_val == culling_type::CCW) {
		glFrontFace(GL_CCW);
	} else {
		glFrontFace(GL_CW);
	}
}

/* Centers the object in the screen when loaded. */
void setViewLoc(float min, float max) {
	float center, dist;

	center = (max + min) / 2;
	dist = abs(max - min) * 2;
	
	Renderer::m_camera->setModelCenter(center, dist);

	// Round to 2 decimal places for gui 
	dPositionX = 0;
	dPositionY = ((double)((int)(center*100))) /100;
	dPositionZ = ((double)((int)((-1 * (double)dist) * 100))) / 100;

	// Update gui handles:
	gui_PositionX->setValue(dPositionX);
	gui_PositionY->setValue(dPositionY);
	gui_PositionZ->setValue(dPositionZ);
}

/* Initialize Nanogui settings */
void Renderer::nanogui_init(GLFWwindow* window) {
	// Create a nanogui screen and pass the glfw pointer to initialize
	Renderer::m_nanogui_screen = new nanogui::Screen();
	Renderer::m_nanogui_screen->initialize(window, true);

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);
	glfwSwapInterval(0);
	glfwSwapBuffers(window);

	// Create nanogui gui
	bool enabled = true;
	nanogui::FormHelper *gui = new nanogui::FormHelper(Renderer::m_nanogui_screen);

	nanogui::ref<nanogui::Window> nanoguiWindow = gui->addWindow(Eigen::Vector2i(10, 10), "Nanogui Control Bar"); // Gui Header

	// Object color:
	gui->addGroup("Color");
	gui_ColObject = gui->addVariable("Object Color", colObject);

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
	gui_RenderType->setItems({ "Point", "Line", "Triangle" });

	gui_CullingType = gui->addVariable("Culling Type", culling_val, enabled);
	gui_CullingType->setItems({ "CW", "CCW" });

	gui_ObjectFile = gui->addVariable("Model Name", Renderer::model_name);

	gui->addButton("Reload Model", &reloadObjectModel);
	gui->addButton("Reset", &reset);

	// Callbacks to set global variables when changed in the gui:
	gui_ObjectFile->setCallback([](const std::string &str) { Renderer::model_name = str; });
	gui_RotValue->setCallback([](double val) { dRotValue = val; });
	gui_ColObject->setFinalCallback([](const nanogui::Color &c) { colObject = c; SetColor(); });
	gui_PositionX->setCallback([](double val) { dPositionX = val; Renderer::m_camera->localPos.x = dPositionX; });
	gui_PositionY->setCallback([](double val) { dPositionY = val; Renderer::m_camera->localPos.y = dPositionY; });
	gui_PositionZ->setCallback([](double val) { dPositionZ = val; Renderer::m_camera->localPos.z = dPositionZ; });
	gui_FarPlane->setCallback([](double val) { dFarPlane = val; Renderer::m_camera->farPlane = dFarPlane; });
	gui_NearPlane->setCallback([](double val) { dNearPlane = val; Renderer::m_camera->nearPlane = dNearPlane; });
	gui_RenderType->setCallback([](const render_type &val) {render_val = val;});
	gui_CullingType->setCallback([](const culling_type &val) {culling_val = val; setCulling(); });

	// Init screen:
	Renderer::m_nanogui_screen->setVisible(true);
	Renderer::m_nanogui_screen->performLayout();

	// Set additional callbacks:

	glfwSetCursorPosCallback(window,
		[](GLFWwindow *, double x, double y) {
		Renderer::m_nanogui_screen->cursorPosCallbackEvent(x, y);
	}
	);

	glfwSetMouseButtonCallback(window,
		[](GLFWwindow *, int button, int action, int modifiers) {
		Renderer::m_nanogui_screen->mouseButtonCallbackEvent(button, action, modifiers);
	}
	);

	glfwSetKeyCallback(window,
		[](GLFWwindow *, int key, int scancode, int action, int mods) {
		Renderer::m_nanogui_screen->keyCallbackEvent(key, scancode, action, mods);
	}
	);

	glfwSetCharCallback(window,
		[](GLFWwindow *, unsigned int codepoint) {
		Renderer::m_nanogui_screen->charCallbackEvent(codepoint);
	}
	);

	glfwSetDropCallback(window,
		[](GLFWwindow *, int count, const char **filenames) {
		Renderer::m_nanogui_screen->dropCallbackEvent(count, filenames);
	}
	);

	glfwSetScrollCallback(window,
		[](GLFWwindow *, double x, double y) {
		Renderer::m_nanogui_screen->scrollCallbackEvent(x, y);
	}
	);

	glfwSetFramebufferSizeCallback(window,
		[](GLFWwindow *, int width, int height) {
		Renderer::m_nanogui_screen->resizeCallbackEvent(width, height);
	}
	);
}

/* Reset the camera to its original position  */
void reset() {
    Renderer::m_camera->reset();
    setViewLoc(m_object->Min_Z, m_object->Max_Z);
}
