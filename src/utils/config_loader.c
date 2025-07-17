#include <stdio.h>
#include <stdlib.h>

#include "../libs/cjson/cJSON.h"
#include "../include/config_loader.h"

char* read_file_into_string(const char* filepath);

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

	cJSON *item = NULL;
	cJSON_ArrayForEach(item, config_json) {
		if (cJSON_IsString(item) && (item->valuestring != NULL)) {
			add_to_context(context, item->string, item->valuestring);
		}
	}

	cJSON_Delete(config_json);
}
