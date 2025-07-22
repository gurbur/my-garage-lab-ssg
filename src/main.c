#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdbool.h>

#include "include/ignore_handler.h"
#include "include/template_engine.h"
#include "include/site_context.h"
#include "include/config_loader.h"
#include "include/build_process.h"
#include "include/file_utils.h"

#define MAX_PATH_LENGTH 1024

int main(int argc, char *argv[]) {
	const char* vault_path = ".";
	if (argc > 1) {
		vault_path = argv[1];
	}

	const char* output_dir = "ssg_output";

	printf("----- STARTING SSG BUILD -----\n");
	printf("Vault Path: %s\n", vault_path);
	printf("Output Dir: %s\n", output_dir);
	printf("------------------------------\n\n");

	printf("[STEP 1] Loading global context from config.json...\n");
	TemplateContext* global_context = create_template_context();
	load_config("config.json", global_context);

	const char* loaded_title = get_from_context(global_context, "site_title");
	const char* loaded_static_dir = get_from_context(global_context, "build.static_dir");
	const char* loaded_image_dir = get_from_context(global_context, "build.image_dir");
	printf("[DEBUG] Loaded site_title: %s\n", loaded_title ? loaded_title : "Not Found");
	printf("[DEBUG] Loaded build.static_dir: %s\n", loaded_static_dir ? loaded_static_dir : "Not Found");
	printf("[DEBUG] Loaded build.image_dir: %s\n", loaded_image_dir? loaded_image_dir : "Not Found");

	printf("[STEP 2] Scanning vault and creating site context...\n");
	SiteContext* site_context = create_site_context(vault_path);

	printf("[STEP 3] Loading .ssgignore and preparing output directory...\n");
	load_ssgignore(vault_path);

	printf("[STEP 4] Generating sidebar...\n");
	generate_sidebar_html(site_context, global_context);

	printf("[STEP 5] Preparing output directory...\n");
	mkdir(output_dir, 0755);

	build_site(vault_path, site_context, global_context);

	printf("[STEP 6] Copying static files...\n");
	const char* static_dir = get_from_context(global_context, "build.static_dir");
	if (static_dir && strlen(static_dir) > 0) {
		char dest_static_path[MAX_PATH_LENGTH];
		snprintf(dest_static_path, sizeof(dest_static_path), "%s/%s", output_dir, static_dir);
		copy_static_files(static_dir, dest_static_path);
	} else {
		printf("[INFO] 'build.static_dir' not found in config.json, skipping.\n");
	}

	printf("[STEP 7] Copying image files...\n");
	const char* image_dir = get_from_context(global_context, "build.image_dir");
	if (image_dir && strlen(image_dir) > 0) {
		char dest_image_path[MAX_PATH_LENGTH];
		snprintf(dest_image_path, sizeof(dest_image_path), "%s/%s", output_dir, image_dir);
		copy_static_files(image_dir, dest_image_path);
	} else {
		printf("[INFO] 'build.image_dir' not found in config.json, skipping.\n");
	}

	printf("[STEP 8] Cleaning up...\n");
	free_site_context(site_context);
	free_template_context(global_context);
	free_ignore_patterns();

	printf("---- BUILD FINISHED SUCCESSFULLY! ----\n");

	return EXIT_SUCCESS;
}
