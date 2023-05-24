#pragma once

#include "object.h"

class SurfaceElement : public ObjectGL
{
public:
   SurfaceElement();
   ~SurfaceElement() override = default;

   [[nodiscard]] GLuint getReceiversBuffer() const { return ReceiversBuffer; }
   [[nodiscard]] GLuint getSurfaceElementsBuffer() const { return SurfaceElementsBuffer; }
   [[nodiscard]] int getVertexBufferSize() const { return static_cast<int>(VertexList.size()); }
   void createSurfaceElements(const std::string& obj_file_path);
   void setBuffer();

private:
   struct Vertex
   {
      int ID;
      float Area;
      glm::vec3 Position;
      glm::vec3 Normal;
      glm::vec2 Separator;

      Vertex() : ID( 0 ), Area( 0.0f ), Position( 0.0f ), Normal( 0.0f ), Separator( 0.0f ) {}
      explicit Vertex(glm::vec3 position) :
         ID( 0 ), Area( 0.0f ), Position( position ), Normal( 0.0f ), Separator( 0.0f ) {}
      Vertex(glm::vec3 position, glm::vec3 normal, glm::vec2 separator, float area) :
         ID( 0 ), Area( area ), Position( position ), Normal( normal ), Separator( separator ) {}
   };

   struct Element
   {
      float Area;
      int Index;
      int Height;
      glm::vec3 Position;
      glm::vec3 Normal;
      glm::vec2 Separator;
      std::shared_ptr<Element> Next;
      std::shared_ptr<Element> Right;
      std::shared_ptr<Element> Child;

      Element() : Area( 0.0f ), Index( -1 ), Height( -1 ), Position( 0.0f ), Normal( 0.0f ), Separator( 0.0f ) {}
      Element(glm::vec3 position, glm::vec3 normal, glm::vec2 separator, float area) :
         Area( area ), Index( -1 ), Height( -1 ), Position( position ), Normal( normal ), Separator( separator ) {}
   };

   struct ElementForShader
   {
      alignas(4) int NextIndex;
      alignas(4) int ChildIndex;
      alignas(4) float AreaOverPi;
      alignas(16) glm::vec3 Position;
      alignas(16) glm::vec3 Normal;

      ElementForShader() = default;
   };

   int IDNum;
   int TotalElementSize;
   GLuint ReceiversBuffer;
   GLuint SurfaceElementsBuffer;
   std::vector<Vertex> VertexList;
   std::shared_ptr<Element> ElementTree;
   std::vector<ElementForShader> ElementBuffer;

   void prepareBentNormal();
   void prepareAccessibility();
   [[nodiscard]] bool setVertexListFromObjectFile(
      std::vector<glm::vec3>& vertices,
      std::vector<glm::vec3>& normals,
      const std::string& file_path
   );
   [[nodiscard]] static float getTriangleArea(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2);
   void setVertexList(
      const std::vector<glm::vec3>& vertices,
      const std::vector<glm::vec2>& textures,
      const std::vector<GLuint>& vertex_indices,
      const std::vector<GLuint>& texture_indices
   );
   [[nodiscard]] std::shared_ptr<Element> getElementList(int id);
   static void getBoundary(glm::vec2& min_point, glm::vec2& max_point, std::shared_ptr<Element>& element_list);
   static float getMedian(std::shared_ptr<Element>& element_list, int dimension, int n, float left, float right);
   [[nodiscard]] std::shared_ptr<Element> createElementTree(std::shared_ptr<Element>& element_list);
   static void relocateElementTree(std::shared_ptr<Element>& element);
   static void linkTree(std::shared_ptr<Element>& element, const std::shared_ptr<Element>& next);
   void updateAllElements();
};