#ifndef __NM_GFX_MATRIX_HPP__
#define __NM_GFX_MATRIX_HPP__
#pragma once

#include <glm/glm.hpp>

namespace nmGfx
{
    /**
     * @brief 
     * 
     * @param translation 
     * @param rotation in degrees
     * @param scale 
     * @param offset 
     * @param baseTransform 
     * @return glm::mat4 
     */
    glm::mat4 CalculateModelMatrix(
        const glm::vec3& translation,
        const glm::vec3& rotation,
        const glm::vec3& scale,
        const glm::vec3& offset = glm::vec3(0.f, 0.f, 0.f),
        const glm::mat4& baseTransform = glm::mat4(1.f));

    /**
     * @brief 
     * 
     * @param position 
     * @param rotation in degrees 
     * @param scale size in pixels (use 'texturewitdh * scale' to scale based on texture)
     * @param offset can be used for rotating texture around its center
     * @param baseTransform
     * @return glm::mat4 
     */
    glm::mat4 CalculateModelMatrix(
        const glm::vec2& position,
        float rotation,
        const glm::vec2& scale,
        const glm::vec2& offset = glm::vec2(0.f, 0.f),
        const glm::mat4& baseTransform = glm::mat4(1.f));



    /**
     * @brief 
     * 
     * @param aspect aspect ratio of perspective projection. Generally videowidth / videoheight 
     * @param fov  fielf of view in degrees
     * @param near 
     * @param far 
     * @return glm::mat4 
     */
    glm::mat4 CalculatePerspective(
        float aspect,
        float fov,
        float near,
        float far);
    
    /**
     * @brief 
     * 
     * @param top 
     * @param bottom 
     * @param left 
     * @param right 
     * @return glm::mat4 
     */
    glm::mat4 CalculateOrtho(
        float left,
        float right,
        float bottom,
        float top,
        float near,
        float far);
    
    bool DecomposeMatrix(const glm::mat4 &transform, glm::vec3 &translation, glm::vec3 &rotation, glm::vec3 &scale);
    
} // namespace nmGfx


#endif