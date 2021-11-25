#include "BatchRenderer.hpp"

#include <array>
#include <vector>
#include <algorithm>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"


namespace Ease
{

// Todo: initialize this texture to a white texture
static Texture s_WhiteTexture;
static Shader s_BatchRendererShader;
static GLuint s_BatchVertexBuffer;
static GLuint s_BatchIndexBuffer;

struct Vertex
{
   Vertex() : texIndex(-1) {}

   glm::vec3 position;
   glm::vec4 color;
   glm::vec2 texCoords;

   /** @brief number that shader will index from  */
   float texIndex = -1;
};


static int MAX_TEXTURE_SLOTS = 32;
const int MAX_QUAD_PER_DRAW_CALL = 1000;
const int MAX_VERTEX_PER_DRAW_CALL = MAX_QUAD_PER_DRAW_CALL * 4;
const int MAX_INDEX_PER_DRAW_CALL = MAX_QUAD_PER_DRAW_CALL * 6;


/* ---------------- Batch Data -------------- */
   /**
    * @brief length of usedTextures = MAX_TEXTURE_SLOTS
    * (usedTextures[0] == 2) => shader sampler2D array with index 0 has textureID 2 bound to it
    * if all slots of usedTextures are !0 it means there is no texture slot available and it flushes
    */
   static std::vector<int> Batch_usedTextures;

   /**
    * @brief current slot of usedTextures to add when adding a new texture to it
    */
   static int Batch_texturePointer;



   /**
    * @brief current index of 'buffer' to add when adding a new vertex to it
    */
   static int Batch_quadCount;
   static std::array<Vertex, MAX_QUAD_PER_DRAW_CALL> Batch_buffer;
/* ---------------- Batch Data -------------- */


static Vertex createVertex(glm::vec3 position, float rotation, glm::vec2 scale, glm::vec2 texCoords, float texIndex, glm::vec4 color)
{
   Vertex vert;
   vert.position = position;
   vert.color = color;
   vert.texCoords = texCoords;
   vert.texIndex = texIndex;
   return vert;
}


static void drawQuad(glm::vec3 location, float rotation, glm::vec2 scale, const Texture& texture, glm::vec4 color)
{
   int texIndex = 0;
   // if the texture is not bound
   if(std::find(Batch_usedTextures.begin(), Batch_usedTextures.end(), texture.getTexID()) == Batch_usedTextures.end() )
   {
      Batch_usedTextures[Batch_texturePointer] = texture.getTexID();
      
      texIndex = Batch_texturePointer;
      Batch_texturePointer++;
   }else
   {
      // if texture is already bound
      for(size_t i=0; i<Batch_usedTextures.size(); i++)
      {
         if((GLuint)Batch_usedTextures[i] == texture.getTexID())
         {
            texIndex = i;
            break;
         }
      }
   }
   
   

std::cout << location.x << "-" << location.y << "-" << texture.getHeight() << "-" << texIndex << std::endl;
   Batch_buffer[(Batch_quadCount*4)+0] = createVertex({location.x-64 , location.y-32   , 0}    , 0, scale,    {0, 0},   (float)texIndex,  color);
   Batch_buffer[(Batch_quadCount*4)+1] = createVertex({location.x-64 , location.y+32   , 0}    , 0, scale,    {0, 1},   (float)texIndex,  color);
   Batch_buffer[(Batch_quadCount*4)+2] = createVertex({location.x+128, location.y+32   , 0}    , 0, scale,    {1, 1},   (float)texIndex,  color);
   Batch_buffer[(Batch_quadCount*4)+3] = createVertex({location.x+128, location.y-32   , 0}    , 0, scale,    {1, 0},   (float)texIndex,  color);
   
   //Batch_buffer[(Batch_quadCount*4)+0] = createVertex( {-0.5f*128, -0.5f*128, 0.0f}    , 0, scale,    {0, 0},   (float)texIndex,  color);
   //Batch_buffer[(Batch_quadCount*4)+1] = createVertex( {-0.5f*128,  0.5f*128, 0.0f}    , 0, scale,    {0, 1},   (float)texIndex,  color);
   //Batch_buffer[(Batch_quadCount*4)+2] = createVertex( { 0.5f*128,  0.5f*128, 0.0f}    , 0, scale,    {1, 1},   (float)texIndex,  color);
   //Batch_buffer[(Batch_quadCount*4)+3] = createVertex( { 0.5f*128, -0.5f*128, 0.0f}    , 0, scale,    {1, 0},   (float)texIndex,  color);

   Batch_quadCount++;


   // if all texture slots are bound
   if(Batch_texturePointer >= MAX_TEXTURE_SLOTS || Batch_quadCount > MAX_QUAD_PER_DRAW_CALL)
   {
      BatchRenderer::flush2D();
   }
}


void BatchRenderer::initialize()
{
   s_BatchRendererShader.createShader("res://shaders/batch2d.vert", "res://shaders/batch2d.frag");
   
   // MAX_TEXTURE_SLOTS = glgetmaxtextureslot...
   Batch_usedTextures.resize(MAX_TEXTURE_SLOTS);
   Batch_texturePointer = 0;
   Batch_quadCount = 0;

   for(int i=0; i<MAX_TEXTURE_SLOTS; i++)
   {
      Batch_usedTextures[i] = 0;
   }

   glGenBuffers(1, &s_BatchVertexBuffer);
   glBindBuffer(GL_ARRAY_BUFFER, s_BatchVertexBuffer);
   glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*MAX_VERTEX_PER_DRAW_CALL, nullptr, GL_DYNAMIC_DRAW );


   // Attributes
   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);// (const void*)offsetof(Vertex, position) );

   glEnableVertexAttribArray(1);
   glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, color));
   
   glEnableVertexAttribArray(2);
   glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, texCoords));

   glEnableVertexAttribArray(3);
   glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, texIndex));


   /*std::array<float, MAX_INDEX_PER_DRAW_CALL> indices;
   for(int i=0; i<MAX_VERTEX_PER_DRAW_CALL; i+=4)
   {
      indices[i+0] = i+0;
      indices[i+1] = i+1;
      indices[i+2] = i+2;

      indices[i+3] = i+2;
      indices[i+4] = i+3;
      indices[i+5] = i+0;
   }*/
   float indices[] = {
      0, 1, 2,  2, 3, 0,
      4, 5, 6,  6, 7, 4
   };

   // Generate index array
   glGenBuffers(1, &s_BatchIndexBuffer);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_BatchIndexBuffer);
   //glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t)*indices.size(), indices.data(), GL_STATIC_DRAW);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(float)*12, indices, GL_STATIC_DRAW);
}


/* --------------- 2D --------------- */
//static
void BatchRenderer::begin2D()
{

}


//static
void BatchRenderer::submitQuad(glm::vec3 location, float rotation, glm::vec2 scale, const Texture& texture, glm::vec4 color/* = {1.f, 1.f, 1.f, 1.f}*/)
{
   Texture tex;
   if(texture.isValid())
      tex = Texture(tex.getTexID(), tex.getWidth(), tex.getHeight(), tex.getChannels());
   
   
   drawQuad(location, rotation, scale, texture, color);
}

//static
void BatchRenderer::flush2D()
{
   glm::mat4 proj;
   proj = glm::ortho(0, 1366, 0, 768, -10, 10);

   std::cout << "-----------------------" << std::endl;

 
   s_BatchRendererShader.bind();
   for(int i=0; i<Batch_texturePointer; i++)
   {
      glActiveTexture(GL_TEXTURE0+i);
      glBindTexture(GL_TEXTURE_2D, Batch_usedTextures[i]);
   }
   
   

   int textures[MAX_TEXTURE_SLOTS];
   for(int i=0; i<MAX_TEXTURE_SLOTS; i++)
   {
      textures[i] = Batch_usedTextures[i];
   }

   auto loc = glGetUniformLocation(s_BatchRendererShader.getID(), "u_Textures");
   glUniform1iv( loc, MAX_TEXTURE_SLOTS, textures );

   s_BatchRendererShader.bindUniform("u_Proj", proj);



   glBindBuffer(GL_ARRAY_BUFFER, s_BatchVertexBuffer);
   glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex)*(Batch_quadCount*4), Batch_buffer.data());

   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_BatchIndexBuffer);

   
   glDrawElements(GL_TRIANGLES, Batch_quadCount*6, GL_UNSIGNED_INT, nullptr);


   // reset Batch struct properties
   Batch_texturePointer = 0;
   for(size_t i=0; i<Batch_usedTextures.size(); i++)
   {
      Batch_usedTextures[i] = 0;
   }
   Batch_quadCount = 0;
}


} // namespace Ease
