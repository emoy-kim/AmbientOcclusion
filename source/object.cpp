#include "object.h"

ObjectGL::ObjectGL() :
   AdjacencyMode( false ), VAO( 0 ), VBO( 0 ), IBO( 0 ), DrawMode( 0 ), VerticesCount( 0 ),
   EmissionColor( 0.0f, 0.0f, 0.0f, 1.0f ), AmbientReflectionColor( 0.2f, 0.2f, 0.2f, 1.0f ),
   DiffuseReflectionColor( 0.8f, 0.8f, 0.8f, 1.0f ), SpecularReflectionColor( 0.0f, 0.0f, 0.0f, 1.0f ),
   SpecularReflectionExponent( 0.0f )
{
}

ObjectGL::~ObjectGL()
{
   if (IBO != 0) glDeleteBuffers( 1, &IBO );
   if (VBO != 0) glDeleteBuffers( 1, &VBO );
   if (VAO != 0) glDeleteVertexArrays( 1, &VAO );
   for (const auto& texture_id : TextureID) {
      if (texture_id != 0) glDeleteTextures( 1, &texture_id );
   }
   for (const auto& buffer : CustomBuffers) {
      if (buffer.second != 0) glDeleteBuffers( 1, &buffer.second );
   }
}

void ObjectGL::setEmissionColor(const glm::vec4& emission_color)
{
   EmissionColor = emission_color;
}

void ObjectGL::setAmbientReflectionColor(const glm::vec4& ambient_reflection_color)
{
   AmbientReflectionColor = ambient_reflection_color;
}

void ObjectGL::setDiffuseReflectionColor(const glm::vec4& diffuse_reflection_color)
{
   DiffuseReflectionColor = diffuse_reflection_color;
}

void ObjectGL::setSpecularReflectionColor(const glm::vec4& specular_reflection_color)
{
   SpecularReflectionColor = specular_reflection_color;
}

void ObjectGL::setSpecularReflectionExponent(const float& specular_reflection_exponent)
{
   SpecularReflectionExponent = specular_reflection_exponent;
}

bool ObjectGL::prepareTexture2DUsingFreeImage(const std::string& file_path, bool is_grayscale) const
{
   const FREE_IMAGE_FORMAT format = FreeImage_GetFileType( file_path.c_str(), 0 );
   FIBITMAP* texture = FreeImage_Load( format, file_path.c_str() );
   if (!texture) return false;

   FIBITMAP* texture_converted;
   const uint n_bits_per_pixel = FreeImage_GetBPP( texture );
   const uint n_bits = is_grayscale ? 8 : 32;
   if (is_grayscale) {
      texture_converted = n_bits_per_pixel == n_bits ? texture : FreeImage_GetChannel( texture, FICC_RED );
   }
   else {
      texture_converted = n_bits_per_pixel == n_bits ? texture : FreeImage_ConvertTo32Bits( texture );
   }

   const auto width = static_cast<GLsizei>(FreeImage_GetWidth( texture_converted ));
   const auto height = static_cast<GLsizei>(FreeImage_GetHeight( texture_converted ));
   GLvoid* data = FreeImage_GetBits( texture_converted );
   glTextureStorage2D( TextureID.back(), 1, is_grayscale ? GL_R8 : GL_RGBA8, width, height );
   glTextureSubImage2D( TextureID.back(), 0, 0, 0, width, height, is_grayscale ? GL_RED : GL_BGRA, GL_UNSIGNED_BYTE, data );

   FreeImage_Unload( texture_converted );
   if (n_bits_per_pixel != n_bits) FreeImage_Unload( texture );
   return true;
}

int ObjectGL::addTexture(const std::string& texture_file_path, bool is_grayscale)
{
   GLuint texture_id = 0;
   glCreateTextures( GL_TEXTURE_2D, 1, &texture_id );
   TextureID.emplace_back( texture_id );
   if (!prepareTexture2DUsingFreeImage( texture_file_path, is_grayscale )) {
      glDeleteTextures( 1, &texture_id );
      TextureID.erase( TextureID.end() - 1 );
      std::cerr << "Could not read image file " << texture_file_path.c_str() << "\n";
      return -1;
   }

   glTextureParameteri( texture_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
   glTextureParameteri( texture_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
   glTextureParameteri( texture_id, GL_TEXTURE_WRAP_S, GL_REPEAT );
   glTextureParameteri( texture_id, GL_TEXTURE_WRAP_T, GL_REPEAT );
   glGenerateTextureMipmap( texture_id );
   return static_cast<int>(TextureID.size() - 1);
}

void ObjectGL::addTexture(int width, int height, bool is_grayscale)
{
   GLuint texture_id = 0;
   glCreateTextures( GL_TEXTURE_2D, 1, &texture_id );
   glTextureStorage2D(
      texture_id, 1,
      is_grayscale ? GL_R8 : GL_RGBA8,
      width, height
   );
   glTextureParameteri( texture_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
   glTextureParameteri( texture_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
   glTextureParameteri( texture_id, GL_TEXTURE_WRAP_S, GL_REPEAT );
   glTextureParameteri( texture_id, GL_TEXTURE_WRAP_T, GL_REPEAT );
   glGenerateTextureMipmap( texture_id );
   TextureID.emplace_back( texture_id );
}

int ObjectGL::addTexture(const uint8_t* image_buffer, int width, int height, bool is_grayscale)
{
   addTexture( width, height, is_grayscale );
   glTextureSubImage2D(
      TextureID.back(), 0, 0, 0,
      width, height,
      is_grayscale ? GL_RED : GL_RGBA,
      GL_UNSIGNED_BYTE,
      image_buffer
   );
   return static_cast<int>(TextureID.size() - 1);
}

void ObjectGL::prepareTexture(bool normals_exist) const
{
   const uint offset = normals_exist ? 6 : 3;
   glVertexArrayAttribFormat( VAO, TextureLoc, 2, GL_FLOAT, GL_FALSE, offset * sizeof( GLfloat ) );
   glEnableVertexArrayAttrib( VAO, TextureLoc );
   glVertexArrayAttribBinding( VAO, TextureLoc, 0 );
}

void ObjectGL::prepareNormal() const
{
   glVertexArrayAttribFormat( VAO, NormalLoc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof( GLfloat ) );
   glEnableVertexArrayAttrib( VAO, NormalLoc );
   glVertexArrayAttribBinding( VAO, NormalLoc, 0 );
}

void ObjectGL::prepareVertexBuffer(int n_bytes_per_vertex)
{
   glCreateBuffers( 1, &VBO );
   glNamedBufferStorage( VBO, sizeof( GLfloat ) * DataBuffer.size(), DataBuffer.data(), GL_DYNAMIC_STORAGE_BIT );

   glCreateVertexArrays( 1, &VAO );
   glVertexArrayVertexBuffer( VAO, 0, VBO, 0, n_bytes_per_vertex );
   glVertexArrayAttribFormat( VAO, VertexLoc, 3, GL_FLOAT, GL_FALSE, 0 );
   glEnableVertexArrayAttrib( VAO, VertexLoc );
   glVertexArrayAttribBinding( VAO, VertexLoc, 0 );
}

void ObjectGL::prepareIndexBuffer()
{
   assert( VAO != 0 );

   if (IBO != 0) glDeleteBuffers( 1, &IBO );

   glCreateBuffers( 1, &IBO );
   glNamedBufferStorage( IBO, sizeof( GLuint ) * IndexBuffer.size(), IndexBuffer.data(), GL_DYNAMIC_STORAGE_BIT );
   glVertexArrayElementBuffer( VAO, IBO );
}

void ObjectGL::getSquareObject(
   std::vector<glm::vec3>& vertices,
   std::vector<glm::vec3>& normals,
   std::vector<glm::vec2>& textures
)
{
   vertices = {
      { 1.0f, 0.0f, 0.0f },
      { 1.0f, 1.0f, 0.0f },
      { 0.0f, 1.0f, 0.0f },

      { 1.0f, 0.0f, 0.0f },
      { 0.0f, 1.0f, 0.0f },
      { 0.0f, 0.0f, 0.0f }
   };
   normals = {
      { 0.0f, 0.0f, 1.0f },
      { 0.0f, 0.0f, 1.0f },
      { 0.0f, 0.0f, 1.0f },

      { 0.0f, 0.0f, 1.0f },
      { 0.0f, 0.0f, 1.0f },
      { 0.0f, 0.0f, 1.0f }
   };
   textures = {
      { 1.0f, 0.0f },
      { 1.0f, 1.0f },
      { 0.0f, 1.0f },

      { 1.0f, 0.0f },
      { 0.0f, 1.0f },
      { 0.0f, 0.0f }
   };
}

void ObjectGL::setObject(GLenum draw_mode, const std::vector<glm::vec3>& vertices)
{
   DrawMode = draw_mode;
   VerticesCount = 0;
   DataBuffer.clear();
   for (auto& vertex : vertices) {
      DataBuffer.push_back( vertex.x );
      DataBuffer.push_back( vertex.y );
      DataBuffer.push_back( vertex.z );
      VerticesCount++;
   }
   const int n_bytes_per_vertex = 3 * sizeof( GLfloat );
   prepareVertexBuffer( n_bytes_per_vertex );
}

void ObjectGL::setObject(
   GLenum draw_mode,
   const std::vector<glm::vec3>& vertices,
   const std::vector<glm::vec3>& normals
)
{
   DrawMode = draw_mode;
   VerticesCount = 0;
   DataBuffer.clear();
   for (size_t i = 0; i < vertices.size(); ++i) {
      DataBuffer.push_back( vertices[i].x );
      DataBuffer.push_back( vertices[i].y );
      DataBuffer.push_back( vertices[i].z );
      DataBuffer.push_back( normals[i].x );
      DataBuffer.push_back( normals[i].y );
      DataBuffer.push_back( normals[i].z );
      VerticesCount++;
   }
   const int n_bytes_per_vertex = 6 * sizeof( GLfloat );
   prepareVertexBuffer( n_bytes_per_vertex );
   prepareNormal();
}

void ObjectGL::setObject(
   GLenum draw_mode,
   const std::vector<glm::vec3>& vertices,
   const std::vector<glm::vec2>& textures,
   const std::string& texture_file_path,
   bool is_grayscale
)
{
   DrawMode = draw_mode;
   VerticesCount = 0;
   DataBuffer.clear();
   for (size_t i = 0; i < vertices.size(); ++i) {
      DataBuffer.push_back( vertices[i].x );
      DataBuffer.push_back( vertices[i].y );
      DataBuffer.push_back( vertices[i].z );
      DataBuffer.push_back( textures[i].x );
      DataBuffer.push_back( textures[i].y );
      VerticesCount++;
   }
   const int n_bytes_per_vertex = 5 * sizeof( GLfloat );
   prepareVertexBuffer( n_bytes_per_vertex );
   prepareTexture( false );
   addTexture( texture_file_path, is_grayscale );
}

void ObjectGL::setObject(
   GLenum draw_mode,
   const std::vector<glm::vec3>& vertices,
   const std::vector<glm::vec3>& normals,
   const std::vector<glm::vec2>& textures
)
{
   DrawMode = draw_mode;
   VerticesCount = 0;
   DataBuffer.clear();
   for (size_t i = 0; i < vertices.size(); ++i) {
      DataBuffer.push_back( vertices[i].x );
      DataBuffer.push_back( vertices[i].y );
      DataBuffer.push_back( vertices[i].z );
      DataBuffer.push_back( normals[i].x );
      DataBuffer.push_back( normals[i].y );
      DataBuffer.push_back( normals[i].z );
      DataBuffer.push_back( textures[i].x );
      DataBuffer.push_back( textures[i].y );
      VerticesCount++;
   }
   const int n_bytes_per_vertex = 8 * sizeof( GLfloat );
   prepareVertexBuffer( n_bytes_per_vertex );
   prepareNormal();
   prepareTexture( true );
}

void ObjectGL::setObject(
   GLenum draw_mode,
   const std::vector<glm::vec3>& vertices,
   const std::vector<glm::vec3>& normals,
   const std::vector<glm::vec2>& textures,
   const std::string& texture_file_path,
   bool is_grayscale
)
{
   setObject( draw_mode, vertices, normals, textures );
   addTexture( texture_file_path, is_grayscale );
}

void ObjectGL::findNormals(
   std::vector<glm::vec3>& normals,
   const std::vector<glm::vec3>& vertices,
   const std::vector<GLuint>& vertex_indices
)
{
   normals.resize( vertices.size() );
   const auto size = static_cast<int>(vertex_indices.size());
   for (int i = 0; i < size; i += 3) {
      const GLuint n0 = vertex_indices[i];
      const GLuint n1 = vertex_indices[i + 1];
      const GLuint n2 = vertex_indices[i + 2];
      const glm::vec3 normal = glm::cross( vertices[n1] - vertices[n0], vertices[n2] - vertices[n0] );
      normals[n0] += normal;
      normals[n1] += normal;
      normals[n2] += normal;
   }
   for (auto& n : normals) glm::normalize( n );
}

void ObjectGL::findAdjacency(const std::vector<glm::vec3>& vertices, const std::vector<GLuint>& indices)
{
   const auto size = static_cast<int>(indices.size());

   assert( size % 3 == 0 );

   std::vector<std::array<GLuint, 3>> unique_faces;
   std::map<glm::vec3, GLuint, VectorComparison> vertex_to_index;
   std::map<std::pair<GLuint, GLuint>, std::pair<GLuint, GLuint>> edge_to_adjacent_faces;
   for (int i = 0; i < size; i += 3) {
      std::array<GLuint, 3> face{};
      for (int j = 0; j < 3; ++j) {
         GLuint index = indices[i + j];
         const glm::vec3 v = vertices[index];
         if (vertex_to_index.find( v ) != vertex_to_index.end()) index = vertex_to_index[v];
         else vertex_to_index[v] = index;
         face[j] = index;
      }
      unique_faces.emplace_back( face );

      const GLuint face_index = i / 3;
      const auto edge0 = std::make_pair( std::min( face[0], face[1] ), std::max( face[0], face[1] ) );
      const auto edge1 = std::make_pair( std::min( face[1], face[2] ), std::max( face[1], face[2] ) );
      const auto edge2 = std::make_pair( std::min( face[0], face[2] ), std::max( face[0], face[2] ) );
      if (edge_to_adjacent_faces.find( edge0 ) != edge_to_adjacent_faces.end()) {
         edge_to_adjacent_faces[edge0].second = face_index;
      }
      else edge_to_adjacent_faces[edge0] = std::make_pair( face_index, -1 );
      if (edge_to_adjacent_faces.find( edge1 ) != edge_to_adjacent_faces.end()) {
         edge_to_adjacent_faces[edge1].second = face_index;
      }
      else edge_to_adjacent_faces[edge1] = std::make_pair( face_index, -1 );
      if (edge_to_adjacent_faces.find( edge2 ) != edge_to_adjacent_faces.end()) {
         edge_to_adjacent_faces[edge2].second = face_index;
      }
      else edge_to_adjacent_faces[edge2] = std::make_pair( face_index, -1 );
   }

   const auto unique_face_size = static_cast<int>(unique_faces.size());
   IndexBuffer.reserve( unique_face_size * 6 );
   for (int i = 0; i < unique_face_size; ++i) {
      for (int j = 0; j < 3; ++j) {
         const GLuint f0 = unique_faces[i][j];
         const GLuint f1 = unique_faces[i][(j + 1) % 3];
         const auto edge = std::make_pair( std::min( f0, f1 ), std::max( f0, f1 ) );

         assert( edge_to_adjacent_faces.find( edge ) != edge_to_adjacent_faces.end() );

         const std::pair<GLuint, GLuint> faces = edge_to_adjacent_faces[edge];
         const GLuint adjacent_face_index = faces.first == i ? faces.second : faces.first;
         if (adjacent_face_index != -1) {
            for (const auto& f : unique_faces[adjacent_face_index]) {
               if (f != edge.first && f != edge.second) {
                  IndexBuffer.emplace_back( f0 );
                  IndexBuffer.emplace_back( f );
                  break;
               }
            }
         }
         else {
            IndexBuffer.emplace_back( f0 );
            IndexBuffer.emplace_back( f0 );
         }
      }
   }
}

bool ObjectGL::readObjectFile(
   std::vector<glm::vec3>& vertices,
   std::vector<glm::vec3>& normals,
   std::vector<glm::vec2>& textures,
   const std::string& file_path
)
{
   std::ifstream file(file_path);
   if (!file.is_open()) {
      std::cout << "The object file is not correct.\n";
      return false;
   }

   bool found_normals = false, found_textures = false;
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
         found_textures = true;
      }
      else if (word == "vn") {
         glm::vec3 normal;
         file >> normal.x >> normal.y >> normal.z;
         normal_buffer.emplace_back( normal );
         found_normals = true;
      }
      else if (word == "f") {
         std::string face;
         const std::regex delimiter("[/]");
         for (int i = 0; i < 3; ++i) {
            file >> face;
            const std::sregex_token_iterator it(face.begin(), face.end(), delimiter, -1);
            const std::vector<std::string> vtn(it, std::sregex_token_iterator());
            vertex_indices.emplace_back( std::stoi( vtn[0] ) - 1 );
            if (found_textures) texture_indices.emplace_back( std::stoi( vtn[1] ) - 1 );
            if (found_normals) normal_indices.emplace_back( std::stoi( vtn[2] ) - 1 );
         }
      }
      else std::getline( file, word );
   }

   if (!found_normals) findNormals( normal_buffer, vertex_buffer, vertex_indices );

   if (!AdjacencyMode) {
      for (size_t i = 0; i < vertex_indices.size(); ++i) {
         vertices.emplace_back( vertex_buffer[vertex_indices[i]] );
         if (found_normals) normals.emplace_back( normal_buffer[normal_indices[i]] );
         else normals.emplace_back( normal_buffer[vertex_indices[i]] );
         if (found_textures) textures.emplace_back( texture_buffer[texture_indices[i]] );
      }
   }
   else {
      vertices = std::move( vertex_buffer );
      normals = std::move( normal_buffer );
      if (found_textures) textures = std::move( texture_buffer );
      findAdjacency( vertices, vertex_indices );
   }
   return true;
}

void ObjectGL::setObject(GLenum draw_mode, const std::string& obj_file_path)
{
   DrawMode = draw_mode;
   AdjacencyMode = DrawMode == GL_TRIANGLES_ADJACENCY;
   std::vector<glm::vec3> vertices, normals;
   std::vector<glm::vec2> textures;
   if (!readObjectFile( vertices, normals, textures, obj_file_path )) return;

   const bool normals_exist = !normals.empty();
   const bool textures_exist = !textures.empty();
   for (uint i = 0; i < vertices.size(); ++i) {
      DataBuffer.push_back( vertices[i].x );
      DataBuffer.push_back( vertices[i].y );
      DataBuffer.push_back( vertices[i].z );
      if (normals_exist) {
         DataBuffer.push_back( normals[i].x );
         DataBuffer.push_back( normals[i].y );
         DataBuffer.push_back( normals[i].z );
      }
      if (textures_exist) {
         DataBuffer.push_back( textures[i].x );
         DataBuffer.push_back( textures[i].y );
      }
      VerticesCount++;
   }
   int n = 3;
   if (normals_exist) n += 3;
   if (textures_exist) n += 2;
   const auto n_bytes_per_vertex = static_cast<int>(n * sizeof( GLfloat ));
   prepareVertexBuffer( n_bytes_per_vertex );
   if (normals_exist) prepareNormal();
   if (textures_exist) prepareTexture( normals_exist );
   if (AdjacencyMode) prepareIndexBuffer();
}

void ObjectGL::setObject(
   GLenum draw_mode,
   const std::string& obj_file_path,
   const std::string& texture_file_name
)
{
   DrawMode = draw_mode;
   AdjacencyMode = DrawMode == GL_TRIANGLES_ADJACENCY;
   std::vector<glm::vec3> vertices, normals;
   std::vector<glm::vec2> textures;
   if (!readObjectFile( vertices, normals, textures, obj_file_path )) return;

   assert( !textures.empty() );

   const bool normals_exist = !normals.empty();
   for (uint i = 0; i < vertices.size(); ++i) {
      DataBuffer.push_back( vertices[i].x );
      DataBuffer.push_back( vertices[i].y );
      DataBuffer.push_back( vertices[i].z );
      if (normals_exist) {
         DataBuffer.push_back( normals[i].x );
         DataBuffer.push_back( normals[i].y );
         DataBuffer.push_back( normals[i].z );
      }
      DataBuffer.push_back( textures[i].x );
      DataBuffer.push_back( textures[i].y );
      VerticesCount++;
   }
   const int n_bytes_per_vertex = normals_exist ? 8 * sizeof( GLfloat ) : 5 * sizeof( GLfloat );
   prepareVertexBuffer( n_bytes_per_vertex );
   if (normals_exist) prepareNormal();
   prepareTexture( normals_exist );
   addTexture( texture_file_name );
   if (AdjacencyMode) prepareIndexBuffer();
}

void ObjectGL::setSquareObject(GLenum draw_mode, bool use_texture)
{
   std::vector<glm::vec3> square_vertices, square_normals;
   std::vector<glm::vec2> square_textures;
   getSquareObject( square_vertices, square_normals, square_textures );
   if (use_texture) setObject( draw_mode, square_vertices, square_normals, square_textures );
   else setObject( draw_mode, square_vertices, square_normals );
}

void ObjectGL::setSquareObject(
   GLenum draw_mode,
   const std::string& texture_file_path,
   bool is_grayscale
)
{
   std::vector<glm::vec3> square_vertices, square_normals;
   std::vector<glm::vec2> square_textures;
   getSquareObject( square_vertices, square_normals, square_textures );
   setObject( draw_mode, square_vertices, square_normals, square_textures, texture_file_path, is_grayscale );
}

void ObjectGL::transferUniformsToShader(const ShaderGL* shader)
{
   glUniform4fv( shader->getMaterialEmissionLocation(), 1, &EmissionColor[0] );
   glUniform4fv( shader->getMaterialAmbientLocation(), 1, &AmbientReflectionColor[0] );
   glUniform4fv( shader->getMaterialDiffuseLocation(), 1, &DiffuseReflectionColor[0] );
   glUniform4fv( shader->getMaterialSpecularLocation(), 1, &SpecularReflectionColor[0] );
   glUniform1f( shader->getMaterialSpecularExponentLocation(), SpecularReflectionExponent );
}

void ObjectGL::updateDataBuffer(const std::vector<glm::vec3>& vertices, const std::vector<glm::vec3>& normals)
{
   assert( VBO != 0 );

   VerticesCount = 0;
   DataBuffer.clear();
   for (size_t i = 0; i < vertices.size(); ++i) {
      DataBuffer.push_back( vertices[i].x );
      DataBuffer.push_back( vertices[i].y );
      DataBuffer.push_back( vertices[i].z );
      DataBuffer.push_back( normals[i].x );
      DataBuffer.push_back( normals[i].y );
      DataBuffer.push_back( normals[i].z );
      VerticesCount++;
   }
   glNamedBufferSubData( VBO, 0, static_cast<GLsizeiptr>(sizeof( GLfloat ) * DataBuffer.size()), DataBuffer.data() );
}

void ObjectGL::updateDataBuffer(
   const std::vector<glm::vec3>& vertices,
   const std::vector<glm::vec3>& normals,
   const std::vector<glm::vec2>& textures
)
{
   assert( VBO != 0 );

   VerticesCount = 0;
   DataBuffer.clear();
   for (size_t i = 0; i < vertices.size(); ++i) {
      DataBuffer.push_back( vertices[i].x );
      DataBuffer.push_back( vertices[i].y );
      DataBuffer.push_back( vertices[i].z );
      DataBuffer.push_back( normals[i].x );
      DataBuffer.push_back( normals[i].y );
      DataBuffer.push_back( normals[i].z );
      DataBuffer.push_back( textures[i].x );
      DataBuffer.push_back( textures[i].y );
      VerticesCount++;
   }
   glNamedBufferSubData( VBO, 0, static_cast<GLsizeiptr>(sizeof( GLfloat ) * DataBuffer.size()), DataBuffer.data() );
}

void ObjectGL::replaceVertices(
   const std::vector<glm::vec3>& vertices,
   bool normals_exist,
   bool textures_exist
)
{
   assert( VBO != 0 );

   VerticesCount = 0;
   int step = 3;
   if (normals_exist) step += 3;
   if (textures_exist) step += 2;
   for (size_t i = 0; i < vertices.size(); ++i) {
      DataBuffer[i * step] = vertices[i].x;
      DataBuffer[i * step + 1] = vertices[i].y;
      DataBuffer[i * step + 2] = vertices[i].z;
      VerticesCount++;
   }
   glNamedBufferSubData( VBO, 0, static_cast<GLsizeiptr>(sizeof( GLfloat ) * VerticesCount * step), DataBuffer.data() );
}

void ObjectGL::replaceVertices(
   const std::vector<float>& vertices,
   bool normals_exist,
   bool textures_exist
)
{
   assert( VBO != 0 );

   VerticesCount = 0;
   int step = 3;
   if (normals_exist) step += 3;
   if (textures_exist) step += 2;
   for (size_t i = 0, j = 0; i < vertices.size(); i += 3, ++j) {
      DataBuffer[j * step] = vertices[i];
      DataBuffer[j * step + 1] = vertices[i + 1];
      DataBuffer[j * step + 2] = vertices[i + 2];
      VerticesCount++;
   }
   glNamedBufferSubData( VBO, 0, static_cast<GLsizeiptr>(sizeof( GLfloat ) * VerticesCount * step), DataBuffer.data() );
}