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

#include "gstnxrenderer.h"

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

static void gst_nx_renderer_init(GstNxRenderer *me)
{
	GST_DEBUG("ENTERED");
	GST_DEBUG_OBJECT(me, "init");
	gst_base_sink_set_sync(GST_BASE_SINK(me), DEFAULT_SYNC);
	GST_DEBUG("LEAVED");
}

static void gst_nx_renderer_finalize(GObject *obj)
{
	GST_DEBUG("ENTERED");
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
	GstMapInfo info;

	GST_OBJECT_LOCK(me);

	GST_DEBUG_OBJECT(me, "render buffer");

	gst_buffer_map(buf, &info, GST_MAP_READ);
	gst_util_dump_mem(info.data, info.size);
	gst_buffer_unmap(buf, &info);

	GST_OBJECT_UNLOCK(me);

	return GST_FLOW_OK;
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
