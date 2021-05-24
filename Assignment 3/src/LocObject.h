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
		// Tangent
		glm::vec3 Tangent;
		// Bitangent
		glm::vec3 Bitangent;
	};
	std::vector<Vertex> Vertices;
	std::string m_obj_path;

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

					glm::vec3 edge1 = vertex_1.Position - vertex_0.Position;
					glm::vec3 edge2 = vertex_2.Position - vertex_0.Position;
					glm::vec2 deltaUV1 = vertex_1.TexCoords - vertex_0.TexCoords;
					glm::vec2 deltaUV2 = vertex_2.TexCoords - vertex_0.TexCoords;

					float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
					glm::vec3 tangent;
					tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
					tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
					tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
					tangent = glm::normalize(tangent);

					glm::vec3 bitangent;
					bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
					bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
					bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
					bitangent = glm::normalize(bitangent);

					vertex_0.Tangent = tangent;
					vertex_1.Tangent = tangent;
					vertex_2.Tangent = tangent;
					vertex_0.Bitangent = bitangent;
					vertex_1.Bitangent = bitangent;
					vertex_2.Bitangent = bitangent;

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