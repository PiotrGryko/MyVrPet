#include <GLES2/gl2.h>

void Engine_setup_opengl(int width, int height);
void Engine_prepare_draw();
void Engine_setMVPMatrices(float *projection, float *view, float *model);


void Engine_on_surface_changed(int width, int height);
void Engine_glBegin(GLenum type);
void Engine_glVertex3f(float x, float y, float z);
void Engine_glColor3f(float r, float g, float b);
void Engine_glVertex2d(float x, float y);
void Engine_glLineWidth(float size);
void Engine_glEnd();