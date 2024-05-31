//
// gui.h
//
// interop between gui and main program
//

#ifndef Z64SCENE_GUI_H_INCLUDED
#define Z64SCENE_GUI_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>

// C specific
#ifdef __cplusplus
extern "C" {
#endif

enum GuiEnvPreviewMode
{
	GUI_ENV_PREVIEW_EACH,
	GUI_ENV_PREVIEW_TIME,
	GUI_ENV_PREVIEW_COUNT
};

struct GuiInterop
{
	struct {
		bool isFogEnabled;
		bool isLightingEnabled;
		int envPreviewMode;
		int envPreviewEach;
		uint16_t envPreviewTime;
	} env;
};

#ifdef __cplusplus
}
#else

// C++ functions
void GuiInit(GLFWwindow *window);
void GuiCleanup(void);
void GuiDraw(GLFWwindow *window, struct Scene *scene, struct GuiInterop *interop);
int GuiHasFocus(void);

#endif

#endif /* Z64SCENE_GUI_H_INCLUDED */
