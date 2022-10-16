

void Scene() {
    Gui::Text("Scene");
    Gui::BeginCheckerList("##sceneview");

    Gui::CheckerListNextItem();
    if(Gui::BeginTree("Entity 1"))
    {
        Gui::CheckerListNextItem();
        if(Gui::BeginTree("Entity 2"))
        {
            Gui::CheckerListNextItem();
            if(Gui::BeginTree("Entity 3"))
            {
                Gui::EndTree();
            }
            Gui::EndTree();
        }
        Gui::EndTree();
    }

        
    Gui::EndCheckerList();
}

void Console() {
    Gui::Text("Console");
}

void Properties() {
    Gui::Text("Properties");
}

void Filesystem() {
    Gui::Text("Filesystem");
}

void Game() {
    Gui::Button(">", 32, 32);
    Gui::Separator();
}