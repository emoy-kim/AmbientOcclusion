#include "occlusion_tree.h"

OcclusionTree::OcclusionTree() :
   ObjectGL()
{
}

bool OcclusionTree::readObjectFile(
   std::vector<glm::vec3>& vertices,
   std::vector<glm::vec3>& normals,
   const std::string& file_path
)
{
   std::ifstream file(file_path);
   if (!file.is_open()) {
      std::cout << "The object file is not correct.\n";
      return false;
   }

   std::vector<glm::vec3> vertex_buffer, normal_buffer;
   std::vector<glm::vec2> texture_buffer;
   std::vector<GLuint> vertex_indices, normal_indices, texture_indices;
   while (!file.eof()) {
      std::string word;
      file >> word;

      if (word == "v") {
         glm::vec3 vertex;
         file >> vertex.x >> vertex.y >> vertex.z;
         vertex_buffer.emplace_back( vertex );
      }
      else if (word == "vt") {
         glm::vec2 uv;
         file >> uv.x >> uv.y;
         texture_buffer.emplace_back( uv );
      }
      else if (word == "vn") {
         glm::vec3 normal;
         file >> normal.x >> normal.y >> normal.z;
         normal_buffer.emplace_back( normal );
      }
      else if (word == "f") {
         std::string face;
         const std::regex delimiter("[/]");
         for (int i = 0; i < 3; ++i) {
            file >> face;
            const std::sregex_token_iterator it(face.begin(), face.end(), delimiter, -1);
            const std::vector<std::string> vtn(it, std::sregex_token_iterator());
            vertex_indices.emplace_back( std::stoi( vtn[0] ) - 1 );
            texture_indices.emplace_back( std::stoi( vtn[1] ) - 1 );
            normal_indices.emplace_back( std::stoi( vtn[2] ) - 1 );
         }
      }
      else std::getline( file, word );
   }

   normals.resize( vertex_buffer.size(), glm::vec3(0.0f) );
   const auto size = static_cast<int>(vertex_indices.size());
   for (int i = 0; i < size; i += 3) {
      const GLuint v0 = vertex_indices[i];
      const GLuint v1 = vertex_indices[i + 1];
      const GLuint v2 = vertex_indices[i + 2];
      const glm::vec3 n = glm::cross( vertex_buffer[v1] - vertex_buffer[v0], vertex_buffer[v2] - vertex_buffer[v0] );
      normals[v0] += n;
      normals[v1] += n;
      normals[v2] += n;
   }
   for (auto& n : normals) n = glm::normalize( n );
   vertices = std::move( vertex_buffer );
   IndexBuffer = std::move( vertex_indices );
   return true;
}

void OcclusionTree::createOcclusionTree(const std::string& obj_file_path)
{
   DrawMode = GL_TRIANGLES;
   std::vector<glm::vec3> vertices, normals;
   if (!readObjectFile( vertices, normals, obj_file_path )) return;

   for (int i = 0; i < static_cast<int>(vertices.size()); ++i) {
      DataBuffer.emplace_back( vertices[i].x );
      DataBuffer.emplace_back( vertices[i].y );
      DataBuffer.emplace_back( vertices[i].z );
      DataBuffer.emplace_back( normals[i].x );
      DataBuffer.emplace_back( normals[i].y );
      DataBuffer.emplace_back( normals[i].z );
      VerticesCount++;
   }
   vertices.clear();
   normals.clear();

   const auto n_bytes_per_vertex = static_cast<int>(6 * sizeof( GLfloat ));
   prepareVertexBuffer( n_bytes_per_vertex );
   prepareNormal();
   prepareIndexBuffer();
}