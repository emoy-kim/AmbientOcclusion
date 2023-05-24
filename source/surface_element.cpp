#include "surface_element.h"

SurfaceElement::SurfaceElement() :
   ObjectGL(), IDNum( 0 ), TotalElementSize( 0 ), ReceiversBuffer( 0 ), SurfaceElementsBuffer( 0 )
{
}

void SurfaceElement::prepareBentNormal()
{
   constexpr GLuint bent_normal_loc = 2;
   glVertexArrayAttribFormat( VAO, bent_normal_loc, 3, GL_FLOAT, GL_FALSE, 6 * sizeof( GLfloat ) );
   glEnableVertexArrayAttrib( VAO, bent_normal_loc );
   glVertexArrayAttribBinding( VAO, bent_normal_loc, 0 );
}

void SurfaceElement::prepareAccessibility()
{
   constexpr GLuint accessibility_loc = 3;
   glVertexArrayAttribFormat( VAO, accessibility_loc, 1, GL_FLOAT, GL_FALSE, 9 * sizeof( GLfloat ) );
   glEnableVertexArrayAttrib( VAO, accessibility_loc );
   glVertexArrayAttribBinding( VAO, accessibility_loc, 0 );
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
   for (auto& v : VertexList) v.Normal = glm::normalize( v.Normal );

   // use the texture coordinates to separate vertices and construct hierarchy.
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
                  VertexList[vertex_indices[j + k]].Separator = textures[texture_indices[j + k]];
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

   // render with glDrawElements for efficiency, but some .obj files have the different sizes of vertex and texture
   // coordinates. (which means vertex_buffer.size() != texture_buffer.size())
   // skip the texture setup although the texture coordinates is used as a separator.
   for (const auto& v : VertexList) {
      vertices.emplace_back( v.Position );
      normals.emplace_back( v.Normal );
   }
   IndexBuffer = std::move( vertex_indices );
   return true;
}

std::shared_ptr<SurfaceElement::Element> SurfaceElement::getElementList(int id)
{
   std::shared_ptr<Element> head, ptr;
   for (const auto& v : VertexList) {
      if (v.ID == id) {
         auto e = std::make_shared<Element>( v.Position, v.Normal, v.Separator, v.Area );
         if (head == nullptr) head = e;
         else ptr->Next = e;
         ptr = e;
      }
   }
   return head;
}

void SurfaceElement::getBoundary(glm::vec2& min_point, glm::vec2& max_point, std::shared_ptr<Element>& element_list)
{
   min_point = glm::vec2(std::numeric_limits<float>::max());
   max_point = glm::vec2(std::numeric_limits<float>::lowest());
   for (auto ptr = element_list; ptr != nullptr; ptr = ptr->Next) {
      if (ptr->Separator.x < min_point.x) min_point.x = ptr->Separator.x;
      if (ptr->Separator.y < min_point.y) min_point.y = ptr->Separator.y;

      if (ptr->Separator.x > max_point.x) max_point.x = ptr->Separator.x;
      if (ptr->Separator.y > max_point.y) max_point.y = ptr->Separator.y;
   }
}

float SurfaceElement::getMedian(
   std::shared_ptr<Element>& element_list,
   int dimension,
   int n,
   float left,
   float right
)
{
   float median;
   int count;
   int iteration = 20;
   const int h0 = n / 2;
   const int h1 = n & 1 ? h0 + 1 : h0;
   do {
      count = 0;
      median = (left + right) * 0.5f;
      for (auto ptr = element_list; ptr != nullptr; ptr = ptr->Next) {
         if (ptr->Separator[dimension] < median) count++;
      }
      if (count < h0) left = median;
      else right = median;
   } while (count != h0 && count != h1 && iteration-- > 0);
   return median;
}

// create a tree which has child/right subtrees. The original element_list elements will be leaf nodes of this tree.
std::shared_ptr<SurfaceElement::Element> SurfaceElement::createElementTree(std::shared_ptr<Element>& element_list)
{
   if (element_list == nullptr || element_list->Next == nullptr) return element_list;

   glm::vec2 min_point, max_point;
   getBoundary( min_point, max_point, element_list );

   int n = 0;
   for (auto ptr = element_list; ptr != nullptr; ptr = ptr->Next) n++;

   const int dimension = max_point.x - min_point.x < max_point.y - min_point.y ? 1 : 0;
   const float median = dimension == 0 ?
      getMedian( element_list, dimension, n, min_point.x, max_point.x ) :
      getMedian( element_list, dimension, n, min_point.y, max_point.y );

   std::shared_ptr<Element> left, right;
   for (auto ptr = element_list; ptr != nullptr;) {
      std::shared_ptr<Element> next = ptr->Next;
      if (ptr->Separator[dimension] < median) {
         ptr->Next = left;
         left = ptr;
      }
      else {
         ptr->Next = right;
         right = ptr;
      }
      ptr = next;
   }
   if (left == nullptr) {
      left = right;
      right = right->Next;
      left->Next = nullptr;
   }
   else if (right == nullptr) {
      right = left;
      left = left->Next;
      right->Next = nullptr;
   }

   auto root = std::make_shared<Element>();
   root->Child = createElementTree( left );
   root->Right = createElementTree( right );
   return root;
}

// relocate the tree so that it has only child/next nodes.
void SurfaceElement::relocateElementTree(std::shared_ptr<Element>& element)
{
   if (element == nullptr) return;

   int child_num = 0;
   std::array<std::shared_ptr<Element>, 5> children{};
   if (element->Child != nullptr) {
      if (element->Child->Child == nullptr && element->Child->Right == nullptr) {
         children[child_num] = element->Child;
         child_num++;
      }
      else {
         if (element->Child->Child != nullptr) {
            children[child_num] = element->Child->Child;
            child_num++;
         }
         if (element->Child->Right != nullptr) {
            children[child_num] = element->Child->Right;
            child_num++;
         }
         element->Child = nullptr;
      }
   }
   if (element->Right != nullptr) {
      if (element->Right->Child == nullptr && element->Right->Right == nullptr) {
         children[child_num] = element->Right;
         child_num++;
      }
      else {
         if (element->Right->Child != nullptr) {
            children[child_num] = element->Right->Child;
            child_num++;
         }
         if (element->Right->Right != nullptr) {
            children[child_num] = element->Right->Right;
            child_num++;
         }
         element->Right = nullptr;
      }
   }

   for (int i = 0; i < child_num; ++i) {
      children[i]->Next = children[i + 1];
      relocateElementTree( children[i] );
   }
   element->Child = children[0];
}

// link the last one of a node's children to its next node,
// so that all the nodes of the tree can be traversed using only child/next nodes.
void SurfaceElement::linkTree(std::shared_ptr<Element>& element, const std::shared_ptr<Element>& next)
{
   std::shared_ptr<Element> ptr = element;
   while (true) {
      if (ptr->Child != nullptr) {
         linkTree( ptr->Child, ptr->Next != nullptr ? ptr->Next : next );
      }
      if (ptr->Next == nullptr) {
         ptr->Next = next;
         break;
      }
      ptr = ptr->Next;
   }
}

// currently, only the leaf nodes have the information such as area, position, and normal.
// fill the other nodes with information from their children.
void SurfaceElement::updateAllElements()
{
   TotalElementSize = 0;
   int height = 0;
   int index = ElementTree == nullptr ? 0 : 1;
   std::shared_ptr<Element> ptr;
   for (ptr = ElementTree; ptr != nullptr; ptr = ptr->Child != nullptr ? ptr->Child : ptr->Next) {
      TotalElementSize++;
      if (ptr->Child == nullptr) {
         ptr->Height = height;
         ptr->Index = index;
         index++;
      }
   }

   bool done = false;
   while (!done) {
      done = true;
      for (ptr = ElementTree; ptr != nullptr; ptr = ptr->Child != nullptr ? ptr->Child : ptr->Next) {
         if (ptr->Height >= 0) continue;

         done = false;
         int child_num = 0;
         glm::vec3 position(0.0f);
         glm::vec3 normal(0.0f);
         float area_sum = 0.0f;
         std::shared_ptr<Element> next = ptr->Child;
         while (next != nullptr && next != ptr->Next && next->Height >= 0) {
            position += next->Position;
            normal += next->Normal;
            area_sum += next->Area;
            next = next->Next;
            child_num++;
         }
         if (next == nullptr || next == ptr->Next) {
            ptr->Position = position / static_cast<float>(child_num);
            ptr->Normal = glm::normalize( normal );
            ptr->Area = area_sum;
            ptr->Height = height;
            ptr->Index = ptr == ElementTree ? 0 : index++;
         }
      }
      height++;
   }
}

void SurfaceElement::createSurfaceElements(const std::string& obj_file_path)
{
   DrawMode = GL_TRIANGLES;
   std::vector<glm::vec3> vertices, normals;
   if (!setVertexListFromObjectFile( vertices, normals, obj_file_path )) return;

   for (int i = 0; i < static_cast<int>(vertices.size()); ++i) {
      DataBuffer.emplace_back( vertices[i].x );
      DataBuffer.emplace_back( vertices[i].y );
      DataBuffer.emplace_back( vertices[i].z );
      DataBuffer.emplace_back( normals[i].x );
      DataBuffer.emplace_back( normals[i].y );
      DataBuffer.emplace_back( normals[i].z );
      DataBuffer.emplace_back( normals[i].x ); // for bent normal
      DataBuffer.emplace_back( normals[i].y ); // for bent normal
      DataBuffer.emplace_back( normals[i].z ); // for bent normal
      DataBuffer.emplace_back( 1.0f ); // for ambient occlusion
      VerticesCount++;
   }
   vertices.clear();
   normals.clear();

   const auto n_bytes_per_vertex = static_cast<int>(10 * sizeof( GLfloat ));
   prepareVertexBuffer( n_bytes_per_vertex );
   prepareNormal();
   prepareBentNormal();
   prepareAccessibility();
   prepareIndexBuffer();

   ElementTree.reset();
   std::shared_ptr<Element> ptr;
   for (int i = 0; i < IDNum; ++i) {
      std::shared_ptr<Element> element_list = getElementList( i + 1 );
      std::shared_ptr<Element> root = createElementTree( element_list );
      relocateElementTree( root );
      if (ElementTree == nullptr) ElementTree = root;
      else ptr->Next = root;
      ptr = root;
   }

   std::shared_ptr<Element> unmapped_list = getElementList( 0 );
   if (unmapped_list != nullptr) {
      if (ptr == nullptr) ElementTree = unmapped_list;
      else ptr->Next = unmapped_list;
   }

   linkTree( ElementTree, nullptr );
   updateAllElements();
}

void SurfaceElement::setBuffer()
{
   assert( VBO != 0 );

   ReceiversBuffer = VBO;
   addCustomBufferObject<ElementForShader>( "surface_elements", TotalElementSize );
   SurfaceElementsBuffer = getCustomBufferID( "surface_elements" );

   ElementBuffer.clear();
   ElementBuffer.resize( TotalElementSize );
   for (auto ptr = ElementTree; ptr != nullptr; ptr = ptr->Child != nullptr ? ptr->Child : ptr->Next) {
      const int i = ptr->Index;
      ElementBuffer[i].Position = ptr->Position;
      ElementBuffer[i].Normal = ptr->Normal;
      ElementBuffer[i].AreaOverPi = ptr->Area / glm::pi<float>();
      ElementBuffer[i].NextIndex = ptr->Next != nullptr ? ptr->Next->Index : -1;
      ElementBuffer[i].ChildIndex = ptr->Child != nullptr ? ptr->Child->Index : -1;
   }
   glNamedBufferSubData(
      SurfaceElementsBuffer, 0,
      static_cast<GLsizei>(TotalElementSize * sizeof( ElementForShader )),
      ElementBuffer.data()
   );
}