#pragma once

#include "object.h"

class OcclusionTree : public ObjectGL
{
public:
   OcclusionTree();
   ~OcclusionTree() override = default;

   [[nodiscard]] bool robust() const { return Robust; }
   [[nodiscard]] int getRootIndex() const { return RootIndex; }
   [[nodiscard]] int getDiskSize() const { return static_cast<int>(Disks.size()); }
   [[nodiscard]] GLuint getInDisksBuffer() const { return DisksBuffers[TargetBufferIndex]; }
   [[nodiscard]] GLuint getOutDisksBuffer() const { return DisksBuffers[TargetBufferIndex ^ 1]; }
   [[nodiscard]] float getProximityTolerance() const { return ProximityTolerance; }
   [[nodiscard]] float getDistanceAttenuation() const { return DistanceAttenuation; }
   [[nodiscard]] float getTriangleAttenuation() const { return TriangleAttenuation; }
   void createOcclusionTree(const std::string& obj_file_path);
   void setBuffer();
   void swapBuffers() { TargetBufferIndex ^= 1; }
   void adjustProximityTolerance(float delta) { ProximityTolerance = std::max( ProximityTolerance + delta, 0.0f ); }
   void adjustDistanceAttenuation(float delta)
   {
      DistanceAttenuation = std::clamp( DistanceAttenuation + delta, 0.0f, 1.0f );
   }
   void adjustTriangleAttenuation(float delta)
   {
      TriangleAttenuation = std::clamp( TriangleAttenuation + delta, 0.0f, 1.0f );
   }

private:
   inline static int NullIndex = -1;

   struct Disk
   {
      alignas(4) int ParentIndex;
      alignas(4) int NextIndex;
      alignas(4) int LeftChildIndex;
      alignas(4) int RightChildIndex;
      alignas(4) float AreaOverPi;
      alignas(4) float Accessibility;
      alignas(16) glm::vec3 Centroid;
      alignas(16) glm::vec3 Normal;
      alignas(16) glm::vec3 BentNormal;

      Disk() :
         ParentIndex( NullIndex ), NextIndex( NullIndex ), LeftChildIndex( NullIndex ), RightChildIndex( NullIndex ),
         AreaOverPi( 0.0f ), Accessibility( 1.0f ), Centroid( 0.0f ), Normal( 0.0f ), BentNormal( 0.0f ) {}
      explicit Disk(int parent_index) :
         ParentIndex( parent_index ), NextIndex( NullIndex ), LeftChildIndex( NullIndex ), RightChildIndex( NullIndex ),
         AreaOverPi( 0.0f ), Accessibility( 1.0f ), Centroid( 0.0f ), Normal( 0.0f ), BentNormal( 0.0f ) {}
   };

   bool Robust;
   int RootIndex;
   int TargetBufferIndex;
   std::array<GLuint, 2> DisksBuffers;
   float ProximityTolerance;
   float DistanceAttenuation;
   float TriangleAttenuation;
   std::vector<Disk> Disks;
   std::vector<glm::vec3> Vertices;

   [[nodiscard]] bool readObjectFile(std::vector<glm::vec3>& normals, const std::string& file_path);
   void getBoundary(
      glm::vec3& min_point,
      glm::vec3& max_point,
      const std::vector<int>::iterator& begin,
      const std::vector<int>::iterator& end
   );
   [[nodiscard]] static int getDominantAxis(const glm::vec3& p0, const glm::vec3& p1);
   void setParentDisk(Disk& parent_disk);
   [[nodiscard]] int build(
      int parent_index,
      const std::vector<int>::iterator& begin,
      const std::vector<int>::iterator& end
   );
   [[nodiscard]] int getNextIndex(int index);
};