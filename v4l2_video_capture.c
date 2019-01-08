/**
 * @file v4l2_video_capture.c
 *
 * This is small utility making use of basic v4l2 api.
 * It was written only for my educational reasons but now as it works I thought
 * I can upload it to github, so it also could serve its educational goals for others.
 *
 * @author Lukasz Wiecaszek <lukasz.wiecaszek@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 */

/*===========================================================================*\
 * system header files
\*===========================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>

#include <sys/ioctl.h>
#include <sys/mman.h>

#include <linux/videodev2.h>

/*===========================================================================*\
 * project header files
\*===========================================================================*/

/*===========================================================================*\
 * preprocessor #define constants and macros
\*===========================================================================*/
#define V4l2_SELECT_TIMEOUT_SEC 2

/*===========================================================================*\
 * local type definitions
\*===========================================================================*/

/*===========================================================================*\
 * global object definitions
\*===========================================================================*/

/*===========================================================================*\
 * local function declarations
\*===========================================================================*/
static void v4l2_print_usage(const char* progname);
static const char* v4l2_capabilities_to_string(char* buf, size_t size, uint32_t capabilities);
static const char* v4l2_buf_type_to_string(enum v4l2_buf_type buf_type);
static void v4l2_print_capabilities(struct v4l2_capability* caps);
static void v4l2_print_fmtdesc(struct v4l2_fmtdesc* fmtdesc);
static void v4l2_print_frmsizeenum(struct v4l2_frmsizeenum* frmsizeenum);
static void v4l2_print_cropping_capabilities(struct v4l2_cropcap* cropcap);
static uint32_t v4l2_query_capabilities(int fd);
static const uint8_t* v4l2_query_buffers(int fd);
static void v4l2_store_frame(const uint8_t* image, size_t size, int counter);
static int v4l2_capture_frame(int fd, size_t* size);
static int v4l2_video_capture(int fd, const uint8_t* frame_buffer, int number_of_frames);

/*===========================================================================*\
 * local object definitions
\*===========================================================================*/

/*===========================================================================*\
 * inline function definitions
\*===========================================================================*/

/*===========================================================================*\
 * public function definitions
\*===========================================================================*/
int main(int argc, char *argv[])
{
    int fd;
    uint32_t capabilities;
    const uint8_t* frame_buffer;
    int number_of_frames = 1;

    static struct option long_options[] = {
        {"number-of-frames", required_argument, 0, 'n'},
        {0, 0, 0, 0}
    };

    for (;;) {
        int c = getopt_long(argc, argv, "n:", long_options, 0);
        if (-1 == c)
            break;

        switch (c) {
            case 'n':
                number_of_frames = atoi(optarg);
                break;

            default:
                /* do nothing */
                break;
        }
    }

    if (number_of_frames < 1)
        number_of_frames = 1;

    const char* filename = argv[optind];
    if (!filename) {
        fprintf(stderr, "device filename is not provided\n");
        v4l2_print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    fd = open(filename, O_RDWR);
    if (-1 == fd) {
        fprintf(stderr, "cannot open '%s': %s\n", filename, strerror(errno));
        v4l2_print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    capabilities = v4l2_query_capabilities(fd);
    if (!(capabilities & V4L2_CAP_VIDEO_CAPTURE) ||
        !(capabilities & V4L2_CAP_STREAMING)) {
        fprintf(stderr, "%s do not support video capture and streaming\n", filename);
        exit(EXIT_FAILURE);
    }

    if (NULL == (frame_buffer = v4l2_query_buffers(fd))) {
        fprintf(stderr, "v4l2_query_buffers() failed\n");
        exit(EXIT_FAILURE);
    }

    if (v4l2_video_capture(fd, frame_buffer, number_of_frames)) {
        fprintf(stderr, "v4l2_capture_image() failed\n");
        exit(EXIT_FAILURE);
    }

    close(fd);
    return 0;
}

/*===========================================================================*\
 * local function definitions
\*===========================================================================*/
static void v4l2_print_usage(const char* progname)
{
    fprintf(stdout, "usage: %s [-n <frames>] <filename>\n", progname);
    fprintf(stdout, " options:\n");
    fprintf(stdout, "  -n <frames> --number-of-frames=<frames> : number of frames to be captured (default: 1)\n");
    fprintf(stdout, "  <filename>                              : capture device (e.g. /dev/video0)\n");
}

static const char* v4l2_capabilities_to_string(char* buf, size_t size, uint32_t capabilities)
{
    size_t i;
    int n;
    char *p = buf;

    static const char* caps[] = {
        "V4L2_CAP_VIDEO_CAPTURE",
        "V4L2_CAP_VIDEO_OUTPUT",
        "V4L2_CAP_VIDEO_OVERLAY",
        "UNKNOWN_0x00000008",
        "V4L2_CAP_VBI_CAPTURE",
        "V4L2_CAP_VBI_OUTPUT",
        "V4L2_CAP_SLICED_VBI_CAPTURE",
        "V4L2_CAP_SLICED_VBI_OUTPUT",
        "V4L2_CAP_RDS_CAPTURE",
        "V4L2_CAP_VIDEO_OUTPUT_OVERLAY",
        "V4L2_CAP_HW_FREQ_SEEK",
        "V4L2_CAP_RDS_OUTPUT",
        "V4L2_CAP_VIDEO_CAPTURE_MPLANE",
        "V4L2_CAP_VIDEO_OUTPUT_MPLANE",
        "V4L2_CAP_VIDEO_M2M_MPLANE",
        "V4L2_CAP_VIDEO_M2M",
        "V4L2_CAP_TUNER",
        "V4L2_CAP_AUDIO",
        "V4L2_CAP_RADIO",
        "V4L2_CAP_MODULATOR",
        "V4L2_CAP_SDR_CAPTURE",
        "V4L2_CAP_EXT_PIX_FORMAT",
        "V4L2_CAP_SDR_OUTPUT",
        "UNKNOWN_0x00800000",
        "V4L2_CAP_READWRITE",
        "V4L2_CAP_ASYNCIO",
        "V4L2_CAP_STREAMING",
        "UNKNOWN_0x08000000",
        "V4L2_CAP_TOUCH",
        "UNKNOWN_0x20000000",
        "UNKNOWN_0x40000000",
        "V4L2_CAP_DEVICE_CAPS",
    };

    memset(p, 0, size);

    for (i = 0; i < (sizeof(caps) / sizeof(caps[0])); ++i)
        if (capabilities & (1U << i)) {
            n = snprintf(p, size, "\t\t%s\n", caps[i]);
            if (n < 0)
                return NULL;
            if ((size_t)n >= size)
                break;
            p += n;
            size -= n;
        }

    return buf;
}

static const char* v4l2_buf_type_to_string(enum v4l2_buf_type buf_type)
{
    static const char* buf_types[] = {
        [0]                                  = "0",
        [V4L2_BUF_TYPE_VIDEO_CAPTURE]        = "V4L2_BUF_TYPE_VIDEO_CAPTURE",
        [V4L2_BUF_TYPE_VIDEO_OVERLAY]        = "V4L2_BUF_TYPE_VIDEO_OVERLAY",
        [V4L2_BUF_TYPE_VIDEO_OUTPUT]         = "V4L2_BUF_TYPE_VIDEO_OUTPUT",
        [V4L2_BUF_TYPE_VBI_CAPTURE]          = "V4L2_BUF_TYPE_VBI_CAPTURE",
        [V4L2_BUF_TYPE_VBI_OUTPUT]           = "V4L2_BUF_TYPE_VBI_OUTPUT",
        [V4L2_BUF_TYPE_SLICED_VBI_CAPTURE]   = "V4L2_BUF_TYPE_SLICED_VBI_CAPTURE",
        [V4L2_BUF_TYPE_SLICED_VBI_OUTPUT]    = "V4L2_BUF_TYPE_SLICED_VBI_OUTPUT",
        [V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY] = "V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY",
        [V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE] = "V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE",
        [V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE]  = "V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE",
        [V4L2_BUF_TYPE_SDR_CAPTURE]          = "V4L2_BUF_TYPE_SDR_CAPTURE",
        [V4L2_BUF_TYPE_SDR_OUTPUT]           = "V4L2_BUF_TYPE_SDR_OUTPUT",
    };

    if (buf_type > V4L2_BUF_TYPE_SDR_OUTPUT)
        buf_type = 0;

    return buf_types[buf_type];
}

static void v4l2_print_capabilities(struct v4l2_capability* caps)
{
    char buf1[1024];
    char buf2[1024];

    fprintf(stdout,
        "VIDIOC_QUERYCAP:\n"
        "\tdriver       : %s\n"
        "\tcard         : %s\n"
        "\tbus_info     : %s\n"
        "\tversion      : 0x%08x\n"
        "\tcapabilities : 0x%08x\n"
        "%s"
        "\tdevice_caps  : 0x%08x\n"
        "%s",
        caps->driver,
        caps->card,
        caps->bus_info,
        caps->version,
        caps->capabilities,
        v4l2_capabilities_to_string(buf1, sizeof(buf1), caps->capabilities),
        caps->device_caps,
        v4l2_capabilities_to_string(buf2, sizeof(buf2), caps->device_caps)
        );
}

static void v4l2_print_fmtdesc(struct v4l2_fmtdesc* fmtdesc)
{
    fprintf(stdout,
        "VIDIOC_ENUM_FMT:\n"
        "\tindex       : %u\n"
        "\ttype        : %s\n"
        "\tflags       : 0x%08x\n"
        "\tdescription : %s\n"
        "\tpixelformat : '%c%c%c%c'\n",
        fmtdesc->index,
        v4l2_buf_type_to_string(fmtdesc->type),
        fmtdesc->flags,
        fmtdesc->description,
        (fmtdesc->pixelformat >>  0) & 0xff,
        (fmtdesc->pixelformat >>  8) & 0xff,
        (fmtdesc->pixelformat >> 16) & 0xff,
        (fmtdesc->pixelformat >> 24) & 0xff
        );
}

static void v4l2_print_frmsizeenum(struct v4l2_frmsizeenum* frmsizeenum)
{
    fprintf(stdout,
        "\tVIDIOC_ENUM_FRAMESIZES:\n"
        "\t\tindex       : %u\n"
        "\t\tpixelformat : '%c%c%c%c'\n"
        "\t\ttype        : %s\n"
        "\t\tdiscrete    : width: %u, height: %u\n",
        frmsizeenum->index,
        (frmsizeenum->pixel_format >>  0) & 0xff,
        (frmsizeenum->pixel_format >>  8) & 0xff,
        (frmsizeenum->pixel_format >> 16) & 0xff,
        (frmsizeenum->pixel_format >> 24) & 0xff,
        "V4L2_FRMIVAL_TYPE_DISCRETE",
        frmsizeenum->discrete.width,
        frmsizeenum->discrete.height
        );
}

static void v4l2_print_cropping_capabilities(struct v4l2_cropcap* cropcap)
{
    fprintf(stdout,
        "VIDIOC_CROPCAP:\n"
        "\tbounds      : left: %d, top: %d, width: %u, height: %u\n"
        "\tdefrect     : left: %d, top: %d, width: %u, height: %u\n"
        "\tpixelaspect : numerator: %u, denominator: %u\n",
        cropcap->bounds.left, cropcap->bounds.top, cropcap->bounds.width, cropcap->bounds.height,
        cropcap->defrect.left, cropcap->defrect.top, cropcap->defrect.width, cropcap->defrect.height,
        cropcap->pixelaspect.numerator, cropcap->pixelaspect.denominator
        );
}

static uint32_t v4l2_query_capabilities(int fd)
{
    uint32_t capabilities = 0;

    do {
        int status;
        struct v4l2_capability caps;
        struct v4l2_fmtdesc fmtdesc;
        struct v4l2_cropcap cropcap;
        struct v4l2_frmsizeenum frmsizeenum;

        memset(&caps, 0, sizeof(caps));
        status = ioctl(fd, VIDIOC_QUERYCAP, &caps);
        if (-1 == status) {
            fprintf(stderr, "VIDIOC_QUERYCAP failed: %s\n", strerror(errno));
            break;
        }

        capabilities = caps.capabilities;
        v4l2_print_capabilities(&caps);

        memset(&fmtdesc, 0, sizeof(fmtdesc));
        fmtdesc.index = 0;
        fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        while (0 == (status = ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc))) {
            fmtdesc.index++;
            v4l2_print_fmtdesc(&fmtdesc);

            memset(&frmsizeenum, 0, sizeof(frmsizeenum));
            frmsizeenum.index = 0;
            frmsizeenum.pixel_format = fmtdesc.pixelformat;
            while (0 == (status = ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsizeenum))) {
                frmsizeenum.index++;
                if (V4L2_FRMIVAL_TYPE_DISCRETE == frmsizeenum.type)
                    v4l2_print_frmsizeenum(&frmsizeenum);
            }
            if (-1 == status && errno != EINVAL)
                fprintf(stderr, "VIDIOC_ENUM_FRAMESIZES failed: %s\n", strerror(errno));
        }
        if (-1 == status && errno != EINVAL)
            fprintf(stderr, "VIDIOC_ENUM_FMT failed: %s\n", strerror(errno));

        memset(&cropcap, 0, sizeof(cropcap));
        cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        status = ioctl (fd, VIDIOC_CROPCAP, &cropcap);
        if (-1 == status) {
            fprintf(stderr, "VIDIOC_CROPCAP failed: %s\n", strerror(errno));
            break;
        }

        v4l2_print_cropping_capabilities(&cropcap);
    } while (0);

    return capabilities;
}

static const uint8_t* v4l2_query_buffers(int fd)
{
    const uint8_t* frame_buffer = NULL;

    do {
        struct v4l2_requestbuffers requestbuffers;
        struct v4l2_buffer buffer;

#if defined(SET_MJPG_FORMAT)
        struct v4l2_format format;
        memset(&format, 0, sizeof(format));
        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        format.fmt.pix.width = 1280;
        format.fmt.pix.height = 720;
        format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
        if (-1 == ioctl(fd, VIDIOC_S_FMT, &format)) {
            fprintf(stderr, "VIDIOC_S_FMT failed: %s\n", strerror(errno));
            break;
        }
#elif defined(SET_YUYV_FORMAT)
        struct v4l2_format format;
        memset(&format, 0, sizeof(format));
        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        format.fmt.pix.width = 1280;
        format.fmt.pix.height = 720;
        format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
        if (-1 == ioctl(fd, VIDIOC_S_FMT, &format)) {
            fprintf(stderr, "VIDIOC_S_FMT failed: %s\n", strerror(errno));
            break;
        }
#endif

        memset(&requestbuffers, 0, sizeof(requestbuffers));
        requestbuffers.count = 1;
        requestbuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        requestbuffers.memory = V4L2_MEMORY_MMAP;

        if (-1 == ioctl(fd, VIDIOC_REQBUFS, &requestbuffers)) {
            fprintf(stderr, "VIDIOC_REQBUFS failed: %s\n", strerror(errno));
            break;
        }

        fprintf(stdout,
            "VIDIOC_REQBUFS:\n"
            "\tcount: %u\n",
            requestbuffers.count
            );

        memset(&buffer, 0, sizeof(buffer));
        buffer.index = 0;
        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.memory = V4L2_MEMORY_MMAP;
        if(-1 == ioctl(fd, VIDIOC_QUERYBUF, &buffer)) {
            fprintf(stderr, "VIDIOC_QUERYBUF failed: %s\n", strerror(errno));
            break;
        }

        fprintf(stdout,
            "VIDIOC_QUERYBUF:\n"
            "\tlength: %u\n"
            "\toffset: %u\n",
            buffer.length,
            buffer.m.offset
            );

        frame_buffer = (const uint8_t*)mmap(
            NULL, buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buffer.m.offset);
        if (MAP_FAILED == frame_buffer) {
            frame_buffer = NULL;
            fprintf(stderr, "mmap() failed: %s\n", strerror(errno));
            break;
        }
    } while (0);

    return frame_buffer;
}

static void v4l2_store_frame(const uint8_t* image, size_t size, int counter)
{
    char image_filename[256];
    int fd = -1;

    do {
        int n;

        n = snprintf(image_filename, sizeof(image_filename), "image%04d.jpg", counter);
        if (n < 0)
            break;
        if ((size_t)n >= sizeof(image_filename))
            break;

        fd = open(image_filename, O_WRONLY | O_CREAT | O_TRUNC, 0664);
        if (-1 == fd){
            fprintf(stderr, "cannot open '%s': %s\n", image_filename, strerror(errno));
            break;
        }

        if (-1 == write(fd, image, size)) {
            fprintf(stderr, "write() failed: %s\n", strerror(errno));
            break;
        }

        fprintf(stdout, "'%s', size: %zu\n", image_filename, size);
    } while (0);

    if (fd != -1)
        close(fd);
}

static int v4l2_capture_frame(int fd, size_t* size)
{
    int retval = -1;

    do {
        int status;
        struct v4l2_buffer buffer;
        fd_set fds;
        struct timespec ts;

        memset(&buffer, 0, sizeof(buffer));
        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.memory = V4L2_MEMORY_MMAP;
        buffer.index = 0;

        if(-1 == ioctl(fd, VIDIOC_QBUF, &buffer)) {
            fprintf(stderr, "VIDIOC_QBUF failed: %s\n", strerror(errno));
            break;
        }

        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        ts.tv_sec = V4l2_SELECT_TIMEOUT_SEC;
        ts.tv_nsec = 0;
        status = pselect(fd+1, &fds, NULL, NULL, &ts, NULL);
        if (-1 == status) {
            fprintf(stderr, "pselect() failed: %s\n", strerror(errno));
            break;
        } else
        if (0 == status) {
            fprintf(stderr, "no data within %d seconds, timeout expired\n", V4l2_SELECT_TIMEOUT_SEC);
            break;
        }

        if(-1 == ioctl(fd, VIDIOC_DQBUF, &buffer)) {
            fprintf(stderr, "VIDIOC_DQBUF failed: %s\n", strerror(errno));
            break;
        }

        if (size)
            *size = buffer.bytesused;

        retval = 0;
    } while (0);

    return retval;
}

static int v4l2_video_capture(int fd, const uint8_t* frame_buffer, int number_of_frames)
{
    uint32_t type;
    size_t frame_size;
    int i;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if(-1 == ioctl(fd, VIDIOC_STREAMON, &type)) {
        fprintf(stderr, "VIDIOC_STREAMON failed: %s\n", strerror(errno));
        return -1;
    }

    for (i = 0; i < number_of_frames; ++i)
        if (0 == v4l2_capture_frame(fd, &frame_size))
            v4l2_store_frame(frame_buffer, frame_size, i + 1);

    if(-1 == ioctl(fd, VIDIOC_STREAMOFF, &type)) {
        fprintf(stderr, "VIDIOC_STREAMOFF failed: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

