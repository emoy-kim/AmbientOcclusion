#pragma once

#include "object.h"

class SurfaceElement : public ObjectGL
{
public:
   SurfaceElement();
   ~SurfaceElement() override;

   void createSurfaceElements(const std::string& obj_file_path, const std::string& texture_file_name);

private:
   struct Vertex
   {
      int ID;
      float Area;
      glm::vec3 Position;
      glm::vec3 Normal;
      glm::vec2 Texture;

      Vertex() : ID( 0 ), Area( 0.0f ), Position( 0.0f ), Normal( 0.0f ), Texture( 0.0f ) {}
      explicit Vertex(glm::vec3 position) :
         ID( 0 ), Area( 0.0f ), Position( position ), Normal( 0.0f ), Texture( 0.0f ) {}
      Vertex(glm::vec3 position, glm::vec3 normal, glm::vec2 texture, float area) :
         ID( 0 ), Area( area ), Position( position ), Normal( normal ), Texture( texture ) {}
   };

   struct Element
   {
      float Area;
      glm::vec3 Position;
      glm::vec3 Normal;
      glm::vec2 Texture;
      std::unique_ptr<Element> Next;
      std::unique_ptr<Element> Right;
      std::unique_ptr<Element> Child;

      Element() : Area( 0.0f ), Position( 0.0f ), Normal( 0.0f ), Texture( 0.0f ) {}
      Element(glm::vec3 position, glm::vec3 normal, glm::vec2 texture, float area) :
         Area( area ), Position( position ), Normal( normal ), Texture( texture ) {}
      explicit Element(Element* element) : Element()
      {
         if (element != nullptr) {
            Area = element->Area;
            Position = element->Position;
            Normal = element->Normal;
            Texture = element->Texture;
            Next = std::move( element->Next );
            Right = std::move( element->Right );
            Child = std::move( element->Child );
         }
      }
   };

   int IDNum;
   std::vector<Vertex> VertexList;
   std::unique_ptr<Element> ElementTree;

   void prepareAmbientOcclusion() const;
   [[nodiscard]] bool setVertexListFromObjectFile(
      std::vector<glm::vec3>& vertices,
      std::vector<glm::vec3>& normals,
      std::vector<glm::vec2>& textures,
      const std::string& file_path
   );
   [[nodiscard]] static float getTriangleArea(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2);
   void setVertexList(
      const std::vector<glm::vec3>& vertices,
      const std::vector<glm::vec2>& textures,
      const std::vector<GLuint>& vertex_indices,
      const std::vector<GLuint>& texture_indices
   );
   [[nodiscard]] std::list<Element> getElementList(int id);
   static void getBoundary(glm::vec2& min_point, glm::vec2& max_point, const std::list<Element>& element_list);
   static float getMedian(const std::list<Element>& element_list, int dimension, float left, float right);
   [[nodiscard]] std::unique_ptr<Element> createElementTree(std::list<Element>& element_list);
   static void relocateElementTree(Element* element);
   static void linkTree(Element* element, Element* next);
};