#pragma once

#include "object.h"

class OcclusionTree : public ObjectGL
{
public:
   OcclusionTree();
   ~OcclusionTree() override = default;

   void createOcclusionTree(const std::string& obj_file_path);
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

   [[nodiscard]] bool readObjectFile(
      std::vector<glm::vec3>& vertices,
      std::vector<glm::vec3>& normals,
      const std::string& file_path
   );

};