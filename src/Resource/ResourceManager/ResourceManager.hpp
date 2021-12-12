/**
 * @file ResourceManager.hpp
 * @author Lexographics
 * @brief 
 * @version 0.1
 * @date 2021-12-12
 */
#pragma once
#include "Debug.hpp"


#include <memory>


typedef uint32_t ResourceID;

namespace Ease
{
class Texture;


// Singleton class, dont instantiate
class cl_ResourceManager
{
public:
   static void Init();

   /**
    * @brief 
    * 
    * @param path Relative to Project Directory
    * @return std::shared_ptr<Texture> 
    */
   std::shared_ptr<Texture> LoadTexture(const std::string& path);


private:
   cl_ResourceManager() = default;

   // increments on every loaded resource
   uint32_t m_ResourceIDCounter = 1;

   ResourceID GenerateID() { return m_ResourceIDCounter++; }
private:
   friend class Resource;
   void DestroyResource(ResourceID id);

};



} // namespace Ease
