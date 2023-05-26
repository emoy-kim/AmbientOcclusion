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
   inline static uint NullIndex = std::numeric_limits<uint>::max();

   struct Disk
   {
      float Area;
      float Occlusion;
      glm::vec3 Centroid;
      glm::vec3 Normal;
      uint ParentIndex;
      uint NextIndex;
      uint LeftChildIndex;
      uint RightChildIndex;

      Disk() :
         Area( 0.0f ), Occlusion( 1.0f ), Centroid( 0.0f ), Normal( 0.0f ), ParentIndex( NullIndex ),
         NextIndex( NullIndex ), LeftChildIndex( NullIndex ), RightChildIndex( NullIndex ) {}
      explicit Disk(uint parent_index) :
         Area( 0.0f ), Occlusion( 1.0f ), Centroid( 0.0f ), Normal( 0.0f ), ParentIndex( parent_index ),
         NextIndex( NullIndex ), LeftChildIndex( NullIndex ), RightChildIndex( NullIndex ) {}
   };

   uint RootIndex;
   std::vector<Disk> Disks;
   std::vector<glm::vec3> Vertices;

   [[nodiscard]] bool readObjectFile(std::vector<glm::vec3>& normals, const std::string& file_path);
   void getBoundary(
      glm::vec3& min_point,
      glm::vec3& max_point,
      const std::vector<uint>::iterator& begin,
      const std::vector<uint>::iterator& end
   );
   [[nodiscard]] static int getDominantAxis(const glm::vec3& p0, const glm::vec3& p1);
   void setParentDisk(Disk& parent_disk);
   [[nodiscard]] uint build(
      uint parent_index,
      const std::vector<uint>::iterator& begin,
      const std::vector<uint>::iterator& end
   );
   [[nodiscard]] uint getNextIndex(uint index);
};