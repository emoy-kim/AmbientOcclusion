#include "renderer.h"

RendererGL::RendererGL() :
   Window( nullptr ), Pause( false ), NeedUpdate( true ), UseBentNormal( true ), FrameWidth( 1920 ), FrameHeight( 1080 ),
   ActiveLightIndex( 0 ), PassNum( 2 ), ClickedPoint( -1, -1 ), Texter( std::make_unique<TextGL>() ),
   MainCamera( std::make_unique<CameraGL>() ), TextCamera( std::make_unique<CameraGL>() ),
   TextShader( std::make_unique<ShaderGL>() ), DynamicAmbientOcclusionShader( std::make_unique<ShaderGL>() ),
   HighQualityAmbientOcclusionShader( std::make_unique<ShaderGL>() ), SceneShader( std::make_unique<ShaderGL>() ),
   BunnyObjectFromSurfaceElement( std::make_unique<SurfaceElement>() ),
   BunnyObjectFromOcclusionTree( std::make_unique<OcclusionTree>() ), Lights( std::make_unique<LightGL>() ),
   AlgorithmToCompare( ALGORITHM_TO_COMPARE::DYNAMIC )
{
   Renderer = this;

   initialize();
   printOpenGLInformation();
}

void RendererGL::printOpenGLInformation()
{
   std::cout << "****************************************************************\n";
   std::cout << " - GLFW version supported: " << glfwGetVersionString() << "\n";
   std::cout << " - OpenGL renderer: " << glGetString( GL_RENDERER ) << "\n";
   std::cout << " - OpenGL version supported: " << glGetString( GL_VERSION ) << "\n";
   std::cout << " - OpenGL shader version supported: " << glGetString( GL_SHADING_LANGUAGE_VERSION ) << "\n";
   std::cout << "****************************************************************\n\n";
}

void RendererGL::initialize()
{
   if (!glfwInit()) {
      std::cout << "Cannot Initialize OpenGL...\n";
      return;
   }
   glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
   glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 6 );
   glfwWindowHint( GLFW_DOUBLEBUFFER, GLFW_TRUE );
   glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );
   glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );

   Window = glfwCreateWindow( FrameWidth, FrameHeight, "Ambient Occlusion", nullptr, nullptr );
   glfwMakeContextCurrent( Window );

   if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
      std::cout << "Failed to initialize GLAD" << std::endl;
      return;
   }

   registerCallbacks();

   glEnable( GL_CULL_FACE );
   glEnable( GL_DEPTH_TEST );
   glClearColor( 0.4f, 0.5f, 0.6f, 1.0f );

   Texter->initialize( 30.0f );

   TextCamera->update2DCamera( FrameWidth, FrameHeight );
   MainCamera->updatePerspectiveCamera( FrameWidth, FrameHeight );

   const std::string shader_directory_path = std::string(CMAKE_SOURCE_DIR) + "/shaders";
   TextShader->setShader(
      std::string(shader_directory_path + "/text.vert").c_str(),
      std::string(shader_directory_path + "/text.frag").c_str()
   );
   DynamicAmbientOcclusionShader->setComputeShaders(
      std::string(shader_directory_path + "/dynamic/ambient_occlusion.comp").c_str()
   );
   HighQualityAmbientOcclusionShader->setComputeShaders(
      std::string(shader_directory_path + "/high-quality/ambient_occlusion.comp").c_str()
   );
   SceneShader->setShader(
      std::string(shader_directory_path + "/scene_shader.vert").c_str(),
      std::string(shader_directory_path + "/scene_shader.frag").c_str()
   );
}

void RendererGL::writeFrame(const std::string& name) const
{
   const int size = FrameWidth * FrameHeight * 3;
   auto* buffer = new uint8_t[size];
   glPixelStorei( GL_PACK_ALIGNMENT, 1 );
   glReadBuffer( GL_COLOR_ATTACHMENT0 );
   glReadPixels( 0, 0, FrameWidth, FrameHeight, GL_BGR, GL_UNSIGNED_BYTE, buffer );
   FIBITMAP* image = FreeImage_ConvertFromRawBits(
      buffer, FrameWidth, FrameHeight, FrameWidth * 3, 24,
      FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, false
   );
   FreeImage_Save( FIF_PNG, image, name.c_str() );
   FreeImage_Unload( image );
   delete [] buffer;
}

void RendererGL::cleanup(GLFWwindow* window)
{
   glfwSetWindowShouldClose( window, GLFW_TRUE );
}

void RendererGL::keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
   if (action != GLFW_PRESS) return;

   switch (key) {
      case GLFW_KEY_1:
         if (!Renderer->Pause) {
            Renderer->AlgorithmToCompare = ALGORITHM_TO_COMPARE::DYNAMIC;
            std::cout << "Dynamic Ambient Occlusion Algorithm Selected\n";
         }
         break;
      case GLFW_KEY_2:
         if (!Renderer->Pause) {
            Renderer->AlgorithmToCompare = ALGORITHM_TO_COMPARE::HIGH_QUALITY;
            std::cout << "High Quality Ambient Occlusion Algorithm Selected\n";
         }
         break;
      case GLFW_KEY_UP:
         if (!Renderer->Pause) {
            Renderer->PassNum++;
            Renderer->NeedUpdate = true;
            std::cout << "Pass Num: " << Renderer->PassNum << std::endl;
         }
         break;
      case GLFW_KEY_DOWN:
         if (!Renderer->Pause) {
            Renderer->PassNum--;
            Renderer->NeedUpdate = true;
            if (Renderer->PassNum < 1) Renderer->PassNum = 1;
            std::cout << "Pass Num: " << Renderer->PassNum << std::endl;
         }
         break;
      case GLFW_KEY_B:
         if (!Renderer->Pause) {
            Renderer->UseBentNormal = !Renderer->UseBentNormal;
            Renderer->NeedUpdate = true;
            if (Renderer->UseBentNormal) std::cout << "Bent Normal Used\n";
            else std::cout << "Original Normal Used\n";
         }
         break;
      case GLFW_KEY_C:
         Renderer->writeFrame( "../result.png" );
         break;
      case GLFW_KEY_L:
         Renderer->Lights->toggleLightSwitch();
         std::cout << "Light Turned " << (Renderer->Lights->isLightOn() ? "On!\n" : "Off!\n");
         break;
      case GLFW_KEY_P: {
         const glm::vec3 pos = Renderer->MainCamera->getCameraPosition();
         std::cout << "Camera Position: " << pos.x << ", " << pos.y << ", " << pos.z << "\n";
      } break;
      case GLFW_KEY_SPACE:
         Renderer->Pause = !Renderer->Pause;
         break;
      case GLFW_KEY_Q:
      case GLFW_KEY_ESCAPE:
         cleanup( window );
         break;
      default:
         return;
   }
}

void RendererGL::cursor(GLFWwindow* window, double xpos, double ypos)
{
   if (Renderer->Pause) return;

   if (Renderer->MainCamera->getMovingState()) {
      const auto x = static_cast<int>(std::round( xpos ));
      const auto y = static_cast<int>(std::round( ypos ));
      const int dx = x - Renderer->ClickedPoint.x;
      const int dy = y - Renderer->ClickedPoint.y;
      Renderer->MainCamera->moveForward( -dy );
      Renderer->MainCamera->rotateAroundWorldY( -dx );

      if (glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_RIGHT ) == GLFW_PRESS) {
         Renderer->MainCamera->pitch( -dy );
      }

      Renderer->ClickedPoint.x = x;
      Renderer->ClickedPoint.y = y;
   }
}

void RendererGL::mouse(GLFWwindow* window, int button, int action, int mods)
{
   if (Renderer->Pause) return;

   if (button == GLFW_MOUSE_BUTTON_LEFT) {
      const bool moving_state = action == GLFW_PRESS;
      if (moving_state) {
         double x, y;
         glfwGetCursorPos( window, &x, &y );
         Renderer->ClickedPoint.x = static_cast<int>(std::round( x ));
         Renderer->ClickedPoint.y = static_cast<int>(std::round( y ));
      }
      Renderer->MainCamera->setMovingState( moving_state );
   }
}

void RendererGL::mousewheel(GLFWwindow* window, double xoffset, double yoffset)
{
   if (Renderer->Pause) return;

   if (yoffset >= 0.0) Renderer->MainCamera->zoomIn();
   else Renderer->MainCamera->zoomOut();
}

void RendererGL::registerCallbacks() const
{
   glfwSetWindowCloseCallback( Window, cleanup );
   glfwSetKeyCallback( Window, keyboard );
   glfwSetCursorPosCallback( Window, cursor );
   glfwSetMouseButtonCallback( Window, mouse );
   glfwSetScrollCallback( Window, mousewheel );
}

void RendererGL::setLights() const
{
   const glm::vec4 light_position(500.0f, 500.0f, 500.0f, 1.0f);
   const glm::vec4 ambient_color(1.0f, 1.0f, 1.0f, 1.0f);
   const glm::vec4 diffuse_color(0.9f, 0.9f, 0.9f, 1.0f);
   const glm::vec4 specular_color(0.9f, 0.9f, 0.9f, 1.0f);
   Lights->addLight( light_position, ambient_color, diffuse_color, specular_color );
}

void RendererGL::setBunnyObjects() const
{
   const std::string sample_directory_path = std::string(CMAKE_SOURCE_DIR) + "/samples";
   const std::string obj_file_path = std::string( sample_directory_path + "/Bunny/bunny.obj");
   BunnyObjectFromSurfaceElement->createSurfaceElements( obj_file_path );
   BunnyObjectFromSurfaceElement->setDiffuseReflectionColor( { 1.0f, 1.0f, 1.0f, 1.0f } );
   BunnyObjectFromSurfaceElement->setBuffer();

   BunnyObjectFromOcclusionTree->createOcclusionTree( obj_file_path );
   BunnyObjectFromOcclusionTree->setDiffuseReflectionColor( { 1.0f, 1.0f, 1.0f, 1.0f } );

}

void RendererGL::drawBunnyObject(ObjectGL* bunny) const
{
   SceneShader->transferBasicTransformationUniforms( glm::mat4(1.0f), MainCamera.get() );
   bunny->transferUniformsToShader( SceneShader.get() );

   glBindVertexArray( bunny->getVAO() );
   glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, bunny->getIBO() );
   glDrawElements( bunny->getDrawMode(), bunny->getIndexNum(), GL_UNSIGNED_INT, nullptr );
}

void RendererGL::calculateDynamicAmbientOcclusion(int pass_num)
{
   const int n = BunnyObjectFromSurfaceElement->getVertexBufferSize();
   const auto m = static_cast<int>(std::ceil( std::sqrt( static_cast<float>(n) ) ));
   const int g = getGroupSize( m );
   glUseProgram( DynamicAmbientOcclusionShader->getShaderProgram() );
   DynamicAmbientOcclusionShader->uniform1i( "Side", m );
   DynamicAmbientOcclusionShader->uniform1i( "VertexBufferSize", n );
   glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 0, BunnyObjectFromSurfaceElement->getReceiversBuffer() );
   glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1, BunnyObjectFromSurfaceElement->getSurfaceElementsBuffer() );
   for (int i = 1; i <= pass_num; ++i) {
      DynamicAmbientOcclusionShader->uniform1i( "Phase", i );
      glDispatchCompute( g, g, 1 );
      glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
   }
   glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 0, 0 );
   glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1, 0 );
}

void RendererGL::calculateHighQualityAmbientOcclusion()
{
   glUseProgram( HighQualityAmbientOcclusionShader->getShaderProgram() );

   glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
}

void RendererGL::drawScene(ObjectGL* object) const
{
   glViewport( 0, 0, FrameWidth, FrameHeight );
   glBindFramebuffer( GL_FRAMEBUFFER, 0 );
   glUseProgram( SceneShader->getShaderProgram() );
   Lights->transferUniformsToShader( SceneShader.get() );
   SceneShader->uniform1i( "UseBentNormal", UseBentNormal ? 1 : 0 );
   SceneShader->uniform1i( "LightIndex", ActiveLightIndex );
   drawBunnyObject( object );
}

void RendererGL::drawText(const std::string& text, glm::vec2 start_position) const
{
   std::vector<TextGL::Glyph*> glyphs;
   Texter->getGlyphsFromText( glyphs, text );

   glViewport( 0, 0, FrameWidth, FrameHeight );
   glBindFramebuffer( GL_FRAMEBUFFER, 0 );
   glUseProgram( TextShader->getShaderProgram() );

   glEnable( GL_BLEND );
   glBlendFunc( GL_SRC_ALPHA, GL_ONE );
   glDisable( GL_DEPTH_TEST );

   glm::vec2 text_position = start_position;
   const ObjectGL* glyph_object = Texter->getGlyphObject();
   glBindVertexArray( glyph_object->getVAO() );
   for (const auto& glyph : glyphs) {
      if (glyph->IsNewLine) {
         text_position.x = start_position.x;
         text_position.y -= Texter->getFontSize();
         continue;
      }

      const glm::vec2 position(
         std::round( text_position.x + glyph->Bearing.x ),
         std::round( text_position.y + glyph->Bearing.y - glyph->Size.y )
      );
      const glm::mat4 to_world =
         glm::translate( glm::mat4(1.0f), glm::vec3(position, 0.0f) ) *
         glm::scale( glm::mat4(1.0f), glm::vec3(glyph->Size.x, glyph->Size.y, 1.0f) );
      TextShader->transferBasicTransformationUniforms( to_world, TextCamera.get() );
      TextShader->uniform2fv( "TextScale", glyph->TopRightTextureCoord );
      glBindTextureUnit( 0, glyph_object->getTextureID( glyph->TextureIDIndex ) );
      glDrawArrays( glyph_object->getDrawMode(), 0, glyph_object->getVertexNum() );

      text_position.x += glyph->Advance.x;
      text_position.y -= glyph->Advance.y;
   }
   glEnable( GL_DEPTH_TEST );
   glDisable( GL_BLEND );
}

void RendererGL::render()
{
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();

   if (AlgorithmToCompare == ALGORITHM_TO_COMPARE::DYNAMIC) {
      if (NeedUpdate) {
         calculateDynamicAmbientOcclusion( PassNum );
         NeedUpdate = false;
      }
      drawScene( BunnyObjectFromSurfaceElement.get() );
   }
   else if (AlgorithmToCompare == ALGORITHM_TO_COMPARE::HIGH_QUALITY) {
      if (NeedUpdate) {
         calculateHighQualityAmbientOcclusion();
         NeedUpdate = false;
      }
      drawScene( BunnyObjectFromOcclusionTree.get() );
   }

   std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
   const auto fps = 1E+6 / static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());

   std::stringstream text;
   text << std::fixed << std::setprecision( 2 ) << fps << " fps";
   drawText( text.str(), { 80.0f, 50.0f } );
}

void RendererGL::play()
{
   if (glfwWindowShouldClose( Window )) initialize();

   setLights();
   setBunnyObjects();

   TextShader->setTextUniformLocations();
   SceneShader->setSceneUniformLocations( 1 );
   DynamicAmbientOcclusionShader->setDynamicAmbientOcclusionUniformLocations();
   HighQualityAmbientOcclusionShader->setHighQualityAmbientOcclusionUniformLocations();

   while (!glfwWindowShouldClose( Window )) {
      if (!Pause) render();

      glfwSwapBuffers( Window );
      glfwPollEvents();
   }
   glfwDestroyWindow( Window );
}