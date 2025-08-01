#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../libs/cjson/cJSON.h"
#include "../include/config_loader.h"

char* read_file_into_string(const char* filepath);

static void populate_context_from_json(TemplateContext* context, cJSON* json_node, const char* prefix) {
	cJSON* item = NULL;
	cJSON_ArrayForEach(item, json_node) {
		char new_key[256];
		if (strlen(prefix) > 0) {
			snprintf(new_key, sizeof(new_key), "%s.%s", prefix, item->string);
		} else {
			snprintf(new_key, sizeof(new_key), "%s", item->string);
		}

		if (cJSON_IsString(item)) {
			add_to_context(context, new_key, item->valuestring);
		} else if (cJSON_IsObject(item)) {
			populate_context_from_json(context, item, new_key);
		}
	}
}

void load_config(const char* config_path, TemplateContext* context) {
	char* config_string = read_file_into_string(config_path);
	if (!config_string) {
		printf("Warning: config.json not found. Using default values.\n");
		return;
	}

	cJSON* config_json = cJSON_Parse(config_string);
	free(config_string);

	if (config_json == NULL) {
		const char* error_ptr = cJSON_GetErrorPtr();
		if (error_ptr != NULL) {
			fprintf(stderr, "Error parsing config.json before: %s\n", error_ptr);
		}
		cJSON_Delete(config_json);
		return;
	}

	populate_context_from_json(context, config_json, "");

	cJSON* category_slugs_json = cJSON_GetObjectItemCaseSensitive(config_json, "category_slugs");
	if (cJSON_IsObject(category_slugs_json)) {
		cJSON* category_slug = NULL;
		cJSON_ArrayForEach(category_slug, category_slugs_json) {
			if (cJSON_IsString(category_slug) && (category_slug->valuestring != NULL)) {
				char key[256];
				snprintf(key, sizeof(key), "category_slugs.%s", category_slug->string);
				add_to_context(context, key, category_slug->valuestring);
			}
		}
	}
	cJSON_Delete(config_json);
}
