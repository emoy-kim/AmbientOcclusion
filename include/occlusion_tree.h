#pragma once

#include "object.h"

class OcclusionTree : public ObjectGL
{
public:
   OcclusionTree();
   ~OcclusionTree() override = default;

   [[nodiscard]] GLuint getDisksBuffer() const { return DisksBuffer; }
   void createOcclusionTree(const std::string& obj_file_path);
   void setBuffer();

private:
   inline static uint NullIndex = std::numeric_limits<uint>::max();

   struct Disk
   {
      alignas(4) float Area;
      alignas(4) uint ParentIndex;
      alignas(4) uint NextIndex;
      alignas(4) uint LeftChildIndex;
      alignas(4) uint RightChildIndex;
      alignas(16) glm::vec3 Centroid;
      alignas(16) glm::vec3 Normal;

      Disk() :
         Area( 0.0f ), ParentIndex( NullIndex ), NextIndex( NullIndex ), LeftChildIndex( NullIndex ),
         RightChildIndex( NullIndex ), Centroid( 0.0f ), Normal( 0.0f ) {}
      explicit Disk(uint parent_index) :
         Area( 0.0f ), ParentIndex( parent_index ), NextIndex( NullIndex ), LeftChildIndex( NullIndex ),
         RightChildIndex( NullIndex ), Centroid( 0.0f ), Normal( 0.0f ) {}
   };

   uint RootIndex;
   GLuint DisksBuffer;
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