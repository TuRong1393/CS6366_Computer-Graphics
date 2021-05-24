#include "Renderer.h"

// Pre-define gui variable handles:
#define GUI_DOUBLE nanogui::detail::FormWidget<double, std::integral_constant<bool, true> >
#define GUI_FLOAT nanogui::detail::FormWidget<float, std::integral_constant<bool, true> >
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
enum culling_type
{
	CW = 0,
	CCW = 1
};
enum depth_type
{
	Less = 0,
	Always = 1
};

enum model_type
{
	Bonsai = 0,
	Teapot = 1,
	Bucky = 2,
	Head = 3
};

#define GUI_RENDERTYPE nanogui::detail::FormWidget<render_type, std::integral_constant<bool, true> >
#define GUI_MODELTYPE nanogui::detail::FormWidget<model_type, std::integral_constant<bool, true> >
#define GUI_CULLINGTYPE nanogui::detail::FormWidget<culling_type, std::integral_constant<bool, true> >
#define GUI_DEPTHTYPE nanogui::detail::FormWidget<depth_type, std::integral_constant<bool, true> >

// Gui variable handles to edit fields in the gui:
GUI_CULLINGTYPE *gui_CullingType;
GUI_DEPTHTYPE *gui_DepthType;
GUI_RENDERTYPE *gui_RenderType;
GUI_COLOR *gui_ColObject;
GUI_DOUBLE *gui_RotValue;
GUI_DOUBLE *gui_PositionX;
GUI_DOUBLE *gui_PositionY;
GUI_DOUBLE *gui_PositionZ;
// GUI_DOUBLE *gui_NearPlane;
// GUI_DOUBLE *gui_FarPlane;

GUI_MODELTYPE *gui_ModelType;
GUI_INT *gui_IsoValue;
GUI_INT *gui_SamplingDistX;
GUI_INT *gui_SamplingDistY;
GUI_INT *gui_SamplingDistZ;
GUI_BOOL *gui_NormalAsColor;
GUI_FLOAT *gui_Alpha;

// Variable defaults modified in the gui:
nanogui::Color colObject(0.5f, 0.5f, 0.7f, 1.0f); // color of the object
double dPositionX = 0.5;
double dPositionY = 0.5;
double dPositionZ = 2.5;
double dRotValue = 0.0;
// double dNearPlane = Renderer::m_camera->NEAR_PLANE;
// double dFarPlane = Renderer::m_camera->FAR_PLANE;
render_type render_val = render_type::Line;
model_type model_val = model_type::Teapot;
culling_type culling_val = culling_type::CCW;
depth_type depth_val = depth_type::Less;

std::string teapotPath = "../src/objs/BostonTeapot_256_256_178.raw";
std::string bonsaiPath = "../src/objs/Bonsai_512_512_154.raw";
std::string buckyPath = "../src/objs/Bucky_32_32_32.raw";
std::string headPath = "../src/objs/Head_256_256_225.raw";
std::string modelPath = teapotPath; //initialized as teapot's path
int dimension[3] = {256, 256, 178};
glm::vec3 invDim = glm::vec3(1.0 / 256, 1.0 / 256, 1.0 / 178);

//sampling distances in voxels
int samplingDistance[3] = {2, 2, 2};

//volume data pointer
GLubyte *pData = NULL;

//the given isovalue to look for
int isoValue = 50;

//vertices vector storing positions and normals
struct Vertex
{
	glm::vec3 pos;
	glm::vec3 normal;
};
std::vector<Vertex> vertices;

// bool bWireframeSign = false;
bool bNormalAsColor = false;
float fAlpha = 0.1;

GLubyte cubeCornerValues[8];
glm::vec3 edgeVertices[12];
glm::vec3 edgeNormals[12];

GLuint VBO, VAO;

const int vertexOffsetTable[8][3] =
	{{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};

//edgeConnectionTable lists the index of the endpoint vertices for each of the 12 edges of the cube
const GLint edgeConnectionTable[12][2] =
	{{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};

//edgeDirectionTable lists the direction vector (vertex1-vertex0) for each edge in the cube
const int edgeDirectionTable[12][3] =
	{{1, 0, 0}, {0, 1, 0}, {-1, 0, 0}, {0, -1, 0}, {1, 0, 0}, {0, 1, 0}, {-1, 0, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1}};

//I found the tables online: http://paulbourke.net/geometry/polygonise/
GLint cubeEdgeFlagsTable[256] =
	{0x000, 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c, 0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
	 0x190, 0x099, 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c, 0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
	 0x230, 0x339, 0x033, 0x13a, 0x636, 0x73f, 0x435, 0x53c, 0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
	 0x3a0, 0x2a9, 0x1a3, 0x0aa, 0x7a6, 0x6af, 0x5a5, 0x4ac, 0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
	 0x460, 0x569, 0x663, 0x76a, 0x066, 0x16f, 0x265, 0x36c, 0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
	 0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0x0ff, 0x3f5, 0x2fc, 0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
	 0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x055, 0x15c, 0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
	 0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0x0cc, 0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
	 0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc, 0x0cc, 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
	 0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c, 0x15c, 0x055, 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
	 0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc, 0x2fc, 0x3f5, 0x0ff, 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
	 0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c, 0x36c, 0x265, 0x16f, 0x066, 0x76a, 0x663, 0x569, 0x460,
	 0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac, 0x4ac, 0x5a5, 0x6af, 0x7a6, 0x0aa, 0x1a3, 0x2a9, 0x3a0,
	 0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c, 0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x033, 0x339, 0x230,
	 0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c, 0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x099, 0x190,
	 0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c, 0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x000};

//  For each of the possible vertex states listed in cubeEdgeFlagsTable there is a specific triangulation
//  of the edge intersection points.  triangleConnectionTable lists all of them in the form of
//  0-5 edge triples with the list terminated by the invalid value -1.
//  For example: triangleConnectionTable[3] list the 2 triangles formed when corner[0]
//  and corner[1] are inside of the surface, but the rest of the cube is not.
GLint triangleConnectionTable[256][16] =
	{{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {0, 8, 3, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1},
	 {3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {1, 9, 0, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1},
	 {3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1},
	 {3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1},
	 {9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1},
	 {1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {3, 4, 7, 3, 0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1},
	 {9, 2, 10, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
	 {2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1},
	 {8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1},
	 {9, 0, 1, 8, 4, 7, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
	 {4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1},
	 {3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1, -1, -1},
	 {1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1},
	 {4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1},
	 {4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1},
	 {9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1},
	 {1, 2, 10, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {3, 0, 8, 1, 2, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
	 {5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1},
	 {2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1},
	 {9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {0, 11, 2, 0, 8, 11, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
	 {0, 5, 4, 0, 1, 5, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
	 {2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1},
	 {10, 3, 11, 10, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1},
	 {4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, -1, -1, -1, -1},
	 {5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1},
	 {5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1},
	 {9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1},
	 {0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1, -1},
	 {1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {9, 7, 8, 9, 5, 7, 10, 1, 2, -1, -1, -1, -1, -1, -1, -1},
	 {10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1, -1},
	 {8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, -1, -1, -1, -1},
	 {2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1},
	 {7, 9, 5, 7, 8, 9, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1},
	 {9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1, -1},
	 {2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, -1, -1, -1, -1},
	 {11, 2, 1, 11, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1, -1},
	 {9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, -1, -1, -1, -1},
	 {5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, -1},
	 {11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, -1},
	 {11, 10, 5, 7, 11, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {0, 8, 3, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {9, 0, 1, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {1, 8, 3, 1, 9, 8, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
	 {1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {1, 6, 5, 1, 2, 6, 3, 0, 8, -1, -1, -1, -1, -1, -1, -1},
	 {9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1, -1},
	 {5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1},
	 {2, 3, 11, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {11, 0, 8, 11, 2, 0, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
	 {0, 1, 9, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
	 {5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, -1, -1, -1, -1},
	 {6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1},
	 {0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1},
	 {3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1},
	 {6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1, -1, -1, -1, -1, -1},
	 {5, 10, 6, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {4, 3, 0, 4, 7, 3, 6, 5, 10, -1, -1, -1, -1, -1, -1, -1},
	 {1, 9, 0, 5, 10, 6, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
	 {10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, -1, -1, -1, -1},
	 {6, 1, 2, 6, 5, 1, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1},
	 {1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, -1, -1, -1, -1},
	 {8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, -1, -1, -1, -1},
	 {7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, -1},
	 {3, 11, 2, 7, 8, 4, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
	 {5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, -1, -1, -1, -1},
	 {0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1},
	 {9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6, -1},
	 {8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, -1, -1, -1, -1},
	 {5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, -1},
	 {0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7, -1},
	 {6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, -1, -1, -1, -1},
	 {10, 4, 9, 6, 4, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {4, 10, 6, 4, 9, 10, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1},
	 {10, 0, 1, 10, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1, -1},
	 {8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, -1, -1, -1, -1},
	 {1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1, -1},
	 {3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, -1, -1, -1, -1},
	 {0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1},
	 {10, 4, 9, 10, 6, 4, 11, 2, 3, -1, -1, -1, -1, -1, -1, -1},
	 {0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, -1, -1, -1, -1},
	 {3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, -1, -1, -1, -1},
	 {6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1, -1},
	 {9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, -1, -1, -1, -1},
	 {8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, -1},
	 {3, 11, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1, -1},
	 {6, 4, 8, 11, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {7, 10, 6, 7, 8, 10, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1},
	 {0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, -1, -1, -1, -1},
	 {10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1},
	 {10, 6, 7, 10, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, -1},
	 {1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1, -1},
	 {2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, -1},
	 {7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, -1},
	 {7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, -1, -1, -1, -1},
	 {2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7, -1},
	 {1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, -1},
	 {11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, -1, -1, -1, -1},
	 {8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6, -1},
	 {0, 9, 1, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, -1, -1, -1, -1},
	 {7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {3, 0, 8, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {0, 1, 9, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {8, 1, 9, 8, 3, 1, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
	 {10, 1, 2, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {1, 2, 10, 3, 0, 8, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
	 {2, 9, 0, 2, 10, 9, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
	 {6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, -1, -1, -1, -1},
	 {7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1, -1},
	 {2, 7, 6, 2, 3, 7, 0, 1, 9, -1, -1, -1, -1, -1, -1, -1},
	 {1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1, -1},
	 {10, 7, 6, 10, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1, -1},
	 {10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1},
	 {0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, -1, -1, -1, -1},
	 {7, 6, 10, 7, 10, 8, 8, 10, 9, -1, -1, -1, -1, -1, -1, -1},
	 {6, 8, 4, 11, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {3, 6, 11, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1, -1},
	 {8, 6, 11, 8, 4, 6, 9, 0, 1, -1, -1, -1, -1, -1, -1, -1},
	 {9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, -1, -1, -1, -1},
	 {6, 8, 4, 6, 11, 8, 2, 10, 1, -1, -1, -1, -1, -1, -1, -1},
	 {1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, -1, -1, -1, -1},
	 {4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, -1, -1, -1, -1},
	 {10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, -1},
	 {8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1},
	 {0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, -1, -1, -1, -1},
	 {1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1, -1},
	 {8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, -1, -1, -1, -1},
	 {10, 1, 0, 10, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1, -1},
	 {4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, -1},
	 {10, 9, 4, 6, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {4, 9, 5, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {0, 8, 3, 4, 9, 5, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
	 {5, 0, 1, 5, 4, 0, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
	 {11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, -1, -1, -1, -1},
	 {9, 5, 4, 10, 1, 2, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
	 {6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, -1, -1, -1, -1},
	 {7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, -1, -1, -1, -1},
	 {3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, -1},
	 {7, 2, 3, 7, 6, 2, 5, 4, 9, -1, -1, -1, -1, -1, -1, -1},
	 {9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, -1, -1, -1, -1},
	 {3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, -1, -1, -1, -1},
	 {6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, -1},
	 {9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, -1, -1, -1, -1},
	 {1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, -1},
	 {4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, -1},
	 {7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, -1, -1, -1, -1},
	 {6, 9, 5, 6, 11, 9, 11, 8, 9, -1, -1, -1, -1, -1, -1, -1},
	 {3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1, -1},
	 {0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, -1, -1, -1, -1},
	 {6, 11, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1, -1},
	 {1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, -1, -1, -1, -1},
	 {0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10, -1},
	 {11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5, -1},
	 {6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, -1, -1, -1, -1},
	 {5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1},
	 {9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1, -1},
	 {1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, -1},
	 {1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6, -1},
	 {10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, -1, -1, -1, -1},
	 {0, 3, 8, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {11, 5, 10, 7, 5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {11, 5, 10, 11, 7, 5, 8, 3, 0, -1, -1, -1, -1, -1, -1, -1},
	 {5, 11, 7, 5, 10, 11, 1, 9, 0, -1, -1, -1, -1, -1, -1, -1},
	 {10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, -1, -1, -1, -1},
	 {11, 1, 2, 11, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1, -1},
	 {0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, -1, -1, -1, -1},
	 {9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, -1, -1, -1, -1},
	 {7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2, -1},
	 {2, 5, 10, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1},
	 {8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, -1, -1, -1, -1},
	 {9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, -1, -1, -1, -1},
	 {9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2, -1},
	 {1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1, -1},
	 {9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1, -1},
	 {9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {5, 8, 4, 5, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1},
	 {5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, -1, -1, -1, -1},
	 {0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, -1, -1, -1, -1},
	 {10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4, -1},
	 {2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, -1, -1, -1, -1},
	 {0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, -1},
	 {0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, -1},
	 {9, 4, 5, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, -1},
	 {5, 10, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1},
	 {3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, -1},
	 {5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, -1, -1, -1, -1},
	 {8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1, -1},
	 {0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, -1, -1, -1, -1},
	 {9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {4, 11, 7, 4, 9, 11, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1},
	 {0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, -1, -1, -1, -1},
	 {1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, -1, -1, -1, -1},
	 {3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, -1},
	 {4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, -1, -1, -1, -1},
	 {9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3, -1},
	 {11, 7, 4, 11, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1, -1},
	 {11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, -1, -1, -1, -1},
	 {2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, -1},
	 {9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7, -1},
	 {3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, -1},
	 {1, 10, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1, -1},
	 {4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, -1, -1, -1, -1},
	 {4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {9, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {3, 0, 9, 3, 9, 11, 11, 9, 10, -1, -1, -1, -1, -1, -1, -1},
	 {0, 1, 10, 0, 10, 8, 8, 10, 11, -1, -1, -1, -1, -1, -1, -1},
	 {3, 1, 10, 11, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {1, 2, 11, 1, 11, 9, 9, 11, 8, -1, -1, -1, -1, -1, -1, -1},
	 {3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, -1, -1, -1, -1},
	 {0, 2, 11, 8, 0, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {2, 3, 8, 2, 8, 10, 10, 8, 9, -1, -1, -1, -1, -1, -1, -1},
	 {9, 10, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, -1, -1, -1, -1},
	 {1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	 {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

//function foward declarations
void reset();
void bind_vaovbo();
void setModel();
void load_3d_raw_data(std::string texture_path);
void setSampleVoxel(const int x, const int y, const int z);
void marchingVolume();
GLubyte getSampleVolume(const int x, const int y, const int z);
glm::vec3 getNormal(const int x, const int y, const int z);
float getOffsetRatio(const GLubyte v1, const GLubyte v2);
void setIsoValue();
void setCulling();
void setDepth();

//empty constructor
Renderer::Renderer()
{
}
Renderer::~Renderer()
{
}

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
	Renderer::m_window = glfwCreateWindow(Renderer::m_camera->width, Renderer::m_camera->height, "CS6366 Final Project: Marching Tetrahedra", nullptr, nullptr);

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
	load_3d_raw_data(modelPath);
	marchingVolume();

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

	// Build and compile our shader program
	Shader ourShader("../src/shader/basic.vert", "../src/shader/basic.frag");

	ourShader.use();

	bind_vaovbo();

	// Main frame while loop
	while (!glfwWindowShouldClose(Renderer::m_window))
	{
		// Specify culling type
		setCulling();
		// Specify depth type
		setDepth();
		// Check if any events are triggered
		glfwPollEvents();

		// Clear the color buffer & depth buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//bind volume vertex array object
		glBindVertexArray(VAO);

		ourShader.use();

		// Set uniform values of shader
		setup_uniform_values(ourShader);

		// Set draw type:
		if (render_val == render_type::Triangle)
		{
			glDrawArrays(GL_TRIANGLES, 0, vertices.size());
		}
		else if (render_val == render_type::Point)
		{
			glPointSize(1.5f); // set point size
			glDrawArrays(GL_POINTS, 0, vertices.size());
		}
		else if (render_val == render_type::Line)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glDrawArrays(GL_TRIANGLES, 0, vertices.size());
		}

		// Set draw mode back to fill & draw gui
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		Renderer::m_nanogui_screen->drawWidgets();

		// Swap the screen buffers
		glfwSwapBuffers(Renderer::m_window);
	}

	// De-allocate all resources once they've outlived their purpose
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	ourShader.del();

	// Terminate GLFW, clearing any resources allocated by GLFW.
	glfwTerminate();
}

/* Setup the uniform values of the shader*/
void Renderer::setup_uniform_values(Shader &shader)
{
	shader.setMatrix4fv("MVP", 1, GL_FALSE, &(Renderer::m_camera->getMVPMatrix()[0][0]));
	shader.setVec4("colObject", colObject.r(), colObject.g(), colObject.b(), colObject.w());
	shader.setBool("bNormalAsColor", bNormalAsColor);
	shader.setVec1("fAlpha", fAlpha);
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
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

	//enable vertex attribute array for position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
	glEnableVertexAttribArray(0);

	//enable vertex attribute array for normals
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid *)offsetof(Vertex, normal));

	glBindVertexArray(0);
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
	gui_CullingType = gui->addVariable("Culling Type", culling_val, enabled);
	gui_CullingType->setItems({"CW", "CCW"});
	gui_DepthType = gui->addVariable("Depth Type", depth_val, enabled);
	gui_DepthType->setItems({"Less", "Always"});

	gui_RenderType = gui->addVariable("Render Type", render_val, enabled);
	gui_RenderType->setItems({"Point", "Line", "Triangle"});

	gui->addButton("Reset", &reset);

	// Object color:
	gui->addGroup("Volume Rendering");
	gui_ColObject = gui->addVariable("Object Color", colObject);
	gui_ModelType = gui->addVariable("Model name", model_val, enabled);
	gui_ModelType->setItems({"Bonsai", "Teapot", "Bucky", "Head"});
	gui_IsoValue = gui->addVariable("IsoValue", isoValue);
	gui_IsoValue->setSpinnable(true);
	gui_SamplingDistX = gui->addVariable("Sampling distance X", samplingDistance[0]);
	gui_SamplingDistX->setSpinnable(true);
	gui_SamplingDistY = gui->addVariable("Sampling distance Y", samplingDistance[1]);
	gui_SamplingDistY->setSpinnable(true);
	gui_SamplingDistZ = gui->addVariable("Sampling distance Z", samplingDistance[2]);
	gui_SamplingDistZ->setSpinnable(true);

	gui_NormalAsColor = gui->addVariable("Normal as color", bNormalAsColor);
	gui_Alpha = gui->addVariable("Normal as color alpha", fAlpha);
	gui_Alpha->setSpinnable(true);

	// Callbacks to set global variables when changed in the gui:
	gui_RotValue->setCallback([](double val) { dRotValue = val; });
	gui_ColObject->setFinalCallback([](const nanogui::Color &c) { colObject = c; });
	gui_PositionX->setCallback([](double val) {
		dPositionX = val;
		Renderer::m_camera->localPos.x = dPositionX;
	});
	gui_PositionY->setCallback([](double val) {
		dPositionY = val;
		Renderer::m_camera->localPos.y = dPositionY;
	});
	gui_PositionZ->setCallback([](double val) {
		dPositionZ = val;
		Renderer::m_camera->localPos.z = dPositionZ;
	});
	// gui_FarPlane->setCallback([](double val) { dFarPlane = val; Renderer::m_camera->farPlane = dFarPlane; });
	// gui_NearPlane->setCallback([](double val) { dNearPlane = val; Renderer::m_camera->nearPlane = dNearPlane; });

	gui_CullingType->setCallback([](const culling_type &val) {
		culling_val = val;
	});
	gui_DepthType->setCallback([](const depth_type &val) { depth_val = val; });
	gui_RenderType->setCallback([](const render_type &val) { render_val = val; });

	gui_ModelType->setCallback([](const model_type &val) { model_val = val; setModel(); });
	gui_IsoValue->setCallback([](int val) { isoValue = val; setIsoValue(); });
	gui_SamplingDistX->setCallback([](int val) { samplingDistance[0] = val > 0 ? val : 1; marchingVolume(); bind_vaovbo(); });
	gui_SamplingDistY->setCallback([](int val) { samplingDistance[1] = val > 0 ? val : 1; marchingVolume(); bind_vaovbo(); });
	gui_SamplingDistZ->setCallback([](int val) { samplingDistance[2] = val > 0 ? val : 1; marchingVolume(); bind_vaovbo(); });

	gui_NormalAsColor->setCallback([](bool b) { bNormalAsColor = b; });
	gui_Alpha->setCallback([](float val) {
		fAlpha = val;
		if (fAlpha < 0.0)
		{
			fAlpha = 0.0f;
		}
		else if (fAlpha > 1.0)
		{
			fAlpha = 1.0f;
		}
	});

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

/* Reset the camera to its original position */
void reset()
{
	Renderer::m_camera->reset();
	dPositionX = 0.5;
	dPositionY = 0.5;
	dPositionZ = 2.5;
	gui_PositionX->setValue(dPositionX);
	gui_PositionY->setValue(dPositionY);
	gui_PositionZ->setValue(dPositionZ);
}

/* Set isoValue*/
void setIsoValue()
{
	if (isoValue < 10)
	{
		isoValue = 10;
	}
	marchingVolume();
	bind_vaovbo();
}

/* Set model when model name changes */
void setModel()
{
	if (model_val == model_type::Bonsai)
	{
		modelPath = bonsaiPath;
		dimension[0] = 512;
		dimension[1] = 512;
		dimension[2] = 154;
	}
	else if (model_val == model_type::Teapot)
	{
		modelPath = teapotPath;
		dimension[0] = 256;
		dimension[1] = 256;
		dimension[2] = 178;
	}
	else if (model_val == model_type::Bucky)
	{
		modelPath = buckyPath;
		dimension[0] = 32;
		dimension[1] = 32;
		dimension[2] = 32;
	}
	else
	{ //Head
		modelPath = headPath;
		dimension[0] = 256;
		dimension[1] = 256;
		dimension[2] = 225;
	}

	for (int i = 0; i < 3; i++)
	{
		invDim[i] = 1.0 / dimension[i];
	}

	//reload model
	load_3d_raw_data(modelPath);
	marchingVolume();
	bind_vaovbo();
}

/* function that load a volume from the given raw data file */
void load_3d_raw_data(std::string path)
{
	std::cout << modelPath << std::endl;
	std::cout << "dimension: " << std::endl;
	std::cout << dimension[0] << " " << dimension[1] << " " << dimension[2] << std::endl;
	size_t size = dimension[0] * dimension[1] * dimension[2];
	std::cout << "size: " << size << std::endl;
	FILE *fp;

	//delete the volume data allocated on heap
	delete[] pData;
	pData = NULL;

	pData = new GLubyte[size]; // 8bit
	if (!(fp = fopen(path.c_str(), "rb")))
	{
		std::cout << "Error: opening .raw file failed" << std::endl;
		exit(EXIT_FAILURE);
	}
	else
	{
		std::cout << "OK: open .raw file successed" << std::endl;
	}
	if (fread(pData, sizeof(char), size, fp) != size)
	{
		std::cout << "Error: read .raw file failed" << std::endl;
		exit(1);
	}
	else
	{
		std::cout << "OK: read .raw file successed" << std::endl;
	}
	fclose(fp);
}

/* main function of marching the volume */
void marchingVolume()
{
	vertices.clear();
	//loop z, y and x to take the sample voxel and set it
	for (int z = 0; z < dimension[2]; z += samplingDistance[2])
	{
		for (int y = 0; y < dimension[1]; y += samplingDistance[1])
		{
			for (int x = 0; x < dimension[0]; x += samplingDistance[0])
			{
				setSampleVoxel(x, y, z);
			}
		}
	}
}

/* For each sample voxel, */
void setSampleVoxel(const int x, const int y, const int z)
{
	//clear cubeCornerValues!!!
	for (int i = 0; i < 8; i++)
	{
		cubeCornerValues[i] = 0;
	}
	//clear edgeVertices & edgeNormals!!!
	for (int i = 0; i < 12; i++)
	{
		edgeVertices[i] = glm::vec3(0.0f, 0.0f, 0.0f);
		edgeNormals[i] = glm::vec3(0.0f, 0.0f, 0.0f);
	}

	//set cube corner values using the table
	for (int i = 0; i < 8; i++)
	{
		cubeCornerValues[i] = getSampleVolume(x + vertexOffsetTable[i][0] * samplingDistance[0],
											  y + vertexOffsetTable[i][1] * samplingDistance[1],
											  z + vertexOffsetTable[i][2] * samplingDistance[2]);
	}

	int flagIdx = 0;
	//loop corner values to find the ones that is smaller than the isoValue
	for (int i = 0; i < 8; i++)
	{
		if (cubeCornerValues[i] <= isoValue)
			flagIdx |= 1 << i;
	}

	//find which edges are intersected by the surface by looping up the table
	int intersectingEdgeFlags = cubeEdgeFlagsTable[flagIdx];

	//if no intersections(cube is entirely inside or outside), do nothing
	if (intersectingEdgeFlags == 0)
	{
		return;
	}

	//for all edges of the cube
	for (int i = 0; i < 12; i++)
	{
		//if there is an intersection on this edge
		if (intersectingEdgeFlags & (1 << i))
		{
			//get the offset
			float offsetRatio = getOffsetRatio(cubeCornerValues[edgeConnectionTable[i][0]], cubeCornerValues[edgeConnectionTable[i][1]]);

			//use offset to get the vertex position
			edgeVertices[i].x = x + (vertexOffsetTable[edgeConnectionTable[i][0]][0] + offsetRatio * edgeDirectionTable[i][0]) * samplingDistance[0];
			edgeVertices[i].y = y + (vertexOffsetTable[edgeConnectionTable[i][0]][1] + offsetRatio * edgeDirectionTable[i][1]) * samplingDistance[1];
			edgeVertices[i].z = z + (vertexOffsetTable[edgeConnectionTable[i][0]][2] + offsetRatio * edgeDirectionTable[i][2]) * samplingDistance[2];

			//use the vertex position to calculate the normal
			edgeNormals[i] = getNormal((int)edgeVertices[i].x, (int)edgeVertices[i].y, (int)edgeVertices[i].z);
		}
	}

	//Draw the triangles that were found.  There can be up to five per cube
	for (int i = 0; i < 5; i++)
	{
		if (triangleConnectionTable[flagIdx][3 * i] < 0)
			break;

		for (int j = 0; j < 3; j++)
		{
			int vertex = triangleConnectionTable[flagIdx][3 * i + j];
			Vertex v;
			v.normal = (edgeNormals[vertex]);
			v.pos = (edgeVertices[vertex]) * invDim;
			vertices.push_back(v);
		}
	}
}

/* get the sample volume from the data read from .raw file */
GLubyte getSampleVolume(const int x, const int y, const int z)
{
	int idx = (x + (y * dimension[0])) + z * (dimension[0] * dimension[1]);
	idx = idx <= 0 ? 0 : (idx >= dimension[0] * dimension[1] * dimension[2] ? (dimension[0] * dimension[1] * dimension[2] - 1) : idx);
	return pData[idx];
}

/* get the normal vector according the given x, y and z */
glm::vec3 getNormal(const int x, const int y, const int z)
{
	glm::vec3 Normal;
	Normal.x = (getSampleVolume(x - 1, y, z) - getSampleVolume(x + 1, y, z)) * 0.5f;
	Normal.y = (getSampleVolume(x, y - 1, z) - getSampleVolume(x, y + 1, z)) * 0.5f;
	Normal.z = (getSampleVolume(x, y, z - 1) - getSampleVolume(x, y, z + 1)) * 0.5f;
	return glm::normalize(Normal);
}

/* get offset */
float getOffsetRatio(const GLubyte v1, const GLubyte v2)
{
	float delta = (float)(v2 - v1);
	if (delta == 0)
		return 0.5f;
	else
		return (isoValue - v1) / delta;
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
