#include <dirent.h>
#include <string.h>
#include "utils.h"
#include "xml_utils.h"
#include "db.h"

void craftshop_load(xmlNodePtr node);

void
xml_boot(void)
{
	DIR *dir;
	dirent *file;
	xmlDocPtr doc;
	xmlNodePtr node;
	char path[256];
	int file_count = 0;

	dir = opendir(XML_PREFIX);
	if (!dir) {
		slog("SYSERR: XML directory does not exist");
		return;
	}

	while ((file = readdir(dir)) != NULL) {
		if (!rindex(file->d_name, '.'))
			continue;
		if (strcmp(rindex(file->d_name, '.'), ".xml"))
			continue;

		snprintf(path, 255, "%s/%s", XML_PREFIX, file->d_name);
		doc = xmlParseFile(path);
		if (!doc) {
			slog("SYSERR: XML parse error while loading %s", path);
			continue;
		}

		node = xmlDocGetRootElement(doc);
		if (!node) {
			xmlFreeDoc(doc);
			slog("SYSERR: XML file %s is empty", path);
			continue;
		}

		while (node) {
			// Parse different nodes here.
			if (xmlMatches(node->name, "craftshop"))
				craftshop_load(node);
			else
				slog("SYSERR: Invalid XML object '%s' in %s", node->name, path);
			node = node->next;
		}

		xmlFreeDoc(doc);
	file_count++;
	}
	closedir(dir);

	if (file_count)
		slog("%d xml file%s loaded", file_count, (file_count == 1) ? "":"s");
	else
		slog("No xml files loaded");
}
