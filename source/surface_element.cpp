#include "surface_element.h"

SurfaceElement::SurfaceElement() : ObjectGL(), IDNum( 0 )
{
}

SurfaceElement::~SurfaceElement()
{
}

void SurfaceElement::prepareAmbientOcclusion() const
{
   constexpr GLuint ambient_occlusion_loc = 3;
   glVertexArrayAttribFormat( VAO, ambient_occlusion_loc, 3, GL_FLOAT, GL_FALSE, 8 * sizeof( GLfloat ) );
   glEnableVertexArrayAttrib( VAO, ambient_occlusion_loc );
   glVertexArrayAttribBinding( VAO, ambient_occlusion_loc, 0 );
}

float SurfaceElement::getTriangleArea(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2)
{
#if 0
   const float a = glm::distance( v0, v1 );
   const float b = glm::distance( v1, v2 );
   const float c = glm::distance( v2, v0 );
   const float h = (a + b + c) * 0.5f;
   return std::sqrt( h * (h - a) * (h - b) * (h - c) );
#else
   return glm::length( glm::cross( v1 - v0, v2 - v0 ) ) * 0.5f;
#endif
}

void SurfaceElement::setVertexList(
   const std::vector<glm::vec3>& vertices,
   const std::vector<glm::vec2>& textures,
   const std::vector<GLuint>& vertex_indices,
   const std::vector<GLuint>& texture_indices
)
{
   assert( vertex_indices.size() % 3 == 0 );
   assert( vertex_indices.size() == texture_indices.size() );

   VertexList.clear();
   VertexList.reserve( vertices.size() );
   for (const auto& v : vertices) VertexList.emplace_back( v );
   const auto size = static_cast<int>(vertex_indices.size());
   for (int i = 0; i < size; i += 3) {
      const GLuint v0 = vertex_indices[i];
      const GLuint v1 = vertex_indices[i + 1];
      const GLuint v2 = vertex_indices[i + 2];
      const float area = getTriangleArea( vertices[v0], vertices[v1], vertices[v2] ) / 3.0f;
      const glm::vec3 normal = glm::cross( vertices[v1] - vertices[v0], vertices[v2] - vertices[v0] );
      VertexList[v0].Area += area;
      VertexList[v0].Normal += normal;
      VertexList[v1].Area += area;
      VertexList[v1].Normal += normal;
      VertexList[v2].Area += area;
      VertexList[v2].Normal += normal;
   }
   for (auto& v : VertexList) glm::normalize( v.Normal );

   int id = 0;
   std::vector<int> visit(textures.size(), 0);
   for (int i = 0; i < static_cast<int>(textures.size()); ++i) {
      if (visit[i] != 0) continue;

      id++;
      visit[i] = id;
      bool is_changed = true;
      while (is_changed) {
         is_changed = false;
         for (int j = 0; j < size; j += 3) {
            if (visit[texture_indices[j]] != id &&
                visit[texture_indices[j + 1]] != id &&
                visit[texture_indices[j + 2]] != id) continue;

            for (int k = 0; k < 3; ++k) {
               if (visit[texture_indices[j + k]] == 0 || VertexList[vertex_indices[j + k]].ID == 0) {
                  visit[texture_indices[j + k]] = id;
                  VertexList[vertex_indices[j + k]].ID = id;
                  VertexList[vertex_indices[j + k]].Texture = textures[texture_indices[j + k]];
                  is_changed = true;
               }
            }
         }
      }
   }
   IDNum = id;
}

bool SurfaceElement::setVertexListFromObjectFile(
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

   setVertexList( vertex_buffer, texture_buffer, vertex_indices, texture_indices );

   for (size_t i = 0; i < vertex_indices.size(); ++i) {
      vertices.emplace_back( VertexList[vertex_indices[i]].Position );
      normals.emplace_back( VertexList[vertex_indices[i]].Normal );
      //textures.emplace_back( VertexList[vertex_indices[i]].Texture );
      textures.emplace_back( texture_buffer[texture_indices[i]] );
   }
   return true;
}

std::list<SurfaceElement::Element> SurfaceElement::getElementList(int id)
{
   std::list<Element> element_list;
   for (const auto& vertex : VertexList) {
      if (vertex.ID == id) {
         element_list.emplace_back( vertex.Position, vertex.Normal, vertex.Texture, vertex.Area );
      }
   }
   return element_list;
}

void SurfaceElement::getBoundary(glm::vec2& min_point, glm::vec2& max_point, const std::list<Element>& element_list)
{
   min_point = glm::vec2(std::numeric_limits<float>::max());
   max_point = glm::vec2(std::numeric_limits<float>::lowest());
   for (const auto& e : element_list) {
      if (e.Texture.x < min_point.x) min_point.x = e.Texture.x;
      if (e.Texture.y < min_point.y) min_point.y = e.Texture.y;

      if (e.Texture.x > max_point.x) max_point.x = e.Texture.x;
      if (e.Texture.y > max_point.y) max_point.y = e.Texture.y;
   }
}

float SurfaceElement::getMedian(const std::list<Element>& element_list, int dimension, float left, float right)
{
   float median;
   int count = 0;
   int iteration = 20;
   const auto n = static_cast<int>(element_list.size());
   const int h0 = n / 2;
   const int h1 = n & 1 ? h0 + 1 : h0;
   do {
      count = 0;
      median = (left + right) * 0.5f;
      for (const auto& e : element_list) {
         if (e.Texture[dimension] < median) count++;
      }
      if (count < h0) left = median;
      else right = median;
   } while (count != h0 && count != h1 && iteration-- > 0);
   return median;
}

std::unique_ptr<SurfaceElement::Element> SurfaceElement::createElementTree(std::list<Element>& element_list)
{
   if (element_list.empty()) return nullptr;
   if (element_list.size() == 1) {
      auto it = element_list.begin();
      auto leaf_node = std::make_unique<Element>( it->Position, it->Normal, it->Texture, it->Area );
      return leaf_node;
   }

   glm::vec2 min_point, max_point;
   getBoundary( min_point, max_point, element_list );

   const int dimension = max_point.x - min_point.x < max_point.y - min_point.y ? 1 : 0;
   const float median = dimension == 0 ?
      getMedian( element_list, dimension, min_point.x, max_point.x ) :
      getMedian( element_list, dimension, min_point.y, max_point.y );

   std::list<Element> left, right;
   for (auto it = element_list.begin(); it != element_list.end();) {
      if (it->Texture[dimension] < median) left.emplace_front( it->Position, it->Normal, it->Texture, it->Area );
      else right.emplace_front( it->Position, it->Normal, it->Texture, it->Area );
      it = element_list.erase( it );
   }
   if (left.empty()) {
      auto it = right.begin();
      left.emplace_front( it->Position, it->Normal, it->Texture, it->Area );
      right.erase( it );
   }
   else if (right.empty()) {
      auto it = left.begin();
      right.emplace_front( it->Position, it->Normal, it->Texture, it->Area );
      left.erase( it );
   }

   std::unique_ptr<Element> root = std::make_unique<Element>();
   root->Child = createElementTree( left );
   root->Right = createElementTree( right );
   return root;
}

void SurfaceElement::relocateElementTree(Element* element)
{
   if (element == nullptr) return;

   int child_num = 0;
   std::array<std::unique_ptr<Element>, 5> children{};
   if (element->Child != nullptr) {
      if (element->Child->Child == nullptr && element->Child->Right == nullptr) {
         children[child_num] = std::move( element->Child );
         child_num++;
      }
      else {
         if (element->Child->Child != nullptr) {
            children[child_num] = std::move( element->Child->Child );
            child_num++;
         }
         if (element->Child->Right != nullptr) {
            children[child_num] = std::move( element->Child->Right );
            child_num++;
         }
         element->Child.reset();
      }
   }
   if (element->Right != nullptr) {
      if (element->Right->Child == nullptr && element->Right->Right == nullptr) {
         children[child_num] = std::move( element->Right );
         child_num++;
      }
      else {
         if (element->Right->Child != nullptr) {
            children[child_num] = std::move( element->Right->Child );
            child_num++;
         }
         if (element->Right->Right != nullptr) {
            children[child_num] = std::move( element->Right->Right );
            child_num++;
         }
         element->Right.reset();
      }
   }

   Element* curr = children[0].get();
   for (int i = 0; i < child_num; ++i) {
      curr->Next = std::move( children[i + 1] );
      curr = curr->Next.get();
      relocateElementTree( curr );
   }
   element->Child = std::move( children[0] );
}

void SurfaceElement::linkTree(Element* element, Element* next)
{
   while (true) {
      if (element->Child != nullptr) {
         linkTree( element->Child.get(), element->Next != nullptr ? element->Next.get() : next );
      }
      if (element->Next == nullptr) {
         element->Next = std::make_unique<Element>( next );
         break;
      }
      element = element->Next.get();
   }
}

void SurfaceElement::createSurfaceElements(const std::string& obj_file_path, const std::string& texture_file_name)
{
   DrawMode = GL_TRIANGLES;
   std::vector<glm::vec3> vertices, normals;
   std::vector<glm::vec2> textures;
   if (!setVertexListFromObjectFile( vertices, normals, textures, obj_file_path )) return;

   for (size_t i = 0; i < vertices.size(); ++i) {
      DataBuffer.push_back( vertices[i].x );
      DataBuffer.push_back( vertices[i].y );
      DataBuffer.push_back( vertices[i].z );
      DataBuffer.push_back( normals[i].x );
      DataBuffer.push_back( normals[i].y );
      DataBuffer.push_back( normals[i].z );
      DataBuffer.push_back( textures[i].x );
      DataBuffer.push_back( textures[i].y );
      DataBuffer.push_back( 1.0f ); // for ambient occlusion
      VerticesCount++;
   }
   constexpr int n = 9;
   const auto n_bytes_per_vertex = static_cast<int>(n * sizeof( GLfloat ));
   prepareVertexBuffer( n_bytes_per_vertex );
   prepareNormal();
   prepareTexture( true );
   addTexture( texture_file_name );
   prepareAmbientOcclusion();

   Element* ptr = nullptr;
   for (int i = 0; i < IDNum; ++i) {
      std::list<Element> element_list = getElementList( i + 1 );
      std::unique_ptr<Element> sub_root = createElementTree( element_list );
      relocateElementTree( sub_root.get() );
      if (ptr == nullptr) {
         ElementTree = std::move( sub_root );
         ptr = ElementTree.get();
      }
      else {
         ptr->Next = std::move( sub_root );
         ptr = ptr->Next.get();
      }
   }

   std::list<Element> element_list = getElementList( 0 );
   if (!element_list.empty()) {
      Element* node = nullptr;
      std::unique_ptr<Element> head;
      for (auto& element : element_list) {
         auto e = std::make_unique<Element>( element.Position, element.Normal, element.Texture, element.Area );
         if (node == nullptr) {
            head = std::move( e );
            node = head.get();
         }
         else {
            node->Next = std::move( e );
            node = node->Next.get();
         }
      }
      if (ptr == nullptr) ElementTree = std::move( head );
      else ptr->Next = std::move( head );
   }
   element_list.clear();

   linkTree( ElementTree.get(), nullptr );
}