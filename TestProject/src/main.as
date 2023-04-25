

class CameraContoller /*: Node2D*/ {
    void _start() {

    }

    void _update() {

    }

    void _enter_tree() {

    }

    void _exit_tree() {
        
    }
}



void main() {
    vec2 c;

    // Debug::Info("Test, {},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}", 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    string format = Format("1 + 1 = {}", 1 + 1);
    Debug::Log(format);
}



class Test {
    int v;
}
void null_pointer_access() {
    Test@ test = null;
    test.v = 20;
}
