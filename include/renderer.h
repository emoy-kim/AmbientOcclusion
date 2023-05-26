/*
 * Author: Jeesun Kim
 * E-mail: emoy.kim_AT_gmail.com
 *
 */

#pragma once

#include "base.h"
#include "text.h"
#include "light.h"
#include "surface_element.h"
#include "occlusion_tree.h"

class RendererGL final
{
public:
   RendererGL();
   ~RendererGL();

   RendererGL(const RendererGL&) = delete;
   RendererGL(const RendererGL&&) = delete;
   RendererGL& operator=(const RendererGL&) = delete;
   RendererGL& operator=(const RendererGL&&) = delete;

   void play();

private:
   enum class ALGORITHM_TO_COMPARE { DYNAMIC = 0, HIGH_QUALITY };

   struct DynamicAmbientOcclusion
   {
      std::unique_ptr<ShaderGL> AmbientOcclusionShader;
      std::unique_ptr<ShaderGL> SceneShader;
      std::unique_ptr<SurfaceElement> BunnyObject;

      DynamicAmbientOcclusion() :
         AmbientOcclusionShader( std::make_unique<ShaderGL>() ), SceneShader( std::make_unique<ShaderGL>() ),
         BunnyObject( std::make_unique<SurfaceElement>() ) {}
   };

   struct HighQualityAmbientOcclusion
   {
      int TargetCanvas;
      std::array<GLuint, 2> Canvas;
      std::array<GLuint, 2> ResultTextures;
      std::unique_ptr<ShaderGL> AmbientOcclusionShader;
      std::unique_ptr<OcclusionTree> BunnyObject;

      HighQualityAmbientOcclusion() :
         TargetCanvas( 0 ), Canvas{ 0, 0 }, ResultTextures{ 0, 0 },
         AmbientOcclusionShader( std::make_unique<ShaderGL>() ), BunnyObject( std::make_unique<OcclusionTree>() ) {}

      [[nodiscard]] GLuint getResultTexture() const { return ResultTextures[TargetCanvas ^ 1]; }
      void swapCanvas() { TargetCanvas ^= 1; }
   };

   inline static RendererGL* Renderer = nullptr;
   GLFWwindow* Window;
   bool Pause;
   bool UseBentNormal;
   int FrameWidth;
   int FrameHeight;
   int ActiveLightIndex;
   int PassNum;
   glm::ivec2 ClickedPoint;
   std::unique_ptr<TextGL> Texter;
   std::unique_ptr<CameraGL> MainCamera;
   std::unique_ptr<CameraGL> TextCamera;
   std::unique_ptr<ShaderGL> TextShader;
   std::unique_ptr<LightGL> Lights;
   DynamicAmbientOcclusion Dynamic;
   HighQualityAmbientOcclusion HighQuality;
   ALGORITHM_TO_COMPARE AlgorithmToCompare;

   // 16 and 32 do well, anything in between or below is bad.
   // 32 seems to do well on laptop/desktop Windows Intel and on NVidia/AMD as well.
   // further hardware-specific tuning might be needed for optimal performance.
   static constexpr int ThreadGroupSize = 32;
   [[nodiscard]] static int getGroupSize(int size)
   {
      return (size + ThreadGroupSize - 1) / ThreadGroupSize;
   }

   void registerCallbacks() const;
   void initialize();
   void writeFrame(const std::string& name) const;

   static void printOpenGLInformation();

   static void cleanup(GLFWwindow* window);
   static void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods);
   static void cursor(GLFWwindow* window, double xpos, double ypos);
   static void mouse(GLFWwindow* window, int button, int action, int mods);
   static void mousewheel(GLFWwindow* window, double xoffset, double yoffset);

   void setLights() const;
   void setDynamicAmbientOcclusionAlgorithm() const;
   void setHighQualityAmbientOcclusionAlgorithm();
   void calculateDynamicAmbientOcclusion(int pass_num) const;
   void drawSceneWithDynamicAmbientOcclusion() const;
   void calculateHighQualityAmbientOcclusion(int pass_num);
   void drawText(const std::string& text, glm::vec2 start_position) const;
   void render();
};