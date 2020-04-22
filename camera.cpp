#include "camera.h"

Camera::Camera()
{

}

int Camera::dev_open()
{
    if ((fd = open("/dev/video0", O_RDWR)) == -1)
        {
            qDebug("Cannot open \n");
            return 0;
        }
     qDebug("PROFIT open \n");
    return 1;
}

int Camera::init_mmap(void)
{
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count  = 1;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1)
    {
        qDebug("FAIL V4L2_CAP_VIDEO_CAPTURE \n");
        return 0;
    }
    //    errno_exit("VIDIOC_REQBUFS");

    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;

    if (ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1)
    {
        qDebug("FAIL V4L2_CAP_VIDEO_CAPTURE \n");
        return 0;
    }
    //    errno_exit("VIDIOC_QUERYBUF");

    length = buf.length;
    buffer_start = mmap(NULL /* start anywhere */,
            length,
            PROT_READ | PROT_WRITE /* required */,
            MAP_SHARED /* recommended */,
            fd, buf.m.offset);

    if (buffer_start == MAP_FAILED)
    {
        qDebug("FAIL V4L2_CAP_VIDEO_CAPTURE \n");
        return 0;
    }
 return 1;
    //    errno_exit("mmap");
}

int Camera::dev_init()
{
    struct v4l2_capability cap;

    memset(&cap, 0, sizeof(cap));
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1)
        {
            qDebug("FAIL VIDIOC_QUERYCAP \n");
            return 0;
        }


    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        {
            qDebug("FAIL V4L2_CAP_VIDEO_CAPTURE \n");
            return 0;
        }

    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        {
            qDebug("FAIL V4L2_CAP_STREAMING \n");
            return 0;
        }

    }

    // Default to YUYV
    memset(&fmt, 0, sizeof(fmt));
    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;

//    get_pixelformat();

    // it'll adjust to the bigger screen available in the driver
    fmt.fmt.pix.width  = 3000;
    fmt.fmt.pix.height = 3000;

    if (ioctl(fd, VIDIOC_S_FMT, &fmt) == -1)
    {
        qDebug("FAIL VIDIOC_S_FMT\n");
        return 0;
    }


    init_mmap ();
    return 0;
}

int Camera::init_view()
{
    if (SDL_Init(SDL_INIT_VIDEO)) {
        printf("Unable to initialize SDL\n");
        return -1;
    }

    if (SDL_CreateWindowAndRenderer(fmt.fmt.pix.width
                    , fmt.fmt.pix.height
                    , SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL
                    , &window
                    , &renderer)) {
        printf("SDL_CreateWindowAndRenderer failed: %s\n", SDL_GetError());
        return -1;
    }

    if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV) {
        texture = SDL_CreateTexture(renderer
                        // YUY2 is also know as YUYV in SDL
                        , SDL_PIXELFORMAT_YUY2
                        , SDL_TEXTUREACCESS_STREAMING
                        , fmt.fmt.pix.width
                        , fmt.fmt.pix.height);
        if (!texture) {
            printf("SDL_CreateTexture failed: %s\n", SDL_GetError());
            return -1;
        }
    } else if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_MJPEG) {
        if (!IMG_Init(IMG_INIT_JPG)) {
            printf("Unable to initialize IMG\n");
            return -1;
        }
    }

    printf("Device: %s\nWidth: %d\nHeight: %d\n"
        , "/dev/video0", fmt.fmt.pix.width, fmt.fmt.pix.height);

    return 0;

}
static int sdl_filter(void *userdata, SDL_Event *event)
{
    (void)userdata;
    return event->type == SDL_QUIT;
}

void Camera::run_SDL_SetEventFilter()
{
SDL_SetEventFilter(sdl_filter, NULL);
}

int Camera::start_capturing()
{
    struct v4l2_buffer buf;
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    memset(&buf, 0, sizeof(buf));
    buf.type = type;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;

    if (ioctl(fd, VIDIOC_QBUF, &buf) == -1)
    {
        qDebug("FAIL VIDIOC_QBUF\n");
        return 0;
    }
    //    errno_exit("VIDIOC_QBUF ... !!!");

    if (ioctl(fd, VIDIOC_STREAMON, &type) == -1)
    {
        qDebug("FAIL VIDIOC_STREAMON\n");
        return 0;
    }
     //   errno_exit("VIDIOC_STREAMON");
return 1;
}

void Camera::draw_YUV()
{
    // YUYV is two bytes per pixel, so multiple line width by 2
    SDL_UpdateTexture(texture, NULL, buffer_start, fmt.fmt.pix.width * 2);

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void Camera::draw_MJPEG()
{
    SDL_RWops *buf_stream = SDL_RWFromMem(buffer_start, (int)length);
    SDL_Surface *frame = IMG_Load_RW(buf_stream, 0);
    SDL_Texture *tx = SDL_CreateTextureFromSurface(renderer, frame);

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, tx, NULL, NULL);
    SDL_RenderPresent(renderer);

    SDL_DestroyTexture(tx);
    SDL_FreeSurface(frame);
    SDL_RWclose(buf_stream);
}

int Camera::read_frame()
{
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_DQBUF, &buf) == -1) {
        //printf("ioctl dqbuf is wrong !!!\n");
        return 0;
    /*	switch (errno) {
            case EAGAIN:
                return 0;
            case EIO:
                // Could ignore EIO, see spec.
                // fall through
            default:
                errno_exit("VIDIOC_DQBUF");
        }*/
    }

    if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV)
        draw_YUV();
    else if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_MJPEG)
        draw_MJPEG();

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_QBUF, &buf) == -1)
    {
    return 0;
    }
    //	errno_exit("VIDIOC_QBUF");

    return 1;

}

int Camera::mainloop()
{
    SDL_Event event;
    for (;;) {
        while (SDL_PollEvent(&event))
            if (event.type == SDL_QUIT)
                return 0;
        for (;;) {
            fd_set fds;
            struct timeval tv;
            int r;

            FD_ZERO(&fds);
            FD_SET(fd, &fds);

            tv.tv_sec = 2;
            tv.tv_usec = 0;

            r = select(fd + 1, &fds, NULL, NULL, &tv);

            if (-1 == r)
            {
                if (EINTR == errno)
                    continue;

                //errno_exit("select");
                return 0;
            }

            if (0 == r)
            {
                //fprintf(stderr, "select timeout\n");
                //exit(EXIT_FAILURE);
                return 0;
            }

            if (read_frame())
                break;
            // EAGAIN - continue select loop.
        }
    }

}

int Camera::dev_close()
{

return 0;
}




