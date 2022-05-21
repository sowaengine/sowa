#include "EditorTheme.h"

namespace Ease
{
   EditorTheme::EditorTheme()
   {

   }

   EditorTheme::~EditorTheme()
   {

   }
   
   void EditorTheme::LoadFromStyle(ImGuiStyle _style)
   {
      style = _style;
   }
} // namespace Ease