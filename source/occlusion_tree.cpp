#include "occlusion_tree.h"

OcclusionTree::OcclusionTree() :
   ObjectGL(), Robust( false ), RootIndex( NullIndex ), TargetBufferIndex( 0 ), DisksBuffers{ 0, 0 },
   ProximityTolerance( 8.0f ), DistanceAttenuation( 0.0f ), TriangleAttenuation( 0.5f )
{
}

bool OcclusionTree::readObjectFile(std::vector<glm::vec3>& normals, const std::string& file_path)
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

   normals.resize( vertex_buffer.size(), glm::vec3(0.0f) );
   const auto size = static_cast<int>(vertex_indices.size());
   for (int i = 0; i < size; i += 3) {
      const GLuint v0 = vertex_indices[i];
      const GLuint v1 = vertex_indices[i + 1];
      const GLuint v2 = vertex_indices[i + 2];
      const glm::vec3 n = glm::cross( vertex_buffer[v1] - vertex_buffer[v0], vertex_buffer[v2] - vertex_buffer[v0] );
      normals[v0] += n;
      normals[v1] += n;
      normals[v2] += n;
   }
   for (auto& n : normals) n = glm::normalize( n );
   Vertices = std::move( vertex_buffer );
   IndexBuffer = std::move( vertex_indices );
   return true;
}

void OcclusionTree::getBoundary(
   glm::vec3& min_point,
   glm::vec3& max_point,
   const std::vector<int>::iterator& begin,
   const std::vector<int>::iterator& end
)
{
   min_point = glm::vec3(std::numeric_limits<float>::max());
   max_point = glm::vec3(std::numeric_limits<float>::lowest());
   for (auto face_index = begin; face_index != end; ++face_index) {
      const int i = 3 * *face_index;
      const glm::vec3 v0 = Vertices[IndexBuffer[i]];
      const glm::vec3 v1 = Vertices[IndexBuffer[i + 1]];
      const glm::vec3 v2 = Vertices[IndexBuffer[i + 2]];
      const glm::vec3 centroid = (v0 + v1 + v2) / 3.0f;
      if (centroid.x < min_point.x) min_point.x = centroid.x;
      if (centroid.y < min_point.y) min_point.y = centroid.y;
      if (centroid.z < min_point.z) min_point.z = centroid.z;

      if (centroid.x > max_point.x) max_point.x = centroid.x;
      if (centroid.y > max_point.y) max_point.y = centroid.y;
      if (centroid.z > max_point.z) max_point.z = centroid.z;
   }
}

int OcclusionTree::getDominantAxis(const glm::vec3& p0, const glm::vec3& p1)
{
   int axis;
   float max_length = -1.0f;
   float diff = p1.x - p0.x;
   if (max_length < diff) {
      max_length = diff;
      axis = 0;
   }

   diff = p1.y - p0.y;
   if (max_length < diff) {
      max_length = diff;
      axis = 1;
   }

   diff = p1.z - p0.z;
   if (max_length < diff) {
      max_length = diff;
      axis = 2;
   }
   return axis;
}

void OcclusionTree::setParentDisk(Disk& parent_disk)
{
   const Disk& left_disk = Disks[parent_disk.LeftChildIndex];
   const Disk& right_disk = Disks[parent_disk.RightChildIndex];
   parent_disk.AreaOverPi = left_disk.AreaOverPi + right_disk.AreaOverPi;
   const float weight = right_disk.AreaOverPi / parent_disk.AreaOverPi;
   parent_disk.Centroid = glm::mix( left_disk.Centroid, right_disk.Centroid, weight );
   parent_disk.Normal = glm::normalize( glm::mix( left_disk.Normal, right_disk.Normal, weight ) );
   if (std::isnan( parent_disk.Centroid.x )) parent_disk.Centroid = (left_disk.Centroid + right_disk.Centroid) * 0.5f;
   if (std::isnan( parent_disk.Normal.x )) parent_disk.Normal = glm::normalize( parent_disk.Centroid );
}

int OcclusionTree::build(
   int parent_index,
   const std::vector<int>::iterator& begin,
   const std::vector<int>::iterator& end
)
{
   assert( begin <= end );

   if (begin == end) return NullIndex;
   else if (begin + 1 == end) {
      Disk disk;
      const int i = 3 * *begin;
      const glm::vec3 v0 = Vertices[IndexBuffer[i]];
      const glm::vec3 v1 = Vertices[IndexBuffer[i + 1]];
      const glm::vec3 v2 = Vertices[IndexBuffer[i + 2]];
      const glm::vec3 n = glm::cross( v1 - v0, v2 - v0 );
      disk.Normal = glm::normalize( n );
      disk.AreaOverPi = glm::length( n ) * 0.5f / glm::pi<float>();
      disk.Centroid = (v0 + v1 + v2) / 3.0f;
      disk.ParentIndex = parent_index;
      Disks[*begin] = disk;
      return *begin;
   }

   glm::vec3 min_point, max_point;
   getBoundary( min_point, max_point, begin, end );
   const int dominant_axis = getDominantAxis( min_point, max_point );
   std::sort(
      begin, end,
      [this, dominant_axis](int a, int b)
      {
         const int i = 3 * a;
         const int j = 3 * b;
         const float triple_centroid_a =
            Vertices[IndexBuffer[i]][dominant_axis] +
            Vertices[IndexBuffer[i + 1]][dominant_axis] +
            Vertices[IndexBuffer[i + 2]][dominant_axis];
         const float triple_centroid_b =
            Vertices[IndexBuffer[j]][dominant_axis] +
            Vertices[IndexBuffer[j + 1]][dominant_axis] +
            Vertices[IndexBuffer[j + 2]][dominant_axis];
         return triple_centroid_a < triple_centroid_b;
      }
   );
   const auto mid = begin + (end - begin) / 2;

   assert( begin <= mid && mid <= end && begin <= end );

   const auto location = static_cast<int>(Disks.size());
   Disk& new_disk = Disks.emplace_back( parent_index );
   new_disk.LeftChildIndex = build( location, begin, mid );
   new_disk.RightChildIndex = build( location, mid, end );

   assert( new_disk.LeftChildIndex != NullIndex && new_disk.RightChildIndex != NullIndex );

   setParentDisk( new_disk );
   return location;
}

int OcclusionTree::getNextIndex(int index)
{
   if (index == RootIndex) return NullIndex;
   else {
      const Disk& disk = Disks[index];
      const Disk& parent_disk = Disks[disk.ParentIndex];
      return parent_disk.LeftChildIndex == index ? parent_disk.RightChildIndex : getNextIndex( disk.ParentIndex );
   }
}

void OcclusionTree::createOcclusionTree(const std::string& obj_file_path)
{
   DrawMode = GL_TRIANGLES;
   std::vector<glm::vec3> normals;
   if (!readObjectFile( normals, obj_file_path )) return;

   for (int i = 0; i < static_cast<int>(Vertices.size()); ++i) {
      DataBuffer.emplace_back( Vertices[i].x );
      DataBuffer.emplace_back( Vertices[i].y );
      DataBuffer.emplace_back( Vertices[i].z );
      DataBuffer.emplace_back( normals[i].x );
      DataBuffer.emplace_back( normals[i].y );
      DataBuffer.emplace_back( normals[i].z );
      VerticesCount++;
   }
   normals.clear();

   const auto n_bytes_per_vertex = static_cast<int>(6 * sizeof( GLfloat ));
   prepareVertexBuffer( n_bytes_per_vertex );
   prepareNormal();
   prepareIndexBuffer();

   const size_t face_num = IndexBuffer.size() / 3;

   Disks.clear();
   Disks.resize( face_num );

   std::vector<int> indexer(face_num);
   std::iota( indexer.begin(), indexer.end(), 0 );
   RootIndex = build( NullIndex, indexer.begin(), indexer.end() );

   for (int i = 0; i < static_cast<int>(Disks.size()); ++i) {
      Disks[i].NextIndex = getNextIndex( i );
   }
}

void OcclusionTree::setBuffer()
{
   const auto size = static_cast<int>(Disks.size());
   addCustomBufferObject<Disk>( "in_disks", size );
   addCustomBufferObject<Disk>( "out_disks", size );
   DisksBuffers[TargetBufferIndex] = getCustomBufferID( "in_disks" );
   DisksBuffers[TargetBufferIndex ^ 1] = getCustomBufferID( "out_disks" );
   glNamedBufferSubData(
      DisksBuffers[0], 0,
      static_cast<GLsizei>(size * sizeof( Disk )),
      Disks.data()
   );
   glNamedBufferSubData(
      DisksBuffers[1], 0,
      static_cast<GLsizei>(size * sizeof( Disk )),
      Disks.data()
   );
}