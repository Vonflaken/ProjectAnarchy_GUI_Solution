# ProjectAnarchy_GUI_Solution
It´s a GUI solution for Project Anarchy engine. It wraps VisScreenMask_cl with useful functions for get an awesome UI control.

## Features
* Handles OnClickUp/OnClickDown input.
* Transform control: position, rotation and scale.
* Ease transform (and color) transitions.
* Frame animation.
* TexturePacker spritesheet support.
* Input and animation callbacks.
* Relative positioning.
* Depth ordering.
* SD and HD textures support.

## Usage
Prepare TexturePacker project settings (download from https://www.codeandweb.com/texturepacker):  
* Data format -> JSON (Array).
* Data file and Texture file named the same way.
* Frames of a same element must be named with '_#' sufix from 0. Example: gui_element**_0**.png, gui_element**_1**.png, etc.
* Packing with rotated elements is not supported. 

In source code, first of all:  
```
#include "GUIAnimationManager.h"
#include "GUIAnimation.h"
```
It loads UI textures from TexturePacker output and make it usable (from now I will use 'TP' as alias of 'TexturePacker'):

1.   Use ```GUIAnimationManager::Instance().LoadTexturePackerJSON( "TP_OUTPUT_FILENAME_WITHOUT_EXTENSION", "PATH_TO_TP_OUTPUT_FILES" )``` to map every UI element by name with their frame (x, y, width, height). You can load multiple texture atlases.
2.   Then, in order to create a GUI element It uses ```CreateAnimation( "PATH_TO_TP_OUTPUT_FILES", "TP_OUTPUT_FILENAME_WITH_EXTENSION", FIRST_FRAME, LAST_FRAME, FRAMES_NUMBER, ID, ANIM_TYPE )```. Also you can create GUI elements from single textures, just point out path and filename to this particular texture in previous function.
3.   Update GUI calling ```GUIAnimationManager::Instance().Update( Vision::GetTimer()->GetTimeDifference() )``` every frame. Normally put it in **OnUpdateSceneBegin** callback.
4.   Use GUIAnimation API however you want.
5.   In order to free memory and resources call ```GUIAnimationManager::Instance().DeInit()```. Normally when the app closes.

## Used in
* Cut-shumoto (http://enjoystickstudios.com/). Mobile game.

## Thanks 
* EnjoyStick Studios. My team suffered the development of this project ;).
* UIToolkit (https://github.com/oddgames/UIToolkit). I based the easing transition and positioning code of GUI elements in that library.

## Contact
Feel free to message me about issues, improvements or whatever in **vonflaken@gmail.com**
