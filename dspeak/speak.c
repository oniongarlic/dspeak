/*
 * This file is part of dspeak
 *
 * Copyright (C) 2010-2011 Kaj-Michael Lang
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <espeak/speak_lib.h>

#include "speak.h"

void
speak_set_parameters(guint speed, guint pitch)
{
espeak_SetParameter(espeakRATE, speed, 0);
espeak_SetParameter(espeakPITCH, pitch, 0);
}

gboolean
speak_init(gchar *voice, guint speed, guint pitch)
{
gint r;
r=espeak_Initialize(AUDIO_OUTPUT_PLAYBACK, 250, NULL, 0);
if (r==-1)
	return FALSE;

espeak_SetVoiceByName(voice);
speak_set_parameters(speed, pitch);
espeak_SetParameter(espeakVOLUME, 100, 0);
return TRUE;
}

void
speak_deinit(void)
{
espeak_Terminate();
}

gboolean
speak_text(const gchar *text)
{
espeak_ERROR ee;

g_debug("Speak: [%s] (%zu)", text, strlen(text));
ee=espeak_Synth(text, strlen(text)+1, 0, POS_CHARACTER, 0, espeakCHARS_UTF8, NULL, NULL);
if (ee==EE_BUFFER_FULL) {
	g_warning("Espeak buffer full");
	return FALSE;
}
return TRUE;
}

gboolean
speak_speaking(void)
{
return espeak_IsPlaying()==1 ? TRUE : FALSE;
}

gboolean
speak_stop(void)
{
return espeak_Cancel()==EE_OK ? TRUE : FALSE;
}
