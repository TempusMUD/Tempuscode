/* ************************************************************************
*   File: weather.c                                     Part of CircleMUD *
*  Usage: functions handling time and the weather                         *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: weather.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#include <stdio.h>
#include <string.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "db.h"

extern struct time_info_data time_info;
extern int lunar_day;

void weather_and_time(int mode);
void another_hour(int mode);
void weather_change(void);
void zone_weather_change(struct zone_data *zone);
void jet_stream(void);

int jet_stream_state = TRUE;
void
weather_and_time(int mode)
{
	another_hour(mode);
	if (mode)
		weather_change();
}

void
set_local_time(struct zone_data *zone, struct time_info_data *local_time)
{

	local_time->hours = time_info.hours + zone->hour_mod;
	local_time->day = time_info.day;
	local_time->month = time_info.month;
	local_time->year = time_info.year + zone->year_mod;

	while (local_time->hours > 23) {
		local_time->hours -= 24;
		local_time->day++;
	}
	while (local_time->hours < 0) {
		local_time->hours += 24;
		local_time->day--;
	}

	while (local_time->day > 34) {
		local_time->day -= 35;
		local_time->month++;
	}

	while (local_time->month > 15) {
		local_time->month -= 16;
		local_time->year++;
	}
}

void
another_hour(int mode)
{
	struct zone_data *zone = NULL;
	struct time_info_data local_time;
	int LUNAR_RISE_TIME;
	int lunar_phase;

	time_info.hours++;

	while (time_info.hours > 23) {
		time_info.hours -= 24;
		time_info.day++;
		lunar_day++;
		if (lunar_day > 23)
			lunar_day = 0;

		lunar_phase = get_lunar_phase(lunar_day);
		switch (lunar_phase) {
		case MOON_FULL:
			send_to_clerics("The time of full moon has begun.\r\n"); break;
		case MOON_WANE_GIBBOUS:
			send_to_clerics("The time of full moon has passed.\r\n"); break;
		case MOON_NEW:
			send_to_clerics("The dark time of new moon has begun.\r\n"); break;
		case MOON_WAX_CRESCENT:
			send_to_clerics("The dark time of new moon has passed.\r\n"); break;
		}
	}
	lunar_phase = get_lunar_phase(lunar_day);

	while (time_info.day > 34) {
		time_info.day -= 35;
		time_info.month++;
	}
	while (time_info.month > 15) {
		time_info.month -= 16;
		time_info.year++;
	}

	if (!mode)
		return;

	for (zone = zone_table; zone; zone = zone->next) {
		if (!zone->world)
			continue;

		if (!PRIME_MATERIAL_ROOM(zone->world) ||
			zone->time_frame == TIME_TIMELESS ||
			GET_PLANE(zone->world) == PLANE_UNDERDARK)
			continue;

		set_local_time(zone, &local_time);

		if (local_time.hours == (4 - daylight_mod[(int)local_time.month])) {
			if (!number(0, 2))
				send_to_zone("A small blue sun rises in the east.\r\n", zone,
					1);
			else if (zone->weather->sky == SKY_CLOUDLESS)
				send_to_zone
					("The blue sun rises in the cloudless skies of the east.\r\n",
					zone, 1);
			else if (zone->weather->sky == SKY_CLOUDY)
				send_to_zone
					("Through the veil of clouds, you see a glimmer of blue to the east.\r\n",
					zone, 1);
			else if (zone->weather->sky == SKY_RAINING)
				send_to_zone
					("The faint light of the blue sun appears through the rain to the east.\r\n",
					zone, 1);
			else
				send_to_zone("A small blue sun rises in the east.\r\n", zone,
					1);

			/* SUN_RISE */
		} else if (local_time.hours ==
			(5 - daylight_mod[(int)local_time.month])) {
			zone->weather->sunlight = SUN_RISE;
			if (!number(0, 2))
				send_to_zone
					("The huge red sun rises over the eastern horizon.\r\n",
					zone, 1);
			else if (zone->weather->sky == SKY_CLOUDLESS)
				send_to_zone
					("The blazing red giant rises into the clear sky.\r\n",
					zone, 1);
			else if (zone->weather->sky == SKY_CLOUDY)
				send_to_zone
					("The red light of the sun appears in the cloudy sky.\r\n",
					zone, 1);
			else
				send_to_zone
					("The huge red sun rises over the eastern horizon.\r\n",
					zone, 1);

		} else if (local_time.hours ==
			(6 - daylight_mod[(int)local_time.month])) {
			zone->weather->sunlight = SUN_LIGHT;
			if (!number(0, 2))
				send_to_zone
					("The day has begun, both suns above the horizon.\r\n",
					zone, 1);
			else if (zone->weather->sky == SKY_CLOUDLESS)
				send_to_zone("The day begins under a cloudless sky.\r\n", zone,
					1);
			else if (zone->weather->sky == SKY_CLOUDY)
				send_to_zone("The cloudy day has begun.\r\n", zone, 1);
			else if (zone->weather->sky == SKY_RAINING)
				send_to_zone("The rainy day has begun.\r\n", zone, 1);
			else if (zone->weather->sky == SKY_LIGHTNING)
				send_to_zone("The day begins, immersed in a thunderstorm.\r\n",
					zone, 1);
			else
				send_to_zone
					("The day has begun, both suns above the horizon.\r\n",
					zone, 1);

			if (local_time.day == 0) {
				switch (local_time.month) {
				case 2:
					send_to_zone("It is the first day of spring.\r\n", zone,
						1);
					break;
				case 6:
					send_to_zone("It is the first day of summer.\r\n", zone,
						1);
					break;
				case 10:
					send_to_zone("It is the first day of autumn.\r\n", zone,
						1);
					break;
				case 14:
					send_to_zone("It is the first day of winter.\r\n", zone,
						1);
					break;
				}
			}
			break;

		} else if (local_time.hours == 12) {
			send_to_zone("The red giant is high overhead now, at noon.\r\n",
				zone, 1);

		} else if (local_time.hours ==
			(20 + daylight_mod[(int)local_time.month])) {
			zone->weather->sunlight = SUN_SET;
			send_to_zone("The red giant sun slowly sets in the west.\r\n",
				zone, 1);

		} else if (local_time.hours ==
			(21 + daylight_mod[(int)local_time.month])) {
			zone->weather->sunlight = SUN_SET;
			send_to_zone("The red giant sun slowly sets in the west.\r\n",
				zone, 1);

		} else if (local_time.hours ==
			(22 + daylight_mod[(int)local_time.month])) {
			zone->weather->sunlight = SUN_DARK;
			send_to_zone("The night has begun.\r\n", zone, 1);
		}

		/* lunar stuff here */

		if (lunar_phase == MOON_NEW)
			return;

		LUNAR_RISE_TIME = lunar_day + 5;
		if (LUNAR_RISE_TIME > 23)
			LUNAR_RISE_TIME -= 24;

		if (local_time.hours == LUNAR_RISE_TIME) {
			zone->weather->moonlight = MOON_SKY_RISE;
			if (zone->weather->sky || !lunar_day)
				return;
			sprintf(buf, "The %s moon rises in the east.\r\n",
				lunar_phases[lunar_phase]);
			send_to_zone(buf, zone, 1);
		}
		if (local_time.hours == LUNAR_RISE_TIME + 1 ||
			local_time.hours == LUNAR_RISE_TIME - 23) {
			zone->weather->moonlight = MOON_SKY_EAST;
		}
		if (local_time.hours == LUNAR_RISE_TIME + 7 ||
			local_time.hours == LUNAR_RISE_TIME - 17) {
			zone->weather->moonlight = MOON_SKY_HIGH;

			if (zone->weather->sky || !lunar_day)
				return;
			sprintf(buf, "The %s moon is directly overhead.\r\n",
				lunar_phases[lunar_phase]);
			send_to_zone(buf, zone, 1);

		}
		if (local_time.hours == LUNAR_RISE_TIME + 8 ||
			local_time.hours == LUNAR_RISE_TIME - 16) {
			zone->weather->moonlight = MOON_SKY_WEST;
		}
		if (local_time.hours == LUNAR_RISE_TIME + 13 ||
			local_time.hours == LUNAR_RISE_TIME - 11) {
			zone->weather->moonlight = MOON_SKY_SET;
			sprintf(buf, "The %s moon begins to sink low in the west.\r\n",
				lunar_phases[lunar_phase]);
			send_to_zone(buf, zone, 1);

		}
		if (local_time.hours == LUNAR_RISE_TIME + 14 ||
			local_time.hours == LUNAR_RISE_TIME - 10) {
			zone->weather->moonlight = MOON_SKY_NONE;
			if (zone->weather->sky || !lunar_day)
				return;
			sprintf(buf, "The %s moon sets in the west.\r\n",
				lunar_phases[lunar_phase]);
			send_to_zone(buf, zone, 1);
		}
	}
}

void
weather_change(void)
{
	struct zone_data *zone = NULL;

	if (jet_stream_state)
		jet_stream();
	for (zone = zone_table; zone; zone = zone->next)
		zone_weather_change(zone);
}

void
jet_stream()
{

	struct zone_data *fr_zone = NULL, *zone = NULL;
	int count = 0, hour, change;

	for (hour = 2; hour > -4; hour--) {
		for (zone = zone_table; zone; zone = zone->next) {

			if (zone->hour_mod != hour)
				continue;

			for (fr_zone = zone_table, count = 0, change = 0; fr_zone;
				fr_zone = fr_zone->next) {

				if (fr_zone->hour_mod != hour - 1 ||
					fr_zone->lattitude > zone->lattitude + 1 ||
					fr_zone->lattitude < zone->lattitude - 1)
					continue;

				count++;
				change +=
					((fr_zone->weather->pressure +
						zone->weather->pressure) / 2) -
					zone->weather->pressure;
			}

			if (count)
				change /= count;

			zone->weather->change += count;
		}
	}
}

#define CHANGE_NONE         0
#define CHANGE_CLOUDS_START 1
#define CHANGE_RAIN_START   2
#define CHANGE_CLOUDS_STOP  3
#define CHANGE_STORM_START  4
#define CHANGE_RAIN_STOP    5
#define CHANGE_STORM_STOP   6

void
zone_weather_change(struct zone_data *zone)
{
	int diff = 1, change;
	struct time_info_data local_time;

	if (ZONE_FLAGGED(zone, ZONE_NOWEATHER))
		return;

	set_local_time(zone, &local_time);

	if ((local_time.month >= 9) && (local_time.month <= 16))
		diff = (zone->weather->pressure > 1000 ? -1 :
			zone->weather->pressure < 970 ? 1 : 0);
	else
		diff = (zone->weather->pressure > 1025 ? -1 :
			zone->weather->pressure < 990 ? 1 : 0);

	zone->weather->change += diff * dice(4, 4) + (number(-15, 15));

	zone->weather->change = MAX(MIN(zone->weather->change, 15), -15);

	zone->weather->pressure += zone->weather->change;

	zone->weather->pressure = MAX(MIN(zone->weather->pressure, 1040), 960);

	change = CHANGE_NONE;

	switch (zone->weather->sky) {
	case SKY_CLOUDLESS:
		if (zone->weather->pressure < 990)
			change = 1;
		else if (zone->weather->pressure < 1010)
			if (dice(1, 4) == 1)
				change = 1;
		break;
	case SKY_CLOUDY:
		if (zone->weather->pressure < 970)
			change = 2;
		else if (zone->weather->pressure < 990)
			if (dice(1, 4) == 1)
				change = 2;
			else
				change = 0;
		else if (zone->weather->pressure > 1030)
			if (dice(1, 4) == 1)
				change = 3;

		break;
	case SKY_RAINING:
		if (zone->weather->pressure < 970)
			if (dice(1, 4) == 1)
				change = 4;
			else
				change = 0;
		else if (zone->weather->pressure > 1030)
			change = 5;
		else if (zone->weather->pressure > 1010)
			if (dice(1, 4) == 1)
				change = 5;

		break;
	case SKY_LIGHTNING:
		if (zone->weather->pressure > 1010)
			change = 6;
		else if (zone->weather->pressure > 990)
			if (dice(1, 4) == 1)
				change = 6;

		break;
	default:
		change = 0;
		zone->weather->sky = SKY_CLOUDLESS;
		break;
	}

	switch (change) {
	case 0:
		break;
	case 1:
		if (!number(0, 2))
			send_to_zone("The sky starts to get cloudy.\r\n", zone, 1);
		else if (zone->weather->sunlight == SUN_RISE)
			send_to_zone("A veil of clouds appears with the sunrise.\r\n",
				zone, 1);
		else if (zone->weather->sunlight == SUN_SET)
			send_to_zone
				("As the suns dip below the horizon, clouds cover the sky.\r\n",
				zone, 1);
		else if (zone->weather->sunlight == SUN_DARK)
			send_to_zone("A layer of clouds obscures the stars.\r\n", zone, 1);
		else
			send_to_zone("Clouds begin to gather in the sky.\r\n", zone, 1);
		zone->weather->sky = SKY_CLOUDY;
		break;
	case 2:
		if (!number(0, 2))
			send_to_zone("It starts to rain.\r\n", zone, 1);
		else if (zone->weather->sunlight == SUN_RISE)
			send_to_zone
				("Rain begins to fall across the land, as the double suns rise.\r\n",
				zone, 1);
		else if (zone->weather->sunlight == SUN_SET)
			send_to_zone
				("As the suns sink below the horizon, rain begins to fall.\r\n",
				zone, 1);
		else if (zone->weather->sunlight == SUN_DARK)
			send_to_zone("Rain begins to fall from the night sky.\r\n", zone,
				1);
		else
			send_to_zone("Rain begins to fall from the sky.\r\n", zone, 1);
		zone->weather->sky = SKY_RAINING;
		break;
	case 3:
		if (!number(0, 2))
			send_to_zone("The clouds disappear.\r\n", zone, 1);
		else if (zone->weather->sunlight == SUN_RISE)
			send_to_zone("As the suns rise, the clouds in the sky vanish.\r\n",
				zone, 1);
		else if (zone->weather->sunlight == SUN_SET)
			send_to_zone
				("The last of the clouds in the sky vanish with the sunset.\r\n",
				zone, 1);
		else if (zone->weather->sunlight == SUN_DARK)
			send_to_zone
				("The clouds part, leaving a clear view of the stars.\r\n",
				zone, 1);
		else
			send_to_zone("The clouds have vanished, leaving a clear sky.\r\n",
				zone, 1);
		zone->weather->sky = SKY_CLOUDLESS;
		break;
	case 4:
		if (!number(0, 2))
			send_to_zone("Lightning starts to show in the sky.\r\n", zone, 1);
		else if (zone->weather->sunlight == SUN_RISE)
			send_to_zone
				("Lightning and thunder begin to accompany the sunrise.\r\n",
				zone, 1);
		else if (zone->weather->sunlight == SUN_SET)
			send_to_zone
				("As the suns set, lightning begins to flicker across the sky.\r\n",
				zone, 1);
		else if (zone->weather->sunlight == SUN_DARK)
			send_to_zone("The starry sky is lit by flashes of lightning.\r\n",
				zone, 1);
		else
			send_to_zone
				("You can see lightning flickering across the sky.\r\n", zone,
				1);
		zone->weather->sky = SKY_LIGHTNING;
		break;
	case 5:
		if (!number(0, 2))
			send_to_zone("The rain stops.\r\n", zone, 1);
		else if (zone->weather->sunlight == SUN_RISE)
			send_to_zone
				("The rain ceases as the twin suns rise above the horizon.\r\n",
				zone, 1);
		else if (zone->weather->sunlight == SUN_SET)
			send_to_zone
				("The rain ceases as the twin suns sink below the horizon.\r\n",
				zone, 1);
		else if (zone->weather->sunlight == SUN_DARK)
			send_to_zone("The rain stops falling from the night sky.\r\n",
				zone, 1);
		else
			send_to_zone("The rain slacks up, then stops.\r\n", zone, 1);
		zone->weather->sky = SKY_CLOUDY;
		break;
	case 6:
		if (!number(0, 2))
			send_to_zone("The lightning stops.\r\n", zone, 1);
		else if (zone->weather->sunlight == SUN_RISE)
			send_to_zone
				("The lightning ceases as the suns rise on a rainy day.\r\n",
				zone, 1);
		else if (zone->weather->sunlight == SUN_SET)
			send_to_zone("As the suns disappear, the lightning ceases.\r\n",
				zone, 1);
		else if (zone->weather->sunlight == SUN_DARK)
			send_to_zone("The lightning and thunder ceases.\r\n", zone, 1);
		else
			send_to_zone("The lightning seems to have stopped.\r\n", zone, 1);
		zone->weather->sky = SKY_RAINING;
		break;
	default:
		break;
	}
}
