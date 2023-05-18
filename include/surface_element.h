#pragma once

#include "object.h"

class SurfaceElement : public ObjectGL
{
public:
   SurfaceElement();
   ~SurfaceElement() override;

   [[nodiscard]] GLuint getSurfaceElementsBuffer() const { return SurfaceElementsBuffer; }
   [[nodiscard]] GLuint getAmbientOcclusionBuffer() const { return AmbientOcclusionBuffer; }
   void createSurfaceElements(const std::string& obj_file_path, const std::string& texture_file_name);
   void setBuffer();

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
      int Height;
      int MapIndex;
      int VertexListIndex;
      glm::vec3 Position;
      glm::vec3 Normal;
      glm::vec2 Texture;
      std::shared_ptr<Element> Next;
      std::shared_ptr<Element> Right;
      std::shared_ptr<Element> Child;

      Element() :
         Area( 0.0f ), Height( -1 ), MapIndex( -1 ), VertexListIndex( -1 ), Position( 0.0f ), Normal( 0.0f ),
         Texture( 0.0f ) {}
      Element(glm::vec3 position, glm::vec3 normal, glm::vec2 texture, float area, int vertex_list_index) :
         Area( area ), Height( -1 ), MapIndex( -1 ), VertexListIndex( vertex_list_index ), Position( position ),
         Normal( normal ), Texture( texture ) {}
   };

   struct ElementForShader
   {
      int NextIndex;
      int ChildIndex;
      int OutIndex;
      float Area;
      glm::vec3 Position;
      glm::vec3 Normal;
      glm::vec2 Texture;

      ElementForShader() = default;
   };

   int IDNum;
   int TotalElementSize;
   GLuint SurfaceElementsBuffer;
   GLuint AmbientOcclusionBuffer;
   std::vector<Vertex> VertexList;
   std::shared_ptr<Element> ElementTree;

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
   [[nodiscard]] std::shared_ptr<Element> getElementList(int id);
   static void getBoundary(glm::vec2& min_point, glm::vec2& max_point, std::shared_ptr<Element>& element_list);
   static float getMedian(std::shared_ptr<Element>& element_list, int dimension, int n, float left, float right);
   [[nodiscard]] std::shared_ptr<Element> createElementTree(std::shared_ptr<Element>& element_list);
   static void relocateElementTree(std::shared_ptr<Element>& element);
   static void linkTree(std::shared_ptr<Element>& element, const std::shared_ptr<Element>& next);
};