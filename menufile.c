/* WindowLab - an X11 window manager
 * Copyright (c) 2001-2010 Nick Gravgaard
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "windowlab.h"

// semaphor activated by SIGHUP
int do_menuitems;

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
	char menurcpath[PATH_MAX], *c;
	extern int errno;

	menuitems = (MenuItem *)malloc(MAX_MENUITEMS_SIZE);
	if (menuitems == NULL)
	{
		err("Unable to allocate menu items array.");
		return;
	}
	memset(menuitems, 0, MAX_MENUITEMS_SIZE);

	snprintf(menurcpath, sizeof(menurcpath), "%s/.windowlab/windowlab.menurc", getenv("HOME"));
#ifdef DEBUG
	printf("trying to open: %s\n", menurcpath);
#endif
	if ((menufile = fopen(menurcpath, "r")) == NULL)
	{
		ssize_t len;
		// get location of the executable
		if ((len = readlink("/proc/self/exe", menurcpath, PATH_MAX - 1)) == -1)
		{
			err("readlink() /proc/self/exe failed: %s\n", strerror(errno));
			menurcpath[0] = '.';
			menurcpath[1] = '\0';
		}
		else
		{
			// insert null to end the file path properly
			menurcpath[len] = '\0';
		}
		if ((c = strrchr(menurcpath, '/')) != NULL)
		{
			*c = '\0';
		}
		if ((c = strrchr(menurcpath, '/')) != NULL)
		{
			*c = '\0';
		}
		strncat(menurcpath, "/etc/windowlab.menurc", PATH_MAX - strlen(menurcpath) - 1);
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
		while ((!feof(menufile)) && (!ferror(menufile)) && (num_menuitems < MAX_MENUITEMS))
		{
			char menustr[STR_SIZE] = "";
			fgets(menustr, STR_SIZE, menufile);
			if (strlen(menustr) != 0)
			{
				char *pmenustr = menustr;
				while (pmenustr[0] == ' ' || pmenustr[0] == '\t')
				{
					pmenustr++;
				}
				if (pmenustr[0] != '#')
				{
					char labelstr[STR_SIZE] = "", commandstr[STR_SIZE] = "";
					if (parseline(pmenustr, labelstr, commandstr))
					{
						menuitems[num_menuitems].label = (char *)malloc(strlen(labelstr) + 1);
						menuitems[num_menuitems].command = (char *)malloc(strlen(commandstr) + 1);
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
		err("can't find ~/.windowlab/windowlab.menurc, %s or %s\n", menurcpath, DEF_MENURC);
		menuitems[0].command = (char *)malloc(strlen(NO_MENU_COMMAND) + 1);
		strcpy(menuitems[0].command, NO_MENU_COMMAND);
		menuitems[0].label = (char *)malloc(strlen(NO_MENU_LABEL) + 1);
		strcpy(menuitems[0].label, NO_MENU_LABEL);
		num_menuitems = 1;
	}

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
	// menu items have been built
	do_menuitems = 0;
}

int parseline(char *menustr, char *labelstr, char *commandstr)
{
	int success = 0;
	int menustrlen = strlen(menustr);
	char *ptemp = NULL;
	char *menustrcpy = (char *)malloc(menustrlen + 1);

	if (menustrcpy == NULL)
	{
		return 0;
	}

	strcpy(menustrcpy, menustr);
	ptemp = strtok(menustrcpy, ":");

	if (ptemp != NULL)
	{
		strcpy(labelstr, ptemp);
		ptemp = strtok(NULL, "\n");
		if (ptemp != NULL) // right of ':' is not empty
		{
			while (*ptemp == ' ' || *ptemp == '\t')
			{
				ptemp++;
			}
			if (*ptemp != '\0' && *ptemp != '\r' && *ptemp != '\n')
			{
				strcpy(commandstr, ptemp);
				success = 1;
			}
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
