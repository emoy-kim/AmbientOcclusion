#pragma once

#include "object.h"

class SurfaceElement : public ObjectGL
{
public:
   SurfaceElement();
   ~SurfaceElement() override;

private:
   struct Face
   {

   };

   std::forward_list<Face> FaceList;
};