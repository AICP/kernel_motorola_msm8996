/*
 * svghelper.c - helper functions for outputting svg
 *
 * (C) Copyright 2009 Intel Corporation
 *
 * Authors:
 *     Arjan van de Ven <arjan@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "svghelper.h"

static u64 first_time, last_time;
static u64 turbo_frequency, max_freq;


#define SLOT_MULT 30.0
#define SLOT_HEIGHT 25.0
#define WIDTH 1000.0

static u64 total_height;
static FILE *svgfile;

static double cpu2slot(int cpu)
{
	return 2 * cpu + 1;
}

static double cpu2y(int cpu)
{
	return cpu2slot(cpu) * SLOT_MULT;
}

static double time2pixels(u64 time)
{
	double X;

	X = WIDTH * (time - first_time) / (last_time - first_time);
	return X;
}

void open_svg(const char *filename, int cpus, int rows)
{

	svgfile = fopen(filename, "w");
	if (!svgfile) {
		fprintf(stderr, "Cannot open %s for output\n", filename);
		return;
	}
	total_height = (1 + rows + cpu2slot(cpus)) * SLOT_MULT;
	fprintf(svgfile, "<?xml version=\"1.0\" standalone=\"no\"?> \n");
	fprintf(svgfile, "<svg width=\"%4.1f\" height=\"%llu\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\">\n", WIDTH, total_height);

	fprintf(svgfile, "<defs>\n  <style type=\"text/css\">\n    <![CDATA[\n");

	fprintf(svgfile, "      rect          { stroke-width: 1; }\n");
	fprintf(svgfile, "      rect.process  { fill:rgb(180,180,180); fill-opacity:0.9; stroke-width:1;   stroke:rgb(  0,  0,  0); } \n");
	fprintf(svgfile, "      rect.process2 { fill:rgb(180,180,180); fill-opacity:0.9; stroke-width:0;   stroke:rgb(  0,  0,  0); } \n");
	fprintf(svgfile, "      rect.sample   { fill:rgb(  0,  0,255); fill-opacity:0.8; stroke-width:0;   stroke:rgb(  0,  0,  0); } \n");
	fprintf(svgfile, "      rect.blocked  { fill:rgb(255,  0,  0); fill-opacity:0.5; stroke-width:0;   stroke:rgb(  0,  0,  0); } \n");
	fprintf(svgfile, "      rect.waiting  { fill:rgb(255,255,  0); fill-opacity:0.3; stroke-width:0;   stroke:rgb(  0,  0,  0); } \n");
	fprintf(svgfile, "      rect.cpu      { fill:rgb(192,192,192); fill-opacity:0.2; stroke-width:0.5; stroke:rgb(128,128,128); } \n");
	fprintf(svgfile, "      rect.pstate   { fill:rgb(128,128,128); fill-opacity:0.8; stroke-width:0; } \n");
	fprintf(svgfile, "      rect.c1       { fill:rgb(255,214,214); fill-opacity:0.5; stroke-width:0; } \n");
	fprintf(svgfile, "      rect.c2       { fill:rgb(255,172,172); fill-opacity:0.5; stroke-width:0; } \n");
	fprintf(svgfile, "      rect.c3       { fill:rgb(255,130,130); fill-opacity:0.5; stroke-width:0; } \n");
	fprintf(svgfile, "      rect.c4       { fill:rgb(255, 88, 88); fill-opacity:0.5; stroke-width:0; } \n");
	fprintf(svgfile, "      rect.c5       { fill:rgb(255, 44, 44); fill-opacity:0.5; stroke-width:0; } \n");
	fprintf(svgfile, "      rect.c6       { fill:rgb(255,  0,  0); fill-opacity:0.5; stroke-width:0; } \n");
	fprintf(svgfile, "      line.pstate   { stroke:rgb(255,255,  0); stroke-opacity:0.8; stroke-width:2; } \n");

	fprintf(svgfile, "    ]]>\n   </style>\n</defs>\n");
}

void svg_box(int Yslot, u64 start, u64 end, const char *type)
{
	if (!svgfile)
		return;

	fprintf(svgfile, "<rect x=\"%4.8f\" width=\"%4.8f\" y=\"%4.1f\" height=\"%4.1f\" class=\"%s\"/>\n",
		time2pixels(start), time2pixels(end)-time2pixels(start), Yslot * SLOT_MULT, SLOT_HEIGHT, type);
}

void svg_sample(int Yslot, int cpu, u64 start, u64 end, const char *type)
{
	double text_size;
	if (!svgfile)
		return;

	fprintf(svgfile, "<rect x=\"%4.8f\" width=\"%4.8f\" y=\"%4.1f\" height=\"%4.1f\" class=\"%s\"/>\n",
		time2pixels(start), time2pixels(end)-time2pixels(start), Yslot * SLOT_MULT, SLOT_HEIGHT, type);

	text_size = (time2pixels(end)-time2pixels(start));
	if (cpu > 9)
		text_size = text_size/2;
	if (text_size > 1.25)
		text_size = 1.25;
	if (text_size > 0.0001)
		fprintf(svgfile, "<text transform=\"translate(%1.6f,%1.6f)\" font-size=\"%1.6fpt\">%i</text>\n",
			time2pixels(start), Yslot *  SLOT_MULT + SLOT_HEIGHT - 1, text_size,  cpu + 1);

}

static char *cpu_model(void)
{
	static char cpu_m[255];
	char buf[256];
	FILE *file;

	cpu_m[0] = 0;
	/* CPU type */
	file = fopen("/proc/cpuinfo", "r");
	if (file) {
		while (fgets(buf, 255, file)) {
			if (strstr(buf, "model name")) {
				strncpy(cpu_m, &buf[13], 255);
				break;
			}
		}
		fclose(file);
	}
	return cpu_m;
}

void svg_cpu_box(int cpu, u64 __max_freq, u64 __turbo_freq)
{
	char cpu_string[80];
	if (!svgfile)
		return;

	max_freq = __max_freq;
	turbo_frequency = __turbo_freq;

	fprintf(svgfile, "<rect x=\"%4.8f\" width=\"%4.8f\" y=\"%4.1f\" height=\"%4.1f\" class=\"cpu\"/>\n",
		time2pixels(first_time),
		time2pixels(last_time)-time2pixels(first_time),
		cpu2y(cpu), SLOT_MULT+SLOT_HEIGHT);

	sprintf(cpu_string, "CPU %i", (int)cpu+1);
	fprintf(svgfile, "<text transform=\"translate(%4.1f,%4.1f)\">%s</text>\n",
		10+time2pixels(first_time), cpu2y(cpu) + SLOT_HEIGHT/2, cpu_string);

	fprintf(svgfile, "<text transform=\"translate(%4.1f,%4.1f)\" font-size=\"1.25pt\">%s</text>\n",
		10+time2pixels(first_time), cpu2y(cpu) + SLOT_MULT + SLOT_HEIGHT - 4, cpu_model());
}

void svg_process(int cpu, u64 start, u64 end, const char *type, const char *name)
{
	double width;

	if (!svgfile)
		return;

	fprintf(svgfile, "<rect x=\"%4.8f\" width=\"%4.8f\" y=\"%4.1f\" height=\"%4.1f\" class=\"%s\"/>\n",
		time2pixels(start), time2pixels(end)-time2pixels(start), cpu2y(cpu), SLOT_MULT+SLOT_HEIGHT, type);
	width = time2pixels(end)-time2pixels(start);
	if (width > 6)
		width = 6;

	if (width > 0.001)
		fprintf(svgfile, "<text  transform=\"translate(%4.5f,%4.5f) rotate(90)\" font-size=\"%3.4fpt\">%s</text>\n",
			time2pixels(start), cpu2y(cpu), width, name);
}

void svg_cstate(int cpu, u64 start, u64 end, int type)
{
	double width;
	char style[128];

	if (!svgfile)
		return;


	if (type > 6)
		type = 6;
	sprintf(style, "c%i", type);

	fprintf(svgfile, "<rect class=\"%s\" x=\"%4.8f\" width=\"%4.8f\" y=\"%4.1f\" height=\"%4.1f\"/>\n",
		style,
		time2pixels(start), time2pixels(end)-time2pixels(start),
		cpu2y(cpu), SLOT_MULT+SLOT_HEIGHT);

	width = time2pixels(end)-time2pixels(start);
	if (width > 6)
		width = 6;

	if (width > 0.05)
		fprintf(svgfile, "<text  transform=\"translate(%4.5f,%4.5f) rotate(90)\" font-size=\"%3.4fpt\">C%i</text>\n",
			time2pixels(start), cpu2y(cpu), width, type);
}

static char *HzToHuman(unsigned long hz)
{
	static char buffer[1024];
	unsigned long long Hz;

	memset(buffer, 0, 1024);

	Hz = hz;

	/* default: just put the Number in */
	sprintf(buffer, "%9lli", Hz);

	if (Hz > 1000)
		sprintf(buffer, " %6lli Mhz", (Hz+500)/1000);

	if (Hz > 1500000)
		sprintf(buffer, " %6.2f Ghz", (Hz+5000.0)/1000000);

	if (Hz == turbo_frequency)
		sprintf(buffer, "Turbo");

	return buffer;
}

void svg_pstate(int cpu, u64 start, u64 end, u64 freq)
{
	double height = 0;

	if (!svgfile)
		return;

	if (max_freq)
		height = freq * 1.0 / max_freq * (SLOT_HEIGHT + SLOT_MULT);
	height = 1 + cpu2y(cpu) + SLOT_MULT + SLOT_HEIGHT - height;
	fprintf(svgfile, "<line x1=\"%4.8f\" x2=\"%4.8f\" y1=\"%4.1f\" y2=\"%4.1f\" class=\"pstate\"/>\n",
		time2pixels(start), time2pixels(end), height, height);
	fprintf(svgfile, "<text transform=\"translate(%4.1f,%4.1f)\" font-size=\"0.25pt\">%s</text>\n",
		time2pixels(start), height+0.9, HzToHuman(freq));

}


void svg_partial_wakeline(u64 start, int row1, int row2)
{
	double height;

	if (!svgfile)
		return;


	if (row1 < row2) {
		if (row1)
			fprintf(svgfile, "<line x1=\"%4.8f\" y1=\"%4.2f\" x2=\"%4.8f\" y2=\"%4.2f\" style=\"stroke:rgb(32,255,32);stroke-width:0.009\"/>\n",
				time2pixels(start), row1 * SLOT_MULT + SLOT_HEIGHT,  time2pixels(start), row1 * SLOT_MULT + SLOT_HEIGHT + SLOT_MULT/32);

		if (row2)
			fprintf(svgfile, "<line x1=\"%4.8f\" y1=\"%4.2f\" x2=\"%4.8f\" y2=\"%4.2f\" style=\"stroke:rgb(32,255,32);stroke-width:0.009\"/>\n",
				time2pixels(start), row2 * SLOT_MULT - SLOT_MULT/32,  time2pixels(start), row2 * SLOT_MULT);
	} else {
		if (row2)
			fprintf(svgfile, "<line x1=\"%4.8f\" y1=\"%4.2f\" x2=\"%4.8f\" y2=\"%4.2f\" style=\"stroke:rgb(32,255,32);stroke-width:0.009\"/>\n",
				time2pixels(start), row2 * SLOT_MULT + SLOT_HEIGHT,  time2pixels(start), row2 * SLOT_MULT + SLOT_HEIGHT + SLOT_MULT/32);

		if (row1)
			fprintf(svgfile, "<line x1=\"%4.8f\" y1=\"%4.2f\" x2=\"%4.8f\" y2=\"%4.2f\" style=\"stroke:rgb(32,255,32);stroke-width:0.009\"/>\n",
				time2pixels(start), row1 * SLOT_MULT - SLOT_MULT/32,  time2pixels(start), row1 * SLOT_MULT);
	}
	height = row1 * SLOT_MULT;
	if (row2 > row1)
		height += SLOT_HEIGHT;
	if (row1)
		fprintf(svgfile, "<circle  cx=\"%4.8f\" cy=\"%4.2f\" r = \"0.01\"  style=\"fill:rgb(32,255,32)\"/>\n",
			time2pixels(start), height);
}

void svg_wakeline(u64 start, int row1, int row2)
{
	double height;

	if (!svgfile)
		return;


	if (row1 < row2)
		fprintf(svgfile, "<line x1=\"%4.8f\" y1=\"%4.2f\" x2=\"%4.8f\" y2=\"%4.2f\" style=\"stroke:rgb(32,255,32);stroke-width:0.009\"/>\n",
			time2pixels(start), row1 * SLOT_MULT + SLOT_HEIGHT,  time2pixels(start), row2 * SLOT_MULT);
	else
		fprintf(svgfile, "<line x1=\"%4.8f\" y1=\"%4.2f\" x2=\"%4.8f\" y2=\"%4.2f\" style=\"stroke:rgb(32,255,32);stroke-width:0.009\"/>\n",
			time2pixels(start), row2 * SLOT_MULT + SLOT_HEIGHT,  time2pixels(start), row1 * SLOT_MULT);

	height = row1 * SLOT_MULT;
	if (row2 > row1)
		height += SLOT_HEIGHT;
	fprintf(svgfile, "<circle  cx=\"%4.8f\" cy=\"%4.2f\" r = \"0.01\"  style=\"fill:rgb(32,255,32)\"/>\n",
			time2pixels(start), height);
}

void svg_interrupt(u64 start, int row)
{
	if (!svgfile)
		return;

	fprintf(svgfile, "<circle  cx=\"%4.8f\" cy=\"%4.2f\" r = \"0.01\"  style=\"fill:rgb(255,128,128)\"/>\n",
			time2pixels(start), row * SLOT_MULT);
	fprintf(svgfile, "<circle  cx=\"%4.8f\" cy=\"%4.2f\" r = \"0.01\"  style=\"fill:rgb(255,128,128)\"/>\n",
			time2pixels(start), row * SLOT_MULT + SLOT_HEIGHT);
}

void svg_text(int Yslot, u64 start, const char *text)
{
	if (!svgfile)
		return;

	fprintf(svgfile, "<text transform=\"translate(%4.1f,%4.1f)\">%s</text>\n",
		time2pixels(start), Yslot * SLOT_MULT+SLOT_HEIGHT/2, text);
}

static void svg_legenda_box(int X, const char *text, const char *style)
{
	double boxsize;
	boxsize = SLOT_HEIGHT / 2;

	fprintf(svgfile, "<rect x=\"%i\" width=\"%4.8f\" y=\"0\" height=\"%4.1f\" class=\"%s\"/>\n",
		X, boxsize, boxsize, style);
	fprintf(svgfile, "<text transform=\"translate(%4.1f, %4.1f)\" font-size=\"%4.4fpt\">%s</text>\n",
		X + boxsize + 5, boxsize, 0.8 * boxsize, text);
}

void svg_legenda(void)
{
	if (!svgfile)
		return;

	svg_legenda_box(0,	"Running", "sample");
	svg_legenda_box(100,	"Idle","rect.c1");
	svg_legenda_box(200,	"Deeper Idle", "rect.c3");
	svg_legenda_box(350,	"Deepest Idle", "rect.c6");
	svg_legenda_box(550,	"Sleeping", "process2");
	svg_legenda_box(650,	"Waiting for cpu", "waiting");
	svg_legenda_box(800,	"Blocked on IO", "blocked");
}

void svg_time_grid(u64 start, u64 end)
{
	u64 i;

	first_time = start;
	last_time = end;

	first_time = first_time / 100000000 * 100000000;

	if (!svgfile)
		return;

	i = first_time;
	while (i < last_time) {
		int color = 220;
		double thickness = 0.075;
		if ((i % 100000000) == 0) {
			thickness = 0.5;
			color = 192;
		}
		if ((i % 1000000000) == 0) {
			thickness = 2.0;
			color = 128;
		}

		fprintf(svgfile, "<line x1=\"%4.8f\" y1=\"%4.2f\" x2=\"%4.8f\" y2=\"%llu\" style=\"stroke:rgb(%i,%i,%i);stroke-width:%1.3f\"/>\n",
			time2pixels(i), SLOT_MULT/2, time2pixels(i), total_height, color, color, color, thickness);

		i += 10000000;
	}
}

void svg_close(void)
{
	if (svgfile) {
		fprintf(svgfile, "</svg>\n");
		fclose(svgfile);
		svgfile = NULL;
	}
}
