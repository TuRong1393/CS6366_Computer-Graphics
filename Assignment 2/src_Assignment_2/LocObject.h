#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class LocObject
{
public:
	struct Vertex
	{
		// Position
		glm::vec3 Position;
		// Normal
		glm::vec3 Normal;
		// TexCoords
		glm::vec2 TexCoords;
	};
	std::vector<Vertex> Vertices;
	std::string m_obj_path;

	// GLuint vao, vbo;
	float Min_Z, Max_Z;

public:
	LocObject(std::string obj_path)
	{
		this->m_obj_path = obj_path;
		load_obj(this->m_obj_path);
	};

	~LocObject(){};

	bool load_obj(std::string model_name)
	{

		std::vector<glm::vec3> positions;
		std::vector<glm::vec3> normals;
		std::vector<glm::vec2> tex_coords;

		std::ifstream file(model_name.c_str(), std::ios::in);

		Min_Z = std::numeric_limits<float>::max();
		Max_Z = std::numeric_limits<float>::min();

		// Check for valid file
		if (!file)
		{
			printf("ERROR: Invalid file path.\n");
			return false;
		}
		try
		{
			std::string curLine;

			// Read each line
			while (getline(file, curLine))
			{
				std::stringstream ss(curLine);
				std::string firstWord;
				ss >> firstWord;

				// Vertex:
				if (firstWord == "v")
				{
					glm::vec3 vert_pos;
					ss >> vert_pos[0] >> vert_pos[1] >> vert_pos[2];

					// Construct bounds of model:
					if (vert_pos[1] < Min_Z)
					{
						Min_Z = vert_pos[1];
					}
					if (vert_pos[1] > Max_Z)
					{
						Max_Z = vert_pos[1];
					}

					positions.push_back(vert_pos);
				}
				// Texture Coordinate
				else if (firstWord == "vt")
				{
					glm::vec2 tex_coord;
					ss >> tex_coord[0] >> tex_coord[1];
					tex_coords.push_back(tex_coord);
				}
				// Vertex Normal:
				else if (firstWord == "vn")
				{
					glm::vec3 vert_norm;
					ss >> vert_norm[0] >> vert_norm[1] >> vert_norm[2];
					normals.push_back(vert_norm);
				}
				// Face:
				else if (firstWord == "f")
				{
					std::string s_vertex_0, s_vertex_1, s_vertex_2;
					ss >> s_vertex_0 >> s_vertex_1 >> s_vertex_2;
					int pos_idx, tex_idx, norm_idx;
					std::sscanf(s_vertex_0.c_str(), "%d/%d/%d", &pos_idx, &tex_idx, &norm_idx);
					// We have to use index - 1 because the obj index starts at 1
					Vertex vertex_0;
					vertex_0.Position = positions[pos_idx - 1];
					vertex_0.TexCoords = tex_coords[tex_idx - 1];
					vertex_0.Normal = normals[norm_idx - 1];
					sscanf(s_vertex_1.c_str(), "%d/%d/%d", &pos_idx, &tex_idx, &norm_idx);

					Vertex vertex_1;
					vertex_1.Position = positions[pos_idx - 1];
					vertex_1.TexCoords = tex_coords[tex_idx - 1];
					vertex_1.Normal = normals[norm_idx - 1];
					sscanf(s_vertex_2.c_str(), "%d/%d/%d", &pos_idx, &tex_idx, &norm_idx);

					Vertex vertex_2;
					vertex_2.Position = positions[pos_idx - 1];
					vertex_2.TexCoords = tex_coords[tex_idx - 1];
					vertex_2.Normal = normals[norm_idx - 1];
					Vertices.push_back(vertex_0);
					Vertices.push_back(vertex_1);
					Vertices.push_back(vertex_2);
				}
			}
		}
		catch (const std::exception &)
		{
			std::cout << "ERROR: Obj file cannot be read.\n";
			return false;
		}
		return true;
	}
};