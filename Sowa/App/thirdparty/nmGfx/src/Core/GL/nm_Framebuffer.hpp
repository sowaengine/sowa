#ifndef __NM_GFX_FRAMEBUFFER_HPP__
#define __NM_GFX_FRAMEBUFFER_HPP__

namespace nmGfx
{
    class Renderer;
    class Window;

    class Framebuffer
    {
        public:
            Framebuffer() = default;

            /**
             * @brief Creates default framebuffer with gBuffer layout
             * 
             * vec4 gAlbedoSpec
             * vec3 gPosition
             * vec3 gNormal
             * unsigned int drawID
             * 
             * @param width 
             * @param height 
             */
            void CreateGBuffer(Window* pWindow, int width, int height);

            /**
             * @brief Creates default framebuffer with gBuffer layout
             * 
             * vec4 Color
             * unsigned int drawID
             * 
             * @param width 
             * @param height 
             */
            void Create2DDefault(Window* pWindow, int width, int height);

            void Use();

            inline unsigned int GetAlbedoID() { return _gAlbedo; }

        private:
            unsigned int _id = 0;

            unsigned int _gAlbedo;
            unsigned int _gPosition;
            unsigned int _gNormal;
            unsigned int _gDrawID;

            unsigned int _depthBuffer;
            

            friend class Renderer;

            int _width = 0;
            int _height = 0;


            Window* _pWindow; // handle to window used for getting window size
    };
} // namespace nmGfx


#endif // __NM_GFX_FRAMEBUFFER_HPP__