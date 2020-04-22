#ifndef CAMERA_H
#define CAMERA_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <QDebug>

class Camera
{
public:
    Camera();
    int dev_open();
    int dev_init();
    int dev_close();
    int init_view();
    void run_SDL_SetEventFilter();
    int start_capturing();
    int mainloop();

private:
    int fd;
    struct v4l2_format fmt;
    int init_mmap(void);
    size_t length;
    void *buffer_start;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    int read_frame();
    void draw_YUV();
    void draw_MJPEG();
};

#endif // CAMERA_H
