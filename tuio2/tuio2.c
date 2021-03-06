/*
 * Copyright (c) 2015 Hanspeter Portner (dev@open-music-kontrollers.ch)
 *
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the Artistic License 2.0 as published by
 * The Perl Foundation.
 *
 * This source is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * Artistic License 2.0 for more details.
 *
 * You should have received a copy of the Artistic License 2.0
 * along the source as a COPYING file. If not, obtain it from
 * http://www.perlfoundation.org/artistic_license_2_0.
 */

#include <string.h>
#include <stdio.h>
#include <math.h>

#include <chimaera.h>
#include <chimutil.h>
#include <config.h>

#include <tuio2.h>

static const char *frm_str = "/tuio2/frm";
static const char *tok_str = "/tuio2/tok";
static const char *alv_str = "/tuio2/alv";

static const char *frm_fmt = "itis";
static const char *tok_fmt [2] = {
	[0] = "iiifff",					// !derivatives
	[1] = "iiiffffffff"			// derivatives
};
static char alv_fmt [BLOB_MAX+1]; // this has a variable string len

static int32_t alv_ids [BLOB_MAX];
static uint_fast8_t counter;

static const uint32_t dim = (SENSOR_N << 16) | 1;
static char source [NAME_LENGTH + 9];

static osc_data_t *pack;
static osc_data_t *bndl;

static void
tuio2_init(void)
{
	sprintf(source, "%s:0@0x%02x%02x%02x%02x", config.name,
		config.comm.ip[0], config.comm.ip[1], config.comm.ip[2], config.comm.ip[3]); //FIXME this needs to be updated
}

static osc_data_t *
tuio2_engine_frame_cb(osc_data_t *buf, osc_data_t *end, CMC_Frame_Event *fev)
{
	osc_data_t *buf_ptr = buf;
	osc_data_t *itm;

	if(cmc_engines_active + config.dump.enabled > 1)
		buf_ptr = osc_start_bundle_item(buf_ptr, end, &pack);
	buf_ptr = osc_start_bundle(buf_ptr, end, fev->offset, &bndl);

	buf_ptr = osc_start_bundle_item(buf_ptr, end, &itm);
	{
		buf_ptr = osc_set_path(buf_ptr, end, frm_str);
		buf_ptr = osc_set_fmt(buf_ptr, end, frm_fmt);

		buf_ptr = osc_set_int32(buf_ptr, end, fev->fid);
		buf_ptr = osc_set_timetag(buf_ptr, end, fev->now);
		buf_ptr = osc_set_int32(buf_ptr, end, dim);
		buf_ptr = osc_set_string(buf_ptr, end, source);
	}
	buf_ptr = osc_end_bundle_item(buf_ptr, end, itm);

	counter = 0; // reset token pointer

	return buf_ptr;
}

static osc_data_t *
tuio2_engine_end_cb(osc_data_t *buf, osc_data_t *end, CMC_Frame_Event *fev)
{
	(void)fev;
	osc_data_t *buf_ptr = buf;
	osc_data_t *itm;

	uint_fast8_t i;
	for(i=0; i<counter; i++)
		alv_fmt[i] = OSC_INT32;
	alv_fmt[counter] = '\0';

	buf_ptr = osc_start_bundle_item(buf_ptr, end, &itm);
	{
		buf_ptr = osc_set_path(buf_ptr, end, alv_str);
		buf_ptr = osc_set_fmt(buf_ptr, end, alv_fmt);

		for(i=0; i<counter; i++)
			buf_ptr = osc_set_int32(buf_ptr, end, alv_ids[i]);
	}
	buf_ptr = osc_end_bundle_item(buf_ptr, end, itm);

	buf_ptr = osc_end_bundle(buf_ptr, end, bndl);
	if(cmc_engines_active + config.dump.enabled > 1)
		buf_ptr = osc_end_bundle_item(buf_ptr, end, pack);

	return buf_ptr;
}

static osc_data_t *
tuio2_engine_token_cb(osc_data_t *buf, osc_data_t *end, CMC_Blob_Event *bev)
{
	osc_data_t *buf_ptr = buf;
	osc_data_t *itm;

	buf_ptr = osc_start_bundle_item(buf_ptr, end, &itm);
	{
		buf_ptr = osc_set_path(buf_ptr, end, tok_str);
		buf_ptr = osc_set_fmt(buf_ptr, end, tok_fmt[config.tuio2.derivatives]);

		buf_ptr = osc_set_int32(buf_ptr, end, bev->sid);
		buf_ptr = osc_set_int32(buf_ptr, end, bev->pid);
		buf_ptr = osc_set_int32(buf_ptr, end, bev->gid);
		buf_ptr = osc_set_float(buf_ptr, end, bev->x);
		buf_ptr = osc_set_float(buf_ptr, end, bev->y);
		buf_ptr = osc_set_float(buf_ptr, end, bev->pid == CMC_NORTH ? 0.f : M_PI);

		if(config.tuio2.derivatives)
		{
			buf_ptr = osc_set_float(buf_ptr, end, bev->vx);
			buf_ptr = osc_set_float(buf_ptr, end, bev->vy);
			buf_ptr = osc_set_float(buf_ptr, end, 0.f); // angular velocity
			buf_ptr = osc_set_float(buf_ptr, end, bev->m); // acceleration
			buf_ptr = osc_set_float(buf_ptr, end, 0.f); // angular acceleration
		}
	}
	buf_ptr = osc_end_bundle_item(buf_ptr, end, itm);

	alv_ids[counter++] = bev->sid;

	return buf_ptr;
}

CMC_Engine tuio2_engine = {
	tuio2_init,
	tuio2_engine_frame_cb,
	tuio2_engine_token_cb,
	NULL,
	tuio2_engine_token_cb,
	tuio2_engine_end_cb
};

/*
 * Config
 */
static uint_fast8_t
_tuio2_enabled(const char *path, const char *fmt, uint_fast8_t argc, osc_data_t *buf)
{
	uint_fast8_t res = config_check_bool(path, fmt, argc, buf, &config.tuio2.enabled);
	cmc_engines_update();
	return res;
}

static uint_fast8_t
_tuio2_derivatives(const char *path, const char *fmt, uint_fast8_t argc, osc_data_t *buf)
{
	return config_check_bool(path, fmt, argc, buf, &config.tuio2.derivatives);
}

/*
 * Query
 */

const OSC_Query_Item tuio2_tree [] = {
	OSC_QUERY_ITEM_METHOD("enabled", "Enable/disable", _tuio2_enabled, config_boolean_args),
	OSC_QUERY_ITEM_METHOD("derivatives", "Calculate derivatives", _tuio2_derivatives, config_boolean_args),
};
