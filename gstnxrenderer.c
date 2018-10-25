/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Sungwoo, Park <swpark@nexell.co.kr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <xf86drm.h>
#include <drm_fourcc.h>

#include "nx-drm-allocator.h"

#include "gstnxrenderer.h"

#define NX_PLANE_TYPE_RGB       (0<<4)
#define NX_PLANE_TYPE_VIDEO     (1<<4)
#define NX_PLANE_TYPE_UNKNOWN   (0xFFFFFFF)
/* FIXME
 * this define must be moved to nx-renderer library
 */
static const uint32_t dp_formats[] = {

	/* 1 plane */
	DRM_FORMAT_YUYV,
	DRM_FORMAT_YVYU,
	DRM_FORMAT_UYVY,
	DRM_FORMAT_VYUY,

	/* 2 plane */
	DRM_FORMAT_NV12, /* NV12 : 2x2 subsampled Cr:Cb plane */
	DRM_FORMAT_NV21, /* NV21 : 2x2 subsampled Cb:Cr plane */
	DRM_FORMAT_NV16, /* NV16 : 2x1 subsampled Cr:Cb plane */
	DRM_FORMAT_NV61, /* NV61 : 2x1 subsampled Cb:Cr plane */

	/* 3 plane */
	DRM_FORMAT_YUV420, /* YU12 : 2x2 subsampled Cb (1) and Cr (2) planes */
	DRM_FORMAT_YVU420, /* YV12 : 2x2 subsampled Cr (1) and Cb (2) planes */
	DRM_FORMAT_YUV422, /* YU16 : 2x1 subsampled Cb (1) and Cr (2) planes */
	DRM_FORMAT_YVU422, /* YV16 : 2x1 subsampled Cr (1) and Cb (2) planes */
	DRM_FORMAT_YUV444, /* YU24 : non-subsampled Cb (1) and Cr (2) planes */
	DRM_FORMAT_YVU444, /* YV24 : non-subsampled Cr (1) and Cb (2) planes */

	/* RGB 1 buffer */
	DRM_FORMAT_RGB565,
	DRM_FORMAT_BGR565,
	DRM_FORMAT_RGB888,
	DRM_FORMAT_BGR888,
	DRM_FORMAT_ARGB8888,
	DRM_FORMAT_ABGR8888,
	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_XBGR8888,
};

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE("sink",
	GST_PAD_SINK,
	GST_PAD_ALWAYS,
	GST_STATIC_CAPS_ANY);

GST_DEBUG_CATEGORY_STATIC(gst_nx_renderer_debug);
#define GST_CAT_DEFAULT gst_nx_renderer_debug

#define DEFAULT_SYNC	FALSE

#define _do_init \
	GST_DEBUG_CATEGORY_INIT(gst_nx_renderer_debug, "nxrenderer", 0, "nxrenderer element");
#define gst_nx_renderer_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(GstNxRenderer, gst_nx_renderer, GST_TYPE_BASE_SINK,
			_do_init);

static void gst_nx_renderer_set_property(GObject *object,
					 guint prop_id,
					 const GValue *value,
					 GParamSpec *pspec);
static void gst_nx_renderer_get_property(GObject *object,
					 guint prop_id,
					 GValue *value,
					 GParamSpec *pspec);
static void gst_nx_renderer_finalize(GObject *obj);

static GstStateChangeReturn gst_nx_renderer_change_state(GstElement *element,
	GstStateChange transition);

static GstFlowReturn gst_nx_renderer_preroll(GstBaseSink *bsink,
					     GstBuffer *buffer);
static GstFlowReturn gst_nx_renderer_render(GstBaseSink *bsink,
					     GstBuffer *buffer);
static gboolean gst_nx_renderer_event(GstBaseSink *bsink, GstEvent *event);
static gboolean gst_nx_renderer_query(GstBaseSink *bsink, GstQuery *query);

static void gst_nx_renderer_class_init(GstNxRendererClass *klass)
{
	GObjectClass *gobject_class;
	GstElementClass *gstelement_class;
	GstBaseSinkClass *gstbase_sink_class;

	GST_DEBUG("ENTERED");

	gobject_class = G_OBJECT_CLASS(klass);
	gstelement_class = GST_ELEMENT_CLASS(klass);
	gstbase_sink_class = GST_BASE_SINK_CLASS(klass);

	gobject_class->set_property = gst_nx_renderer_set_property;
	gobject_class->get_property = gst_nx_renderer_get_property;
	gobject_class->finalize = gst_nx_renderer_finalize;

	/* TODO : install property */

	gst_element_class_set_static_metadata(gstelement_class,
		"Nx Renderer",
		"Sink",
		"Renderer to display GEM FD to Nexell KMS Driver",
		"Sungwoo Park <swpark@nexell.co.kr> ");
	gst_element_class_add_pad_template(gstelement_class,
		gst_static_pad_template_get(&sinktemplate));

	gstelement_class->change_state =
		GST_DEBUG_FUNCPTR(gst_nx_renderer_change_state);

	gstbase_sink_class->event =
		GST_DEBUG_FUNCPTR(gst_nx_renderer_event);
	gstbase_sink_class->preroll =
		GST_DEBUG_FUNCPTR(gst_nx_renderer_preroll);
	gstbase_sink_class->render =
		GST_DEBUG_FUNCPTR(gst_nx_renderer_render);
	gstbase_sink_class->query =
		GST_DEBUG_FUNCPTR(gst_nx_renderer_query);

	GST_DEBUG("LEAVED");
}
static guint32
_get_pixel_format(GstNxRenderer *me, guint32 mm_format)
{
	guint32 drm_format = -1;
	GST_DEBUG_OBJECT(me, "mm_format = %d", mm_format);
	switch(mm_format) {
		case MM_PIXEL_FORMAT_YUYV:
			drm_format = 0;//DRM_FORMAT_YUYV;
			break;
		case MM_PIXEL_FORMAT_I420:
			drm_format = 8;//DRM_FORMAT_YUV420;
		default:
			break;
	}
	GST_DEBUG_OBJECT(me, "drm_format = %d", drm_format);
	return drm_format;
}

static struct dp_device *dp_device_init(int fd)
{
	struct dp_device *device;
	int err;

	err = drmSetClientCap(fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
	if (err < 0)
		GST_ERROR("drmSetClientCap() failed: %d %m\n", err);

	device = dp_device_open(fd);
	if (!device) {
		GST_ERROR("fail : device open() %m\n");
		return NULL;
	}

	return device;
}

/* FIXME: this function must be moved to nx-renderer library */
static int choose_format(struct dp_plane *plane, int select)
{
	int format;
	int size;

	size = ARRAY_SIZE(dp_formats);

	if (select > (size-1)) {
		GST_ERROR("fail : not support format index (%d) over size %d\n",
			  select, size);
		return -EINVAL;
	}
	format = dp_formats[select];

	if (!dp_plane_supports_format(plane, format)) {
		GST_ERROR("fail : not support %s\n", dp_forcc_name(format));
		return -EINVAL;
	}

	GST_DEBUG("format: %s\n", dp_forcc_name(format));
	return format;
}

static struct dp_framebuffer *framebuffer_alloc(struct dp_device *device,
						int format,
						MMVideoBuffer *mm_buf)
{
	struct dp_framebuffer *fb;
	int i;
	uint32_t offset;
	int err;
	int gem;

	fb = calloc(1, sizeof(struct dp_framebuffer));
	if (!fb) {
		GST_ERROR("failed to alloc dp_framebuffer");
		return NULL;
	}

	gem = import_gem_from_flink(device->fd, mm_buf->handle.gem[0]);
	if (gem < 0) {
		GST_ERROR("failed to import gem from flink(%d)",
			  mm_buf->handle.gem[0]);
		dp_framebuffer_free(fb);
		return NULL;
	}

	fb->device = device;
	fb->width = mm_buf->width[0];
	fb->height = mm_buf->height[0];
	fb->format = format;

	fb->seperated = mm_buf->handle_num > 1 ? true : false;
	fb->planes = mm_buf->handle_num;

	for (i = 0; i < mm_buf->handle_num; i++) {
		fb->sizes[i] = mm_buf->size[i];
	}

	/* FIXME; currently supports only YUV420 format */
	offset = 0;
	//for (i = 0; i < 3; i ++) {
	for (i = 0; i < mm_buf->plane_num; i ++) {
		/* fb->handles[i] = mm_buf->handle.gem[0]; */
		fb->handles[i] = gem;
		fb->pitches[i] = mm_buf->stride_width[i];
		fb->ptrs[i] = NULL;
		fb->offsets[i] = offset;
		offset += mm_buf->stride_width[i] * mm_buf->stride_height[i];
	}

	err = dp_framebuffer_addfb2(fb);
	if (err < 0) {
		GST_ERROR("failed to addfb2");
		dp_framebuffer_free(fb);
		return NULL;
	}

	return fb;
}

/* type : DRM_PLANE_TYPE_OVERLAY | NX_PLANE_TYPE_VIDEO */
int get_plane_index_by_type(struct dp_device *device, uint32_t port, uint32_t type)
{
	int i = 0, j = 0;
	int ret = -1;
	drmModeObjectPropertiesPtr props;
	uint32_t plane_type = -1;
	int find_idx = 0;
	int prop_type = -1;

	/* type overlay or primary or cursor */
	int layer_type = type & 0x3;
	/*	display_type video : 1 << 4, rgb : 0 << 4	*/
	int display_type = type & 0xf0;

	for (i = 0; i < device->num_planes; i++) {
		props = drmModeObjectGetProperties(device->fd,
					device->planes[i]->id,
					DRM_MODE_OBJECT_PLANE);
		if (!props)
			return -ENODEV;

		prop_type = -1;
		plane_type = NX_PLANE_TYPE_VIDEO;

		for (j = 0; j < props->count_props; j++) {
			drmModePropertyPtr prop;

			prop = drmModeGetProperty(device->fd, props->props[j]);
			if (prop) {
				DP_DBG("plane.%2d %d.%d [%s]\n",
					device->planes[i]->id,
					props->count_props,
					j,
					prop->name);

				if (strcmp(prop->name, "type") == 0)
					prop_type = (int)props->prop_values[j];

				if (strcmp(prop->name, "alphablend") == 0)
					plane_type = NX_PLANE_TYPE_RGB;

				drmModeFreeProperty(prop);
			}
		}
		drmModeFreeObjectProperties(props);

		DP_DBG("prop type : %d, layer type : %d\n",
				prop_type, layer_type);
		DP_DBG("disp type : %d, plane type : %d\n",
				display_type, plane_type);
		DP_DBG("find idx : %d, port : %d\n\n",
				find_idx, port);

		if (prop_type == layer_type && display_type == plane_type
				&& find_idx == port) {
			ret = i;
			break;
		}

		if (prop_type == layer_type && display_type == plane_type)
			find_idx++;

		if (find_idx > port)
			break;
	}

	return ret;
}

int get_plane_prop_id_by_property_name(int drm_fd, uint32_t plane_id,
		char *prop_name)
{
	int res = -1;
	drmModeObjectPropertiesPtr props;
	props = drmModeObjectGetProperties(drm_fd, plane_id,
			DRM_MODE_OBJECT_PLANE);

	if (props) {
		int i;

		for (i = 0; i < props->count_props; i++) {
			drmModePropertyPtr this_prop;
			this_prop = drmModeGetProperty(drm_fd, props->props[i]);

			if (this_prop) {
				DP_DBG("prop name : %s, prop id: %d, param prop id: %s\n",
						this_prop->name,
						this_prop->prop_id,
						prop_name
						);

				if (!strncmp(this_prop->name, prop_name,
							DRM_PROP_NAME_LEN)) {

					res = this_prop->prop_id;

					drmModeFreeProperty(this_prop);
					break;
				}
				drmModeFreeProperty(this_prop);
			}
		}
		drmModeFreeObjectProperties(props);
	}

	return res;
}

int set_priority_video_plane(struct dp_device *device, uint32_t plane_idx,
		uint32_t set_idx)
{
	uint32_t plane_id = device->planes[plane_idx]->id;
	uint32_t prop_id = get_plane_prop_id_by_property_name(device->fd,
							plane_id,
							"video-priority");
	int res = -1;

	res = drmModeObjectSetProperty(device->fd,
			plane_id,
			DRM_MODE_OBJECT_PLANE,
			prop_id,
			set_idx);

	return res;
}
static struct dp_framebuffer *dp_buffer_init(struct dp_device *device,
					     int display_index,
					     int plane_index,
					     int op_format,
					     MMVideoBuffer *mm_buf)
{
	struct dp_plane *plane;
	int format;
	uint32_t video_type, video_index;
	int32_t res;

	video_type = DRM_PLANE_TYPE_OVERLAY | NX_PLANE_TYPE_VIDEO;
	video_index = get_plane_index_by_type(device, 0, video_type);
	if (video_index < 0) {
		DP_ERR("fail : not found matching layer\n");
		return -1;
	}
	plane = device->planes[video_index];
	if (res = set_priority_video_plane(device, video_index, 1)) {
		DP_ERR("failed setting priority : %d\n", res);
		return NULL;
	}

	format = choose_format(plane, op_format);
	if (format < 0) {
		GST_ERROR("no matching format found for op_format %d",
			  op_format);
		return NULL;
	}

	return framebuffer_alloc(device, format, mm_buf);
}

static int dp_plane_update(struct dp_device *device, struct dp_framebuffer *fb,
			   int display_index, int plane_index)
{
	struct dp_plane *plane;
	uint32_t video_type, video_index;

	video_type = DRM_PLANE_TYPE_OVERLAY | NX_PLANE_TYPE_VIDEO;
	video_index = get_plane_index_by_type(device, 0, video_type);
	if (video_index < 0) {
		DP_ERR("fail : not found matching layer\n");
		return -1;
	}
	plane = device->planes[video_index];
	/* TODO : source crop, dest position */
	return dp_plane_set(plane, fb,
			    0, 0, fb->width, fb->height,
			    0, 0, fb->width, fb->height);
}

static void gst_nx_renderer_init(GstNxRenderer *me)
{
	int i;
	GST_DEBUG("ENTERED");
	GST_DEBUG_OBJECT(me, "init");

	me->drm_fd = open("/dev/dri/card0", O_RDWR);
	if (me->drm_fd < 0)
		GST_ERROR_OBJECT(me, "failed to open drm device");

	me->dp_device = dp_device_init(me->drm_fd);
	if (!me->dp_device)
		GST_ERROR_OBJECT(me, "failed to dp_device_init");

	for (i = 0; i < MAX_BUFFER_COUNT; i++)
		me->fb[i] = NULL;

	gst_base_sink_set_sync(GST_BASE_SINK(me), DEFAULT_SYNC);
	GST_DEBUG("LEAVED");
}

static void gst_nx_renderer_finalize(GObject *obj)
{
	int i;
	GstNxRenderer *me;

	me = GST_NX_RENDERER(obj);

	GST_DEBUG("ENTERED");

	free_gem(me->drm_fd, me->fb[0]->handles[0]);
	for (i = 0; i < MAX_BUFFER_COUNT; i++)
		if (me->fb[i])
			dp_framebuffer_delfb2(me->fb[i]);

	if (me->dp_device)
		dp_device_close(me->dp_device);
	if (me->drm_fd >= 0)
		close(me->drm_fd);

	GST_DEBUG_OBJECT(obj, "finalize");
	G_OBJECT_CLASS(parent_class)->finalize(obj);
	GST_DEBUG("LEAVED");
}

static void gst_nx_renderer_set_property(GObject *object, guint prop_id,
					 const GValue *value, GParamSpec *pspec)
{
	GstNxRenderer *me;

	me = GST_NX_RENDERER(object);

	GST_DEBUG_OBJECT(me, "set property");

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void gst_nx_renderer_get_property(GObject *object, guint prop_id,
					 GValue *value,
					 GParamSpec *pspec)
{
	GstNxRenderer *me;

	me = GST_NX_RENDERER(object);

	GST_DEBUG_OBJECT(me, "get property");

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static gboolean gst_nx_renderer_event(GstBaseSink *bsink, GstEvent *event)
{
	GstNxRenderer *me = GST_NX_RENDERER(bsink);

	GST_DEBUG_OBJECT(me, "event");

	return GST_BASE_SINK_CLASS(parent_class)->event(bsink, event);
}

static GstFlowReturn gst_nx_renderer_preroll(GstBaseSink *bsink,
					     GstBuffer *buffer)
{
	GstNxRenderer *me = GST_NX_RENDERER(bsink);

	GST_DEBUG_OBJECT(me, "preroll buffer");

	return GST_FLOW_OK;
}

static GstFlowReturn gst_nx_renderer_render(GstBaseSink *bsink, GstBuffer *buf)
{
	GstNxRenderer *me = GST_NX_RENDERER(bsink);
	GstMemory *meta_block = NULL;
	MMVideoBuffer *mm_buf = NULL;
	GstMapInfo info;
	GstFlowReturn ret = GST_FLOW_OK;

	GST_OBJECT_LOCK(me);

	GST_DEBUG_OBJECT(me, "render buffer");

	/* gst_buffer_map(buf, &info, GST_MAP_READ); */
	/* gst_util_dump_mem(info.data, info.size); */
	/* gst_buffer_unmap(buf, &info); */

	memset(&info, 0, sizeof(GstMapInfo));

	meta_block = gst_buffer_peek_memory(buf, 0);
	gst_memory_map(meta_block, &info, GST_MAP_READ);

	mm_buf = (MMVideoBuffer *)info.data;
	if (!mm_buf) {
		GST_ERROR_OBJECT(me, "failed to get MMVideoBuffer!");
	} else {
		GST_DEBUG_OBJECT(me, "get MMVideoBuffer");
		GST_DEBUG_OBJECT(me,
			"type: 0x%x, width: %d, height: %d, plane_num: %d, handle_num: %d, index: %d, format : %d",
			mm_buf->type, mm_buf->width[0],
			mm_buf->height[0], mm_buf->plane_num,
			mm_buf->handle_num, mm_buf->buffer_index,
			mm_buf->format);

		if (!me->fb[mm_buf->buffer_index]) {
			struct dp_framebuffer *fb;
			guint32 format = -1;

			format = _get_pixel_format(me, mm_buf->format);
			GST_DEBUG_OBJECT(me, "format is [%d]", format);
			fb = dp_buffer_init(me->dp_device, 0, 0,
					    format,
					    mm_buf);
			if (!fb) {
				GST_ERROR_OBJECT(me, "failed to dp_buffer_init for index %d",
						 mm_buf->buffer_index);
				ret = GST_FLOW_ERROR;
				goto done;
			}
			GST_DEBUG_OBJECT(me, "make fb for %d",
					 mm_buf->buffer_index);

			me->fb[mm_buf->buffer_index] = fb;
		}

		GST_DEBUG_OBJECT(me, "display fb %d", mm_buf->buffer_index);
		if(me->dp_device) {
			dp_plane_update(me->dp_device, me->fb[mm_buf->buffer_index],
				0, 0);
		}
	}

done:
	gst_memory_unmap(meta_block, &info);

	GST_OBJECT_UNLOCK(me);

	return ret;
}

static gboolean gst_nx_renderer_query(GstBaseSink *bsink, GstQuery *query)
{
	gboolean ret;

	switch (GST_QUERY_TYPE(query)) {
	case GST_QUERY_SEEKING:{
		GstFormat fmt;

		gst_query_parse_seeking(query, &fmt, NULL, NULL, NULL);
		gst_query_set_seeking(query, fmt, FALSE, 0, -1);
		ret = TRUE;
		break;
	}
	default:
		ret = GST_BASE_SINK_CLASS(parent_class)->query(bsink, query);
		break;
	}

	return ret;
}

static GstStateChangeReturn gst_nx_renderer_change_state(GstElement *element,
	GstStateChange transition)
{
	/* TODO */
	/* return GST_STATE_CHANGE_SUCCESS; */
	return GST_ELEMENT_CLASS(parent_class)->change_state(element,
							     transition);
}

static gboolean plugin_init(GstPlugin *plugin)
{
	return gst_element_register(plugin, "nxrenderer",
				    GST_RANK_PRIMARY + 101,
				    gst_nx_renderer_get_type());
}

#ifndef PACKAGE
#define PACKAGE "nexell.gst.renderer"
#endif
GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
		  GST_VERSION_MINOR,
		  nxrenderer,
		  "Nexell Display DRM Video plug-in",
		  plugin_init,
		  "0.0.1",
		  "LGPL",
		  "Nexell Co",
		  "http://www.nexell.co.kr")
