#include "Renderer.h"

// Pre-define gui variable handles:
#define GUI_DOUBLE nanogui::detail::FormWidget<double, std::integral_constant<bool, true> >
#define GUI_STRING nanogui::detail::FormWidget<std::string, std::true_type>
#define GUI_COLOR nanogui::detail::FormWidget<nanogui::Color, std::true_type>
#define GUI_INT nanogui::detail::FormWidget<int, std::integral_constant<bool, true> >
#define GUI_BOOL nanogui::detail::FormWidget<bool, std::integral_constant<bool, true> >

Camera *Renderer::m_camera = new Camera();
GLFWwindow *Renderer::m_window = nullptr;
nanogui::Screen *Renderer::m_nanogui_screen = nullptr;

enum render_type
{
	Point = 0,
	Line = 1,
	Triangle = 2
};

enum model_type
{
	Bonsai = 0,
	Teapot = 1,
	Bucky = 2,
	Head = 3
};

//unit cube vertices => changed to vec3 for convenience
glm::vec3 cube_vertices[8] = {glm::vec3(0.0, 0.0, 0.0),
							  glm::vec3(1.0, 0.0, 0.0),
							  glm::vec3(1.0, 1.0, 0.0),
							  glm::vec3(0.0, 1.0, 0.0),
							  glm::vec3(0.0, 0.0, 1.0),
							  glm::vec3(1.0, 0.0, 1.0),
							  glm::vec3(1.0, 1.0, 1.0),
							  glm::vec3(0.0, 1.0, 1.0)};

//edge lookup table
int edgeTable[8][12] = {
	{0, 1, 5, 6, 4, 8, 11, 9, 3, 7, 2, 10}, // v0 is front
	{0, 4, 3, 11, 1, 2, 6, 7, 5, 9, 8, 10}, // v1 is front
	{1, 5, 0, 8, 2, 3, 7, 4, 6, 10, 9, 11}, // v2 is front
	{7, 11, 10, 8, 2, 6, 1, 9, 3, 0, 4, 5}, // v3 is front
	{8, 5, 9, 1, 11, 10, 7, 6, 4, 3, 0, 2}, // v4 is front
	{9, 6, 10, 2, 8, 11, 4, 7, 5, 0, 1, 3}, // v5 is front
	{9, 8, 5, 4, 6, 1, 2, 0, 10, 7, 11, 3}, // v6 is front
	{10, 9, 6, 5, 7, 2, 3, 1, 11, 4, 8, 0}	// v7 is front
};

const int cube_edges[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {0, 4}, {1, 5}, {2, 6}, {3, 7}, {4, 5}, {5, 6}, {6, 7}, {7, 4}};

//transfer function lookup table of color values
glm::vec4 lookupTable[8];

//for floating point inaccuracy
const float EPSILON = 0.0001f;
//current viewing direction
glm::vec3 viewDir = glm::vec3(0.0, 0.0, 4.0);
// glm::vec3 viewDir = glm::vec3(1, 0, 1);

#define GUI_RENDERTYPE nanogui::detail::FormWidget<render_type, std::integral_constant<bool, true> >
#define GUI_MODELTYPE nanogui::detail::FormWidget<model_type, std::integral_constant<bool, true> >

// Gui variable handles to edit fields in the gui:
GUI_RENDERTYPE *gui_RenderType;
GUI_COLOR *gui_ColObject;
GUI_STRING *gui_ObjectFile;
GUI_DOUBLE *gui_RotValue;
GUI_DOUBLE *gui_PositionX;
GUI_DOUBLE *gui_PositionY;
GUI_DOUBLE *gui_PositionZ;
// GUI_DOUBLE *gui_NearPlane;
// GUI_DOUBLE *gui_FarPlane;

GUI_MODELTYPE *gui_ModelType;
GUI_BOOL *gui_TransferFunctionSign;
GUI_INT *gui_SamplingRate;

// Variable defaults modified in the gui:
nanogui::Color colObject(0.5f, 0.5f, 0.7f, 1.f); // color of the object
double dPositionX = 0.0;
double dPositionY = 0.0;
double dPositionZ = 4.0;
double dRotValue = 0.0;
// double dNearPlane = Renderer::m_camera->NEAR_PLANE;
// double dFarPlane = Renderer::m_camera->FAR_PLANE;
render_type render_val = render_type::Line;
model_type model_val = model_type::Teapot;

std::string teapotPath = "../src/objs/BostonTeapot_256_256_178.raw";
std::string bonsaiPath = "../src/objs/Bonsai_512_512_154.raw";
std::string buckyPath = "../src/objs/Bucky_32_32_32.raw";
std::string headPath = "../src/objs/Head_256_256_225.raw";
std::string modelPath = teapotPath;				//initialized as teapot's path
glm::vec3 dimension = glm::vec3(256, 256, 178); //initialized as teapot's dimension

std::string colorBarPath = "../src/objs/colorbar.png";

bool bTransferFunctionSign = false;
int iSamplingRate = 10; //range[3, 512](when sampling rate is greater than 512, the display won't change much)

double opacityCorrection = pow(0.5, 10.0); //A_0 = 0.5, S_0 = 10, (1-A_0)^(S_0) = 0.5^10
double curAlpha;

float fViewSlider = 1.0;
const int SLIDER_NUM = 8;
float fSlider[SLIDER_NUM] = {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};

//maximum number of slices
const int MAX_SLICES = 512;
//sliced vertices
glm::vec3 vTextureSlices[MAX_SLICES * 12];

GLuint VBO, VAO;

GLuint volTexture;

GLuint tfTexture;

//function foward declarations
void reset();
void reloadObjectModel();
void bind_vaovbo();
void setModel();
GLubyte *load_3d_raw_data(std::string texture_path, glm::vec3 dimension);
int findAbsoluteMax(glm::vec3 v);
void volumeSlicing();
void setSamplingRate();
void loadTexture(GLubyte *data);
void loadTransferFunction();
void setGraph(Eigen::VectorXf &func);
void loadColorBar(std::string colorBarPath);

Renderer::Renderer() {}
Renderer::~Renderer() {}

void Renderer::run()
{
	init();
	display(Renderer::m_window);
}

void Renderer::init()
{
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
	Renderer::m_window = glfwCreateWindow(Renderer::m_camera->width, Renderer::m_camera->height, "CS6366 Assignment 4", nullptr, nullptr);

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
	loadColorBar(colorBarPath);

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

	// Build and compile our shader program
	Shader ourShader("../src/shader/basic.vert", "../src/shader/basic.frag");

	ourShader.use();
	ourShader.setInt("volTexture", 0);
	ourShader.setInt("tfTexture", 1);

	GLubyte *data = load_3d_raw_data(modelPath, dimension);
	loadTexture(data);

	//load the transfer function data and generate the trasnfer function (lookup table) texture
	loadTransferFunction();

	bind_vaovbo();

	//slice the volume dataset initially
	volumeSlicing();

	// Main frame while loop
	while (!glfwWindowShouldClose(Renderer::m_window))
	{
		// Check if any events are triggered
		glfwPollEvents();

		// Clear the colorbuffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		//bind volume vertex array object
		glBindVertexArray(VAO);

		ourShader.use();

		// Set uniform values of shader
		setup_uniform_values(ourShader);

		// Set draw type:
		if (render_val == render_type::Triangle)
		{
			glDrawArrays(GL_TRIANGLES, 0, sizeof(vTextureSlices) / sizeof(vTextureSlices[0]));
		}
		else if (render_val == render_type::Point)
		{
			glPointSize(2.0f); // set point size
			glDrawArrays(GL_POINTS, 0, sizeof(vTextureSlices) / sizeof(vTextureSlices[0]));
		}
		else if (render_val == render_type::Line)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glDrawArrays(GL_TRIANGLES, 0, sizeof(vTextureSlices) / sizeof(vTextureSlices[0]));
		}

		//disable blending
		glDisable(GL_BLEND);

		// Set draw mode back to fill & draw gui
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		Renderer::m_nanogui_screen->drawWidgets();

		// Swap the screen buffers
		glfwSwapBuffers(Renderer::m_window);
	}

	// De-allocate all resources once they've outlived their purpose
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteTextures(1, &volTexture);
	glDeleteTextures(1, &tfTexture);
	ourShader.del();

	// Terminate GLFW, clearing any resources allocated by GLFW.
	glfwTerminate();
}

/* Setup the uniform values of the shader*/
void Renderer::setup_uniform_values(Shader &shader)
{
	shader.setMatrix4fv("MVP", 1, GL_FALSE, &(Renderer::m_camera->getMVPMatrix()[0][0]));
	shader.setVec3("colObject", colObject.r(), colObject.g(), colObject.b());
	shader.setBool("bTransferFunctionSign", bTransferFunctionSign);
	// shader.setVec3("camPos", 1, &(Renderer::m_camera->getCamPos()[0]));
	// shader.setVec3("inputDirectionLightDir", dDirectionLightX, dDirectionLightY, dDirectionLightZ);
}

/* Covers overhead for each time a model is loaded. */
void reloadObjectModel()
{
	//bind_vaovbo();
	//SliceVolume();
}

/* bind VAO and VBO */
void bind_vaovbo()
{
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	// Bind Vertex Array
	glBindVertexArray(VAO);

	// Bind VBO to GL_ARRAY_BUFFER type so that all calls to GL_ARRAY_BUFFER use VBO
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	// Upload vertices to VBO
	glBufferData(GL_ARRAY_BUFFER, sizeof(vTextureSlices), 0, GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);

	std::cout << "Bind VAO & VBO successfully!" << std::endl;
}

/* function to get the max (abs) dimension of the given vertex */
int findAbsoluteMax(glm::vec3 vertex)
{
	vertex = glm::abs(vertex);
	int max_dimension = 0;
	float val = vertex.x;
	if (vertex.y > val)
	{
		val = vertex.y;
		max_dimension = 1;
	}
	if (vertex.z > val)
	{
		val = vertex.z;
		max_dimension = 2;
	}
	return max_dimension;
}

/* main slicing function */
void volumeSlicing()
{
	//clear vTextureSlices!!!
	for (int i = 0; i < MAX_SLICES * 12; i++)
	{
		vTextureSlices[i] = glm::vec3(0, 0, 0);
	}
	//normalize viewDir!!!
	viewDir = glm::normalize(viewDir);

	//get the max and min distance of each vertex of the unit cube
	//in the viewing direction
	float max_dist = glm::dot(viewDir, cube_vertices[0]);
	float min_dist = max_dist;
	int max_index = 0;
	int count = 0;

	for (int i = 1; i < 8; i++)
	{
		//get the distance between the current unit cube vertex and
		//the view vector by dot product
		float dist = glm::dot(viewDir, cube_vertices[i]);

		//if distance is > max_dist, store the value and index
		if (dist > max_dist)
		{
			max_dist = dist;
			max_index = i;
		}

		//if distance is < min_dist, store the value
		if (dist < min_dist)
			min_dist = dist;
	}

	//find tha absolute maximum of the view direction vector
	int max_dim = findAbsoluteMax(viewDir);

	//expand it a little bit
	min_dist -= EPSILON;
	max_dist += EPSILON;

	//local variables to store the start, direction vectors,
	//lambda intersection values
	glm::vec3 vecStart[12];
	glm::vec3 vecDir[12];
	float lambda[12];
	float lambda_inc[12];
	float denom = 0;

	//set the minimum distance as the plane_dist
	//subtract the max and min distances and divide by the
	//total number of slices to get the plane increment
	float plane_dist = min_dist;
	float plane_dist_inc = (max_dist - min_dist) / float(iSamplingRate);

	//for all edges
	for (int i = 0; i < 12; i++)
	{
		//get the start position vertex by table lookup
		vecStart[i] = cube_vertices[cube_edges[edgeTable[max_index][i]][0]];

		//get the direction by table lookup
		vecDir[i] = cube_vertices[cube_edges[edgeTable[max_index][i]][1]] - vecStart[i];

		//do a dot of vecDir with the view direction vector
		denom = glm::dot(vecDir[i], viewDir);

		//determine the plane intersection parameter (lambda) and
		//plane intersection parameter increment (lambda_inc)
		if (1.0 + denom != 1.0)
		{
			lambda_inc[i] = plane_dist_inc / denom;
			lambda[i] = (plane_dist - glm::dot(vecStart[i], viewDir)) / denom;
		}
		else
		{
			lambda[i] = -1.0;
			lambda_inc[i] = 0.0;
		}
	}

	//local variables to store the intesected points
	//note that for a plane and sub intersection, we can have
	//a minimum of 3 and a maximum of 6 vertex polygon
	glm::vec3 intersection[6];
	float dL[12];

	int end = iSamplingRate * (1.0 - fViewSlider);

	//loop through all slices
	for (int i = iSamplingRate - 1; i >= end; i--)
	{

		//determine the lambda value for all edges
		for (int e = 0; e < 12; e++)
		{
			dL[e] = lambda[e] + i * lambda_inc[e];
		}

		//if the values are between 0-1, we have an intersection at the current edge
		//repeat the same for all 12 edges
		if ((dL[0] >= 0.0) && (dL[0] < 1.0))
		{
			intersection[0] = vecStart[0] + dL[0] * vecDir[0];
		}
		else if ((dL[1] >= 0.0) && (dL[1] < 1.0))
		{
			intersection[0] = vecStart[1] + dL[1] * vecDir[1];
		}
		else if ((dL[3] >= 0.0) && (dL[3] < 1.0))
		{
			intersection[0] = vecStart[3] + dL[3] * vecDir[3];
		}
		else
			continue;

		if ((dL[2] >= 0.0) && (dL[2] < 1.0))
		{
			intersection[1] = vecStart[2] + dL[2] * vecDir[2];
		}
		else if ((dL[0] >= 0.0) && (dL[0] < 1.0))
		{
			intersection[1] = vecStart[0] + dL[0] * vecDir[0];
		}
		else if ((dL[1] >= 0.0) && (dL[1] < 1.0))
		{
			intersection[1] = vecStart[1] + dL[1] * vecDir[1];
		}
		else
		{
			intersection[1] = vecStart[3] + dL[3] * vecDir[3];
		}

		if ((dL[4] >= 0.0) && (dL[4] < 1.0))
		{
			intersection[2] = vecStart[4] + dL[4] * vecDir[4];
		}
		else if ((dL[5] >= 0.0) && (dL[5] < 1.0))
		{
			intersection[2] = vecStart[5] + dL[5] * vecDir[5];
		}
		else
		{
			intersection[2] = vecStart[7] + dL[7] * vecDir[7];
		}
		if ((dL[6] >= 0.0) && (dL[6] < 1.0))
		{
			intersection[3] = vecStart[6] + dL[6] * vecDir[6];
		}
		else if ((dL[4] >= 0.0) && (dL[4] < 1.0))
		{
			intersection[3] = vecStart[4] + dL[4] * vecDir[4];
		}
		else if ((dL[5] >= 0.0) && (dL[5] < 1.0))
		{
			intersection[3] = vecStart[5] + dL[5] * vecDir[5];
		}
		else
		{
			intersection[3] = vecStart[7] + dL[7] * vecDir[7];
		}
		if ((dL[8] >= 0.0) && (dL[8] < 1.0))
		{
			intersection[4] = vecStart[8] + dL[8] * vecDir[8];
		}
		else if ((dL[9] >= 0.0) && (dL[9] < 1.0))
		{
			intersection[4] = vecStart[9] + dL[9] * vecDir[9];
		}
		else
		{
			intersection[4] = vecStart[11] + dL[11] * vecDir[11];
		}

		if ((dL[10] >= 0.0) && (dL[10] < 1.0))
		{
			intersection[5] = vecStart[10] + dL[10] * vecDir[10];
		}
		else if ((dL[8] >= 0.0) && (dL[8] < 1.0))
		{
			intersection[5] = vecStart[8] + dL[8] * vecDir[8];
		}
		else if ((dL[9] >= 0.0) && (dL[9] < 1.0))
		{
			intersection[5] = vecStart[9] + dL[9] * vecDir[9];
		}
		else
		{
			intersection[5] = vecStart[11] + dL[11] * vecDir[11];
		}

		//after all 6 possible intersection vertices are obtained,
		//we calculated the proper polygon indices by using indices of a triangular fan
		int indices[] = {0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 5};

		//Using the indices, pass the intersection vertices to the vTextureSlices vector
		for (int i = 0; i < 12; i++)
			vTextureSlices[count++] = intersection[indices[i]];
	}

	//update buffer object with the new vertices
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vTextureSlices), &(vTextureSlices[0].x));
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

	// nanogui::ref<nanogui::Window>
	nanogui::Window *nanoguiWindow = gui->addWindow(Eigen::Vector2i(10, 10), "Nanogui Control Bar_1"); // Gui Header

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
	// gui_NearPlane = gui->addVariable("Z Near", dNearPlane);
	// gui_FarPlane = gui->addVariable("Z Far", dFarPlane);

	// gui_NearPlane->setSpinnable(true);
	// gui_FarPlane->setSpinnable(true);

	gui_RenderType = gui->addVariable("Render Type", render_val, enabled);
	gui_RenderType->setItems({"Point", "Line", "Triangle"});

	gui_ObjectFile = gui->addVariable("Colorbar image path", colorBarPath);

	gui->addButton("Reload Model", &reloadObjectModel);
	gui->addButton("Reset", &reset);

	// Object color:
	gui->addGroup("Volume Rendering");
	gui_ColObject = gui->addVariable("Object Color", colObject);
	gui_ModelType = gui->addVariable("Model name", model_val, enabled);
	gui_ModelType->setItems({"Bonsai", "Teapot", "Bucky", "Head"});
	gui_TransferFunctionSign = gui->addVariable("Transfer function sign", bTransferFunctionSign);
	gui_SamplingRate = gui->addVariable("Sampling rate", iSamplingRate);

	// Callbacks to set global variables when changed in the gui:
	//gui_ObjectFile->setCallback([](const std::string &str) { Renderer::model_name = str; });
	gui_RotValue->setCallback([](double val) { dRotValue = val; });
	gui_ColObject->setFinalCallback([](const nanogui::Color &c) { colObject = c; });
	gui_PositionX->setCallback([](double val) {
		dPositionX = val;
		Renderer::m_camera->localPos.x = dPositionX;
		//viewDir.x = dPositionX;
		viewDir = glm::vec3(dPositionX, dPositionY, dPositionZ);
		volumeSlicing();
	});
	gui_PositionY->setCallback([](double val) {
		dPositionY = val;
		Renderer::m_camera->localPos.y = dPositionY;
		//viewDir.y = dPositionY;
		viewDir = glm::vec3(dPositionX, dPositionY, dPositionZ);
		volumeSlicing();
	});
	gui_PositionZ->setCallback([](double val) {
		dPositionZ = val;
		Renderer::m_camera->localPos.z = dPositionZ;
		//viewDir.z = dPositionZ;
		viewDir = glm::vec3(dPositionX, dPositionY, dPositionZ);
		volumeSlicing();
	});
	// gui_FarPlane->setCallback([](double val) { dFarPlane = val; Renderer::m_camera->farPlane = dFarPlane; });
	// gui_NearPlane->setCallback([](double val) { dNearPlane = val; Renderer::m_camera->nearPlane = dNearPlane; });
	gui_RenderType->setCallback([](const render_type &val) { render_val = val; });

	gui_ModelType->setCallback([](const model_type &val) { model_val = val; setModel(); });
	gui_TransferFunctionSign->setCallback([](bool b) { bTransferFunctionSign = b; });
	gui_SamplingRate->setCallback([](int val) { iSamplingRate = val; setSamplingRate(); });
	gui_SamplingRate->setSpinnable(true);

	// Nanogui Control Bar_2
	nanogui::Window *nanoguiWindow_2 = gui->addWindow(Eigen::Vector2i(320, 10), "Nanogui Control Bar_2");
	nanoguiWindow_2->setLayout(new nanogui::GroupLayout());

	nanogui::Widget *viewSlider_Panel = new nanogui::Widget(nanoguiWindow_2);
	viewSlider_Panel->setLayout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal,
													   nanogui::Alignment::Middle, 0, 20));
	new nanogui::Label(viewSlider_Panel, "View slider");
	nanogui::Slider *viewSlider = new nanogui::Slider(viewSlider_Panel);
	viewSlider->setValue(1.0f);
	viewSlider->setFixedWidth(100);
	nanogui::TextBox *viewSlidertextBox = new nanogui::TextBox(viewSlider_Panel);
	viewSlidertextBox->setValue("1.0");
	viewSlider->setCallback([viewSlidertextBox](float value) {
		viewSlidertextBox->setValue(std::to_string(value));
		fViewSlider = value;
		volumeSlicing();
	});
	viewSlidertextBox->setFixedSize(Eigen::Vector2i(100, 25));
	viewSlidertextBox->setFontSize(18);
	viewSlidertextBox->setAlignment(nanogui::TextBox::Alignment::Right);

	//transfer function graph
	nanogui::Widget *graph_Panel = new nanogui::Widget(nanoguiWindow_2);
	graph_Panel->setLayout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal,
												  nanogui::Alignment::Middle, 0, 20));
	new nanogui::Label(graph_Panel, "Transfer function");
	nanogui::Graph *graph = new nanogui::Graph(graph_Panel, "Alpha: (0, 1)");
	graph->setFixedSize(Eigen::Vector2i(200, 150));
	graph->setHeader("");
	graph->setFooter("Intensity: (0, 255)");
	Eigen::VectorXf &func = graph->values();
	func.resize(8);
	setGraph(func);

	//Slider0
	nanogui::Widget *slider0_Panel = new nanogui::Widget(nanoguiWindow_2);
	slider0_Panel->setLayout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal,
													nanogui::Alignment::Middle, 0, 20));
	new nanogui::Label(slider0_Panel, "Slider 0");
	nanogui::Slider *slider0 = new nanogui::Slider(slider0_Panel);
	slider0->setValue(1.0f);
	slider0->setFixedWidth(100);
	nanogui::TextBox *slider0_textBox = new nanogui::TextBox(slider0_Panel);
	slider0_textBox->setValue("1.0");
	slider0->setCallback([slider0_textBox, graph](float value) {
		slider0_textBox->setValue(std::to_string(value));
		// dSlider0 = value;
		fSlider[0] = value;
		setGraph(graph->values());
		loadTransferFunction();
	});
	slider0_textBox->setFixedSize(Eigen::Vector2i(100, 25));
	slider0_textBox->setFontSize(18);
	slider0_textBox->setAlignment(nanogui::TextBox::Alignment::Right);

	//Slider1
	nanogui::Widget *slider1_Panel = new nanogui::Widget(nanoguiWindow_2);
	slider1_Panel->setLayout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal,
													nanogui::Alignment::Middle, 0, 20));
	new nanogui::Label(slider1_Panel, "Slider 1");
	nanogui::Slider *slider1 = new nanogui::Slider(slider1_Panel);
	slider1->setValue(1.0f);
	slider1->setFixedWidth(100);
	nanogui::TextBox *slider1_textBox = new nanogui::TextBox(slider1_Panel);
	slider1_textBox->setValue("1.0");
	slider1->setCallback([slider1_textBox, graph](float value) {
		slider1_textBox->setValue(std::to_string(value));
		// dSlider1 = value;
		fSlider[1] = value;
		setGraph(graph->values());
		loadTransferFunction();
	});
	slider1_textBox->setFixedSize(Eigen::Vector2i(100, 25));
	slider1_textBox->setFontSize(18);
	slider1_textBox->setAlignment(nanogui::TextBox::Alignment::Right);

	//Slider2
	nanogui::Widget *slider2_Panel = new nanogui::Widget(nanoguiWindow_2);
	slider2_Panel->setLayout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal,
													nanogui::Alignment::Middle, 0, 20));
	new nanogui::Label(slider2_Panel, "Slider 2");
	nanogui::Slider *slider2 = new nanogui::Slider(slider2_Panel);
	slider2->setValue(1.0f);
	slider2->setFixedWidth(100);
	nanogui::TextBox *slider2_textBox = new nanogui::TextBox(slider2_Panel);
	slider2_textBox->setValue("1.0");
	slider2->setCallback([slider2_textBox, graph](float value) {
		slider2_textBox->setValue(std::to_string(value));
		// dSlider2 = value;
		fSlider[2] = value;
		setGraph(graph->values());
		loadTransferFunction();
	});
	slider2_textBox->setFixedSize(Eigen::Vector2i(100, 25));
	slider2_textBox->setFontSize(18);
	slider2_textBox->setAlignment(nanogui::TextBox::Alignment::Right);

	//Slider3
	nanogui::Widget *slider3_Panel = new nanogui::Widget(nanoguiWindow_2);
	slider3_Panel->setLayout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal,
													nanogui::Alignment::Middle, 0, 20));
	new nanogui::Label(slider3_Panel, "Slider 3");
	nanogui::Slider *slider3 = new nanogui::Slider(slider3_Panel);
	slider3->setValue(1.0f);
	slider3->setFixedWidth(100);
	nanogui::TextBox *slider3_textBox = new nanogui::TextBox(slider3_Panel);
	slider3_textBox->setValue("1.0");
	slider3->setCallback([slider3_textBox, graph](float value) {
		slider3_textBox->setValue(std::to_string(value));
		// dSlider3 = value;
		fSlider[3] = value;
		setGraph(graph->values());
		loadTransferFunction();
	});
	slider3_textBox->setFixedSize(Eigen::Vector2i(100, 25));
	slider3_textBox->setFontSize(18);
	slider3_textBox->setAlignment(nanogui::TextBox::Alignment::Right);

	//Slider4
	nanogui::Widget *slider4_Panel = new nanogui::Widget(nanoguiWindow_2);
	slider4_Panel->setLayout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal,
													nanogui::Alignment::Middle, 0, 20));
	new nanogui::Label(slider4_Panel, "Slider 4");
	nanogui::Slider *slider4 = new nanogui::Slider(slider4_Panel);
	slider4->setValue(1.0f);
	slider4->setFixedWidth(100);
	nanogui::TextBox *slider4_textBox = new nanogui::TextBox(slider4_Panel);
	slider4_textBox->setValue("1.0");
	slider4->setCallback([slider4_textBox, graph](float value) {
		slider4_textBox->setValue(std::to_string(value));
		// dSlider4 = value;
		fSlider[4] = value;
		setGraph(graph->values());
		loadTransferFunction();
	});
	slider4_textBox->setFixedSize(Eigen::Vector2i(100, 25));
	slider4_textBox->setFontSize(18);
	slider4_textBox->setAlignment(nanogui::TextBox::Alignment::Right);

	//Slider5
	nanogui::Widget *slider5_Panel = new nanogui::Widget(nanoguiWindow_2);
	slider5_Panel->setLayout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal,
													nanogui::Alignment::Middle, 0, 20));
	new nanogui::Label(slider5_Panel, "Slider 5");
	nanogui::Slider *slider5 = new nanogui::Slider(slider5_Panel);
	slider5->setValue(1.0f);
	slider5->setFixedWidth(100);
	nanogui::TextBox *slider5_textBox = new nanogui::TextBox(slider5_Panel);
	slider5_textBox->setValue("1.0");
	slider5->setCallback([slider5_textBox, graph](float value) {
		slider5_textBox->setValue(std::to_string(value));
		// dSlider5 = value;
		fSlider[5] = value;
		setGraph(graph->values());
		loadTransferFunction();
	});
	slider5_textBox->setFixedSize(Eigen::Vector2i(100, 25));
	slider5_textBox->setFontSize(18);
	slider5_textBox->setAlignment(nanogui::TextBox::Alignment::Right);

	//Slider6
	nanogui::Widget *slider6_Panel = new nanogui::Widget(nanoguiWindow_2);
	slider6_Panel->setLayout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal,
													nanogui::Alignment::Middle, 0, 20));
	new nanogui::Label(slider6_Panel, "Slider 6");
	nanogui::Slider *slider6 = new nanogui::Slider(slider6_Panel);
	slider6->setValue(1.0f);
	slider6->setFixedWidth(100);
	nanogui::TextBox *slider6_textBox = new nanogui::TextBox(slider6_Panel);
	slider6_textBox->setValue("1.0");
	slider6->setCallback([slider6_textBox, graph](float value) {
		slider6_textBox->setValue(std::to_string(value));
		// dSlider6 = value;
		fSlider[6] = value;
		setGraph(graph->values());
		loadTransferFunction();
	});
	slider6_textBox->setFixedSize(Eigen::Vector2i(100, 25));
	slider6_textBox->setFontSize(18);
	slider6_textBox->setAlignment(nanogui::TextBox::Alignment::Right);

	//Slider7
	nanogui::Widget *slider7_Panel = new nanogui::Widget(nanoguiWindow_2);
	slider7_Panel->setLayout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal,
													nanogui::Alignment::Middle, 0, 20));
	new nanogui::Label(slider7_Panel, "Slider 7");
	nanogui::Slider *slider7 = new nanogui::Slider(slider7_Panel);
	slider7->setValue(1.0f);
	slider7->setFixedWidth(100);
	nanogui::TextBox *slider7_textBox = new nanogui::TextBox(slider7_Panel);
	slider7_textBox->setValue("1.0");
	slider7->setCallback([slider7_textBox, graph](float value) {
		slider7_textBox->setValue(std::to_string(value));
		// dSlider7 = value;
		fSlider[7] = value;
		setGraph(graph->values());
		loadTransferFunction();
	});
	slider7_textBox->setFixedSize(Eigen::Vector2i(100, 25));
	slider7_textBox->setFontSize(18);
	slider7_textBox->setAlignment(nanogui::TextBox::Alignment::Right);

	// Init screen:
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

/* update graph */
void setGraph(Eigen::VectorXf &func)
{
	//set graph funcion values to sliders' values
	for (int i = 0; i < SLIDER_NUM; i++)
	{
		func[i] = fSlider[i];
	}
}

/* Set sampling rate when sampling rate changes */
void setSamplingRate()
{
	//check the range of num_slices variable
	if (iSamplingRate < 3)
	{
		iSamplingRate = 3;
	}
	if (iSamplingRate > MAX_SLICES)
	{
		iSamplingRate = MAX_SLICES;
	}
	//opacity correction
	//curAlpha = 1 - pow(opacityCorrection, 1.0 / (double)iSamplingRate);

	//slice the volume
	volumeSlicing();
}

/* Reset the camera to its original position */
void reset()
{
	Renderer::m_camera->reset();
	dPositionX = 0.0;
	dPositionY = 0.0;
	dPositionZ = 4.0;
	gui_PositionX->setValue(dPositionX);
	gui_PositionY->setValue(dPositionY);
	gui_PositionZ->setValue(dPositionZ);
	viewDir = glm::vec3(dPositionX, dPositionY, dPositionZ);
	volumeSlicing();
}

/* Set model when model name changes */
void setModel()
{
	if (model_val == model_type::Bonsai)
	{
		modelPath = bonsaiPath;
		dimension = glm::vec3(512, 512, 154);
	}
	else if (model_val == model_type::Teapot)
	{
		modelPath = teapotPath;
		dimension = glm::vec3(256, 256, 178);
	}
	else if (model_val == model_type::Bucky)
	{
		modelPath = buckyPath;
		dimension = glm::vec3(32, 32, 32);
	}
	else // head
	{
		modelPath = headPath;
		dimension = glm::vec3(256, 256, 225);
	}

	//reload model and update texture
	GLubyte *data = load_3d_raw_data(modelPath, dimension);
	loadTexture(data);
	//loadTransferFunction();
}

/* function that load a volume from the given raw data file */
GLubyte *load_3d_raw_data(std::string texture_path, glm::vec3 dimension)
{
	size_t size = dimension[0] * dimension[1] * dimension[2];

	FILE *fp;
	GLubyte *data = new GLubyte[size]; // 8bit
	if (!(fp = fopen(texture_path.c_str(), "rb")))
	{
		std::cout << "Error: opening .raw file failed" << std::endl;
		exit(EXIT_FAILURE);
	}
	else
	{
		std::cout << "OK: open .raw file successed" << std::endl;
	}
	if (fread(data, sizeof(char), size, fp) != size)
	{
		std::cout << "Error: read .raw file failed" << std::endl;
		exit(1);
	}
	else
	{
		std::cout << "OK: read .raw file successed" << std::endl;
	}
	fclose(fp);
	return data;
}

/* function that generates an OpenGL 3D texture from volume data */
void loadTexture(GLubyte *data)
{
	//generate OpenGL texture
	glGenTextures(1, &volTexture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, volTexture);

	// set the texture parameters
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	//set the mipmap levels (base and max)
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, 4);

	//allocate data with internal format and foramt as (GL_RED)
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RED, dimension[0], dimension[1], dimension[2], 0, GL_RED, GL_UNSIGNED_BYTE, data);

	//generate mipmaps
	glGenerateMipmap(GL_TEXTURE_3D);
	delete[] data;
}

/* Load transfer function */
void loadTransferFunction()
{
	float pData[256][4]; //prepare data array
	int indices[SLIDER_NUM];
	int delta = 256 / SLIDER_NUM; //definde interval

	//fill the color values at the control point
	for (int i = 0; i < SLIDER_NUM; i++)
	{
		int idx = i * delta;
		pData[idx][0] = lookupTable[i].x;
		pData[idx][1] = lookupTable[i].y;
		pData[idx][2] = lookupTable[i].z;
		pData[idx][3] = lookupTable[i].w;
		pData[idx][3] = fSlider[i];
		indices[i] = idx;
	}

	//interpolate
	for (int j = 0; j < SLIDER_NUM - 1; j++)
	{
		float dDataR = (pData[indices[j + 1]][0] - pData[indices[j]][0]);
		float dDataG = (pData[indices[j + 1]][1] - pData[indices[j]][1]);
		float dDataB = (pData[indices[j + 1]][2] - pData[indices[j]][2]);
		float dDataA = (pData[indices[j + 1]][3] - pData[indices[j]][3]);
		int dIdx = indices[j + 1] - indices[j];

		float dDataIncR = dDataR / float(dIdx);
		float dDataIncG = dDataG / float(dIdx);
		float dDataIncB = dDataB / float(dIdx);
		float dDataIncA = dDataA / float(dIdx);
		for (int i = indices[j] + 1; i < indices[j + 1]; i++)
		{
			pData[i][0] = (pData[i - 1][0] + dDataIncR);
			pData[i][1] = (pData[i - 1][1] + dDataIncG);
			pData[i][2] = (pData[i - 1][2] + dDataIncB);
			pData[i][3] = (pData[i - 1][3] + dDataIncA);
		}
	}

	//generate the OpenGL texture
	glGenTextures(1, &tfTexture);
	//bind this texture to texture unit 1
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_1D, tfTexture);

	// set the texture parameters
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	//allocate the data to texture memory
	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 256, 0, GL_RGBA, GL_FLOAT, pData);

	// glGenerateMipmap(GL_TEXTURE_1D);
}

/* load Colorbar and fill the lookup table */
void loadColorBar(std::string colorBarPath)
{
	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
	unsigned char *data = stbi_load(colorBarPath.c_str(), &width, &height, &nrChannels, 0);
	if (data)
	{
		int delta = 256 / SLIDER_NUM;
		//fill out lookup table
		for (int i = 0; i < SLIDER_NUM; i++)
		{
			int index = i * delta * 4;
			lookupTable[i].x = (float)data[index] / (float)255;
			lookupTable[i].y = (float)data[index + 1] / (float)255;
			lookupTable[i].z = (float)data[index + 2] / (float)255;
			lookupTable[i].w = (float)data[index + 3] / (float)255;
		}
	}
}
