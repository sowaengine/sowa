
# g++ -shared -fPIC \
#   imgui_demo.cpp \
#   imgui_draw.cpp \
#   imgui_tables.cpp \
#   imgui_widgets.cpp \
#   imgui.cpp \
#   backends/imgui_impl_glfw.cpp \
#   backends/imgui_impl_opengl3.cpp \
#   -I./ -ldl -lglfw3 -lX11 -lpthread \
#   -o imgui.so

# cp imgui.so ../../

g++ imgui_demo.cpp                    -c -o obj/imgui_demo.o         -I./ -ldl -lglfw3 -lX11 -lpthread
g++ imgui_draw.cpp                    -c -o obj/imgui_draw.o         -I./ -ldl -lglfw3 -lX11 -lpthread
g++ imgui_tables.cpp                  -c -o obj/imgui_tables.o       -I./ -ldl -lglfw3 -lX11 -lpthread
g++ imgui_widgets.cpp                 -c -o obj/imgui_widgets.o      -I./ -ldl -lglfw3 -lX11 -lpthread
g++ imgui.cpp                         -c -o obj/imgui.o              -I./ -ldl -lglfw3 -lX11 -lpthread
g++ backends/imgui_impl_glfw.cpp      -c -o obj/imgui_impl_glfw.o    -I./ -ldl -lglfw3 -lX11 -lpthread
g++ backends/imgui_impl_opengl3.cpp   -c -o obj/imgui_impl_opengl3.o -I./ -ldl -lglfw3 -lX11 -lpthread

ar rcs libimgui.a obj/**.o

cp libimgui.a ../../lib

rm obj/**