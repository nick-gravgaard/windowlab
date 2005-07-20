/* WindowLab - an X11 window manager
 * Copyright (c) 2001-2005 Nick Gravgaard
 * me at nickgravgaard.com
 * http://nickgravgaard.com/windowlab/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "windowlab.h"

static int parseline(char *, char *, char *);

MenuItem *menuitems = NULL;
unsigned int num_menuitems;
#ifdef XFT
XGlyphInfo extents;
#endif

void get_menuitems(void)
{
	unsigned int i, button_startx = 0;
	FILE *menufile = NULL;
	char menurcpath[PATH_MAX];
	extern int errno;

	menuitems = malloc(MAX_MENUITEMS_SIZE);
	memset(menuitems, '\0', MAX_MENUITEMS_SIZE);

	snprintf(menurcpath, sizeof menurcpath, "%s/.windowlab/windowlab.menurc", getenv("HOME"));
#ifdef DEBUG
	printf("trying to open: %s\n", menurcpath);
#endif
	if ((menufile = fopen(menurcpath, "r")) == NULL)
	{
		// get location of the executable
		if (readlink("/proc/self/exe", menurcpath, PATH_MAX) == -1)
		{
			err("readlink() /proc/self/exe failed: %s\n", strerror(errno));
			menurcpath[0] = '.';
			menurcpath[1] = '\0';
		}
		*(strrchr(menurcpath, '/')) = '\0';
		*(strrchr(menurcpath, '/')) = '\0';
		strncat(menurcpath, "/etc/windowlab.menurc", PATH_MAX);
#ifdef DEBUG
		printf("trying to open: %s\n", menurcpath);
#endif
		if ((menufile = fopen(menurcpath, "r")) == NULL)
		{
#ifdef DEBUG
			printf("trying to open: %s\n", DEF_MENURC);
#endif
			menufile = fopen(DEF_MENURC, "r");
		}
	}
	if (menufile != NULL)
	{
		num_menuitems = 0;
		while ((!feof(menufile)) && (!ferror(menufile)))
		{
			char menustr[STR_SIZE];
			strcpy(menustr, "/0");
			fgets(menustr, STR_SIZE, menufile);
			if (strlen(menustr) > 0)
			{
				if (menustr[0] != '#')
				{
					char labelstr[STR_SIZE];
					char commandstr[STR_SIZE];
					if (parseline(menustr, labelstr, commandstr))
					{
						menuitems[num_menuitems].label = malloc(strlen(labelstr) + 1);
						menuitems[num_menuitems].command = malloc(strlen(commandstr) + 1);
						strcpy(menuitems[num_menuitems].label, labelstr);
						strcpy(menuitems[num_menuitems].command, commandstr);
						num_menuitems++;
					}
				}
			}
		}
		fclose(menufile);
	}
	else
	{
		// one menu item - xterm
		err("can't find ~/.windowlab/windowlab.menurc, %s or " DEF_MENURC "\n", menurcpath);
		menuitems[0].command = malloc(strlen(NO_MENU_COMMAND) + 1);
		strcpy(menuitems[0].command, NO_MENU_COMMAND);
		menuitems[0].label = malloc(strlen(NO_MENU_LABEL) + 1);
		strcpy(menuitems[0].label, NO_MENU_LABEL);
		num_menuitems = 1;
	}

	menuitems = realloc((void *)menuitems, MAX_MENUITEMS_SIZE);

	for (i = 0; i < num_menuitems; i++)
	{
		menuitems[i].x = button_startx;
#ifdef XFT
		XftTextExtents8(dsply, xftfont, (unsigned char *)menuitems[i].label, strlen(menuitems[i].label), &extents);
		menuitems[i].width = extents.width + (SPACE * 4);
#else
		menuitems[i].width = XTextWidth(font, menuitems[i].label, strlen(menuitems[i].label)) + (SPACE * 4);
#endif
		button_startx += menuitems[i].width + 1;
	}
}

int parseline(char *menustr, char *labelstr, char *commandstr)
{
	int success = 1;
	int menustrlen = strlen(menustr);
	char *ptemp = NULL;
	char *menustrcpy = (char *)malloc(menustrlen + 1);
	strcpy(menustrcpy, menustr);
	ptemp = strtok(menustrcpy, ":");

	if (ptemp != NULL)
	{
		strcpy(labelstr, ptemp);
		if (ptemp != NULL)
		{
			ptemp = strtok(NULL, ":");
			if (ptemp != NULL)
			{
				strcpy(commandstr, ptemp);
			}
			else
			{
				success = 0;
			}
		}
		else
		{
			success = 0;
		}
	}
	if (menustrcpy != NULL)
	{
		free(menustrcpy);
	}
	return success;
}

void free_menuitems(void)
{
	unsigned int i;
	if (menuitems != NULL)
	{
		for (i = 0; i < num_menuitems; i++)
		{
			if (menuitems[i].label != NULL)
			{
				free(menuitems[i].label);
				menuitems[i].label = NULL;
			}
			if (menuitems[i].command != NULL)
			{
				free(menuitems[i].command);
				menuitems[i].command = NULL;
			}
		}
		free(menuitems);
		menuitems = NULL;
	}
}
